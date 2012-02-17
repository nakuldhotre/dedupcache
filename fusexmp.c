/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall `pkg-config fuse --cflags --libs` fusexmp.c -o fusexmp
*/

#define FUSE_USE_VERSION 26
#define _GNU_SOURCE

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef linux
/* For pread()/pwrite() */
#define _XOPEN_SOURCE 500
#endif

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>

#include "fusexmp.h"
#include <libhashkit/hashkit.h>


static struct cache_entry cache_mem[100];
struct entry hash_table1[100];
struct entry hash_table2[100];
static int hash1_entries;
static int cache_mem_entries;
static int hash2_entries;

static int
xmp_getattr (const char *path, struct stat *stbuf)
{
  int res;

  res = lstat (path, stbuf);
  if (res == -1)
    return -errno;

  return 0;
}

static int
xmp_access (const char *path, int mask)
{
  int res;

  res = access (path, mask);
  if (res == -1)
    return -errno;

  return 0;
}

static int
xmp_readlink (const char *path, char *buf, size_t size)
{
  int res;

  res = readlink (path, buf, size - 1);
  if (res == -1)
    return -errno;

  buf[res] = '\0';
  return 0;
}


static int
xmp_readdir (const char *path, void *buf, fuse_fill_dir_t filler,
             off_t offset, struct fuse_file_info *fi)
{
  DIR *dp;
  struct dirent *de;

  (void) offset;
  (void) fi;

  dp = opendir (path);
  if (dp == NULL)
    return -errno;

  while ((de = readdir (dp)) != NULL)
    {
      struct stat st;

      memset (&st, 0, sizeof (st));
      st.st_ino = de->d_ino;
      st.st_mode = de->d_type << 12;
      if (filler (buf, de->d_name, &st, 0))
        break;
    }

  closedir (dp);
  return 0;
}

static int
xmp_mknod (const char *path, mode_t mode, dev_t rdev)
{
  int res;

  /* On Linux this could just be 'mknod(path, mode, rdev)' but this
     is more portable */
  if (S_ISREG (mode))
    {
      res = open (path, O_CREAT | O_EXCL | O_WRONLY, mode);
      if (res >= 0)
        res = close (res);
    }
  else if (S_ISFIFO (mode))
    res = mkfifo (path, mode);
  else
    res = mknod (path, mode, rdev);
  if (res == -1)
    return -errno;

  return 0;
}

static int
xmp_mkdir (const char *path, mode_t mode)
{
  int res;

  res = mkdir (path, mode);
  if (res == -1)
    return -errno;

  return 0;
}

static int
xmp_unlink (const char *path)
{
  int res;

  res = unlink (path);
  if (res == -1)
    return -errno;

  return 0;
}

static int
xmp_rmdir (const char *path)
{
  int res;

  res = rmdir (path);
  if (res == -1)
    return -errno;

  return 0;
}

static int
xmp_symlink (const char *from, const char *to)
{
  int res;

  res = symlink (from, to);
  if (res == -1)
    return -errno;

  return 0;
}

static int
xmp_rename (const char *from, const char *to)
{
  int res;

  res = rename (from, to);
  if (res == -1)
    return -errno;

  return 0;
}

static int
xmp_link (const char *from, const char *to)
{
  int res;

  res = link (from, to);
  if (res == -1)
    return -errno;

  return 0;
}

static int
xmp_chmod (const char *path, mode_t mode)
{
  int res;

  res = chmod (path, mode);
  if (res == -1)
    return -errno;

  return 0;
}

static int
xmp_chown (const char *path, uid_t uid, gid_t gid)
{
  int res;

  res = lchown (path, uid, gid);
  if (res == -1)
    return -errno;

  return 0;
}

static int
xmp_truncate (const char *path, off_t size)
{
  int res;

  res = truncate (path, size);
  if (res == -1)
    return -errno;

  return 0;
}

static int
xmp_utimens (const char *path, const struct timespec ts[2])
{
  int res;
  struct timeval tv[2];

  tv[0].tv_sec = ts[0].tv_sec;
  tv[0].tv_usec = ts[0].tv_nsec / 1000;
  tv[1].tv_sec = ts[1].tv_sec;
  tv[1].tv_usec = ts[1].tv_nsec / 1000;

  res = utimes (path, tv);
  if (res == -1)
    return -errno;

  return 0;
}

static int
xmp_open (const char *path, struct fuse_file_info *fi)
{
  int res;

  res = open (path, fi->flags);
  if (res == -1)
    return -errno;

  close (res);
  return 0;
}

static int
xmp_read (const char *path, char *buf, size_t size, off_t offset,
          struct fuse_file_info *fi)
{
  int fd,i, found_table1=0, found_table2=0, found_incache_at=0;
  int res, value_length;
  struct stat stbuf;
  char key[8], *rc_str, *mem_buf;
  uint32_t hash_key;
  uint32_t block_num;
  uint32_t flags;

  (void) fi;
  fd = open (path, O_RDONLY);
  if (fd == -1)
    return -errno;
  lstat (path, &stbuf);
  fprintf (stderr, "inode = %u offset = %u size = %u",
           (unsigned int) stbuf.st_ino, offset, size);

  block_num = offset / 4096;
  memcpy(key, &stbuf.st_ino, 4);
  memcpy(key+4, &block_num,4);
   
  fprintf (stderr, "key = %u %u\n", *((uint32_t *)key),(uint32_t)*(key+4));

  for (i=0;i<hash1_entries; i++)
  {
	if (memcmp(hash_table1[i].key,key,8)==0)
        {
		memcpy(buf, cache_mem[hash_table1[i].index].data_block, size);
		res=size;
		fprintf(stderr, "Found in cache_mem !\n");
		found_table1 = 1;
		break;
	}
  }
  if (!found_table1)
  {
	fprintf(stderr, "Doing read fd = %d size = %d offset = %d\n", fd, size,offset);
  	res = pread (fd, buf, size, offset);
	if (res <4096)
	  goto end;
		
	hash_key = libhashkit_murmur(buf, size);

	for (i=0; i<hash2_entries; i++)
	{
		if (hash_table2[i].hash_key == hash_key)
		{
			found_table2 = 1;
			found_incache_at = i;
			break;
		}
	}
	if (!found_table2)
	{
		memcpy(cache_mem[cache_mem_entries].data_block, buf, size);
		memcpy(hash_table1[hash1_entries].key,key,8);
		hash_table2[hash2_entries].hash_key = hash_key;
		hash_table2[hash2_entries].index = cache_mem_entries;
		hash_table1[hash1_entries].index = cache_mem_entries;
		hash1_entries++;	
		cache_mem_entries++;
		hash2_entries++;
	}
	else
	{
		hash_table1[hash1_entries].index = found_incache_at;
		hash1_entries++;
	}
  }

  fprintf (stderr, "found_table1 = %d found_table2 = %d table1 = %d table2 = %d cache = %d\n", found_table1, found_table2, hash1_entries,
                hash2_entries, cache_mem_entries);

	 
  hash_key = libhashkit_murmur(buf, size);
  fprintf (stderr, "murmur hash key = %x\n", hash_key);

end:
  if (res == -1)
    res = -errno;

  close (fd);
  return res;
}

static int
xmp_write (const char *path, const char *buf, size_t size,
           off_t offset, struct fuse_file_info *fi)
{
  int fd;
  int res;

  (void) fi;
  fd = open (path, O_WRONLY);
  if (fd == -1)
    return -errno;

  res = pwrite (fd, buf, size, offset);
  if (res == -1)
    res = -errno;

  close (fd);
  return res;
}

static int
xmp_statfs (const char *path, struct statvfs *stbuf)
{
  int res;

  res = statvfs (path, stbuf);
  if (res == -1)
    return -errno;

  return 0;
}

static int
xmp_release (const char *path, struct fuse_file_info *fi)
{
  /* Just a stub.  This method is optional and can safely be left
     unimplemented */

  (void) path;
  (void) fi;
  return 0;
}

static int
xmp_fsync (const char *path, int isdatasync, struct fuse_file_info *fi)
{
  /* Just a stub.  This method is optional and can safely be left
     unimplemented */

  (void) path;
  (void) isdatasync;
  (void) fi;
  return 0;
}

#ifdef HAVE_SETXATTR
/* xattr operations are optional and can safely be left unimplemented */
static int
xmp_setxattr (const char *path, const char *name, const char *value,
              size_t size, int flags)
{
  int res = lsetxattr (path, name, value, size, flags);

  if (res == -1)
    return -errno;
  return 0;
}

static int
xmp_getxattr (const char *path, const char *name, char *value, size_t size)
{
  int res = lgetxattr (path, name, value, size);

  if (res == -1)
    return -errno;
  return res;
}

static int
xmp_listxattr (const char *path, char *list, size_t size)
{
  int res = llistxattr (path, list, size);

  if (res == -1)
    return -errno;
  return res;
}

static int
xmp_removexattr (const char *path, const char *name)
{
  int res = lremovexattr (path, name);

  if (res == -1)
    return -errno;
  return 0;
}
#endif /* HAVE_SETXATTR */

static void *
xmp_init (struct fuse_conn_info *conn)
{

  fprintf(stderr, "Starting Fuse file system\n");
  hash1_entries = 0;
  
  cache_mem_entries = 0;
	

}

static struct fuse_operations xmp_oper = {
  .getattr = xmp_getattr,
  .access = xmp_access,
  .readlink = xmp_readlink,
  .readdir = xmp_readdir,
  .mknod = xmp_mknod,
  .mkdir = xmp_mkdir,
  .symlink = xmp_symlink,
  .unlink = xmp_unlink,
  .rmdir = xmp_rmdir,
  .rename = xmp_rename,
  .link = xmp_link,
  .chmod = xmp_chmod,
  .chown = xmp_chown,
  .truncate = xmp_truncate,
  .utimens = xmp_utimens,
  .open = xmp_open,
  .read = xmp_read,
  .write = xmp_write,
  .statfs = xmp_statfs,
  .release = xmp_release,
  .fsync = xmp_fsync,
#ifdef HAVE_SETXATTR
  .setxattr = xmp_setxattr,
  .getxattr = xmp_getxattr,
  .listxattr = xmp_listxattr,
  .removexattr = xmp_removexattr,
#endif
  .init = xmp_init,
};

int
main (int argc, char *argv[])
{
  umask (0);
  return fuse_main (argc, argv, &xmp_oper, NULL);
}
