/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall `pkg-config fuse --cflags --libs` fusexmp.c -o fusexmp
*/

#define FUSE_USE_VERSION 26

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
#include "uthash.h"
#include "fusexmp.h"
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif
#include <libhashkit/hashkit.h>



static int xmp_getattr(const char *path, struct stat *stbuf)
{
	int res;

	res = lstat(path, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_access(const char *path, int mask)
{
	int res;

	res = access(path, mask);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_readlink(const char *path, char *buf, size_t size)
{
	int res;

	res = readlink(path, buf, size - 1);
	if (res == -1)
		return -errno;

	buf[res] = '\0';
	return 0;
}


static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
	DIR *dp;
	struct dirent *de;

	(void) offset;
	(void) fi;

	dp = opendir(path);
	if (dp == NULL)
		return -errno;

	while ((de = readdir(dp)) != NULL) {
		struct stat st;
		memset(&st, 0, sizeof(st));
		st.st_ino = de->d_ino;
		st.st_mode = de->d_type << 12;
		if (filler(buf, de->d_name, &st, 0))
			break;
	}

	closedir(dp);
	return 0;
}

static int xmp_mknod(const char *path, mode_t mode, dev_t rdev)
{
	int res;

	/* On Linux this could just be 'mknod(path, mode, rdev)' but this
	   is more portable */
	if (S_ISREG(mode)) {
		res = open(path, O_CREAT | O_EXCL | O_WRONLY, mode);
		if (res >= 0)
			res = close(res);
	} else if (S_ISFIFO(mode))
		res = mkfifo(path, mode);
	else
		res = mknod(path, mode, rdev);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_mkdir(const char *path, mode_t mode)
{
	int res;

	res = mkdir(path, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_unlink(const char *path)
{
	int res;

	res = unlink(path);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_rmdir(const char *path)
{
	int res;

	res = rmdir(path);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_symlink(const char *from, const char *to)
{
	int res;

	res = symlink(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_rename(const char *from, const char *to)
{
	int res;

	res = rename(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_link(const char *from, const char *to)
{
	int res;

	res = link(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_chmod(const char *path, mode_t mode)
{
	int res;

	res = chmod(path, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_chown(const char *path, uid_t uid, gid_t gid)
{
	int res;

	res = lchown(path, uid, gid);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_truncate(const char *path, off_t size)
{
	int res;

	res = truncate(path, size);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_utimens(const char *path, const struct timespec ts[2])
{
	int res;
	struct timeval tv[2];

	tv[0].tv_sec = ts[0].tv_sec;
	tv[0].tv_usec = ts[0].tv_nsec / 1000;
	tv[1].tv_sec = ts[1].tv_sec;
	tv[1].tv_usec = ts[1].tv_nsec / 1000;

	res = utimes(path, tv);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_open(const char *path, struct fuse_file_info *fi)
{
	int res;

	res = open(path, fi->flags);
	if (res == -1)
		return -errno;

	close(res);
	return 0;
}

int md5_cache_add(uint32_t hash_key,int index)
{
HASH_TABLE_MD5_CACHE *temp_md5=NULL;

temp_md5=malloc(sizeof(HASH_TABLE_MD5_CACHE));	

			if(temp_md5)
			{
				temp_md5->hash_key=hash_key;
				temp_md5->index=index;
				HASH_ADD_INT(md5,hash_key,temp_md5);
				return SUCCESS;
			}
				
			else
			{
				fprintf(stderr,"Memory allocation failure\n");		
		 		return FAILURE;
			}

}

void md5_cache_delete()
{
	HASH_TABLE_MD5_CACHE *temp_md5=NULL;
}

int md5_cache_find(uint32_t hash_key,HASH_TABLE_MD5_CACHE **temp_md5)
{
	HASH_FIND_INT(md5,&hash_key,(*temp_md5));

	if(*temp_md5)
	return SUCCESS;
	
	return FAILURE;
}


int binode_cache_add(char *key, int index)
{
HASH_TABLE_INODE_N_BLOCK *temp_binode=NULL;

temp_binode=(HASH_TABLE_INODE_N_BLOCK *)malloc(sizeof(HASH_TABLE_INODE_N_BLOCK));

			if(temp_binode)
			{
				memcpy(temp_binode->key,key,8);
				temp_binode->index=index;
				HASH_ADD_STR(block_inode,key,temp_binode);
				return SUCCESS;
			}
				
			else
			{
				fprintf(stderr,"Memory allocation failure\n");		
				return FAILURE;
			}

}

void binode_cache_delete()
{
HASH_TABLE_INODE_N_BLOCK *temp_binode=NULL;

}

int binode_cache_find(char *key,HASH_TABLE_INODE_N_BLOCK **temp_binode)
{
	HASH_FIND_STR(block_inode,key,(*temp_binode));

	if(temp_binode) 
	return SUCCESS;	

	else
	return FAILURE;
}

int memory_cache_add(int index,char *data,int size)
{
	HASH_TABLE_MEMORY_CACHE *temp_memory=NULL;
	temp_memory=(HASH_TABLE_MEMORY_CACHE* )malloc(sizeof(HASH_TABLE_MEMORY_CACHE ));	

			if(temp_memory)
			{
				temp_memory->index=index;
				memcpy(temp_memory->data_block,data,size);
				HASH_ADD_INT(memory,index,temp_memory);
				return SUCCESS;
			}
				
			else
			{
				fprintf(stderr,"Memory allocation failure\n");		
				return FAILURE;

			}
}

int memory_cache_find(int index,HASH_TABLE_MEMORY_CACHE **temp_memory)
{

	HASH_FIND_INT(memory,&index,(*temp_memory));

	if(*temp_memory)
	return SUCCESS;

	else
	return FAILURE;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
	int fd,status;
	int res,found_index;
	uint32_t block_num,hash_key;
	char key[8],*data_block;
        struct stat stbuf;
	HASH_TABLE_INODE_N_BLOCK *temp_binode=NULL;
	HASH_TABLE_MD5_CACHE *temp_md5=NULL;
	HASH_TABLE_MEMORY_CACHE *temp_memory=NULL;
        
        
        fprintf(stderr,"Reading...\n");
	(void) fi;
	fd = open(path, O_RDONLY);
	if (fd == -1)
		return -errno;
	
	lstat (path, &stbuf);
	block_num = offset / 4096;
        memcpy(key, &stbuf.st_ino, 4);
        memcpy(key+4, &block_num,4);

	//Check if block:inode is present in (block:inode,index) table
	status=binode_cache_find(key,&temp_binode);
	
	if(status==SUCCESS) //If found, just copy the data from (md5,index) table to buffer 
	{
		status=memory_cache_find(temp_binode->index,&temp_memory);
		if(status==SUCCESS)		
		{
			memcpy(buf,temp_memory->data_block,size);
			res=size;
			fprintf(stderr, "Found in cache_mem !\n");
		}
		else
		fprintf(stderr, "Not Found in cache_mem !\n");
	}

	else
	{
		/*if not found 
		   1. Read data block from disk
		   2. Compute md5 of the block
		   3. Use md5 as key to check if block is already in cache
		   4. if block is not present, create a new entries the following tables
                                                (block:inode,index)
						(index, data blocks)
						(md5, index) 
  
		     else create a new entry in (block:inode,index) table and update its index field appropriately.				
		*/
		fprintf(stderr, "Doing read fd = %d size = %d offset = %d\n", fd, size,offset);
  		res = pread (fd, buf, size, offset);
		if (res <4096)
	  	goto end;
		//Use md5 as key to check if the read block is already in cache
		hash_key = libhashkit_md5(buf, res);
		

		status=md5_cache_find(hash_key,&temp_md5);

		if(status==FAILURE)//if block is not present
		{
			

			//Adding a new entry into (inode:blocknumber,index) table
			status=binode_cache_add(key,count_md5_cache);
			if(status)
			goto end;
			count_block_inode++;

			//Adding a new entry into (index,data blocks) table
			status=memory_cache_add(count_md5_cache,buf,res);
			if(status)
			goto end;

			//Adding a new entry into (md5,index) table
			status=md5_cache_add(hash_key,count_md5_cache);
			if(status)
			goto end;
			
			count_md5_cache++;
			
		}	


		else//if block is present, create a new entry in (block:inode,index) table
		{
			//Adding a new entry into (inode:blocknumber,index) table
			status=binode_cache_add(key,temp_md5->index);
			if(status)
			goto end;
			count_block_inode++;

		}

		
	}



end:
	if (res == -1)
		res = -errno;

	close(fd);
	return res;
}

static int xmp_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
	int fd;
	int res;

	(void) fi;
	fd = open(path, O_WRONLY);
	if (fd == -1)
		return -errno;

	res = pwrite(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	close(fd);
	return res;
}

static int xmp_statfs(const char *path, struct statvfs *stbuf)
{
	int res;

	res = statvfs(path, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_release(const char *path, struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */

	(void) path;
	(void) fi;
	return 0;
}

static int xmp_fsync(const char *path, int isdatasync,
		     struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */

	(void) path;
	(void) isdatasync;
	(void) fi;
	return 0;
}

#ifdef HAVE_SETXATTR
/* xattr operations are optional and can safely be left unimplemented */
static int xmp_setxattr(const char *path, const char *name, const char *value,
			size_t size, int flags)
{
	int res = lsetxattr(path, name, value, size, flags);
	if (res == -1)
		return -errno;
	return 0;
}

static int xmp_getxattr(const char *path, const char *name, char *value,
			size_t size)
{
	int res = lgetxattr(path, name, value, size);
	if (res == -1)
		return -errno;
	return res;
}

static int xmp_listxattr(const char *path, char *list, size_t size)
{
	int res = llistxattr(path, list, size);
	if (res == -1)
		return -errno;
	return res;
}

static int xmp_removexattr(const char *path, const char *name)
{
	int res = lremovexattr(path, name);
	if (res == -1)
		return -errno;
	return 0;
}
#endif /* HAVE_SETXATTR */

static struct fuse_operations xmp_oper = {
	.getattr	= xmp_getattr,
	.access		= xmp_access,
	.readlink	= xmp_readlink,
	.readdir	= xmp_readdir,
	.mknod		= xmp_mknod,
	.mkdir		= xmp_mkdir,
	.symlink	= xmp_symlink,
	.unlink		= xmp_unlink,
	.rmdir		= xmp_rmdir,
	.rename		= xmp_rename,
	.link		= xmp_link,
	.chmod		= xmp_chmod,
	.chown		= xmp_chown,
	.truncate	= xmp_truncate,
	.utimens	= xmp_utimens,
	.open		= xmp_open,
	.read		= xmp_read,
	.write		= xmp_write,
	.statfs		= xmp_statfs,
	.release	= xmp_release,
	.fsync		= xmp_fsync,
#ifdef HAVE_SETXATTR
	.setxattr	= xmp_setxattr,
	.getxattr	= xmp_getxattr,
	.listxattr	= xmp_listxattr,
	.removexattr	= xmp_removexattr,
#endif
};

int main(int argc, char *argv[])
{
	umask(0);
	return fuse_main(argc, argv, &xmp_oper, NULL);
}
