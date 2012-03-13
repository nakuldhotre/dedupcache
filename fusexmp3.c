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

#include <sys/resource.h>
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <time.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif
#include <libhashkit/hashkit.h>
#include "nhash.h"
#include "fusexmp1.h"

uint64_t
value (void *key)
{
  return *((uint64_t *) key);
}

unsigned int
keysize (void *key)
{
  return sizeof (uint64_t);
}

unsigned int
datasize (void *data)
{
  return sizeof (uint64_t);
}


static nhash_table *binode_table,*memory_table,*binode_hash_table;

static double mem_add, mem_find, binode_add, binode_find;

static LINKED *binode_list=NULL,*memory_list=NULL;
void
libhashkit_md5_signature_wrap (const char *buf, int res, unsigned char *hash_key)
{
  libhashkit_md5_signature (buf, res, hash_key);
}

void *
xmp_init (struct fuse_conn_info *conn)
{
  struct rlimit rlim;
  int ret;
  ret = getrlimit(RLIMIT_MEMLOCK , &rlim);
  if (ret == 0)
  {
    fprintf(stderr, "memlock limit = soft %lu hard %lu\n",
            rlim.rlim_cur, rlim.rlim_max);
  }
  else
  {
    fprintf(stderr, "getrlimit %s", strerror(ret));
  }
  rlim.rlim_cur = 524288000;
  rlim.rlim_max = 624288000;
  ret = setrlimit(RLIMIT_MEMLOCK, &rlim);
  if (ret == 0)
  {
    fprintf(stderr, "memlock limit = soft %lu hard %lu\n",
            rlim.rlim_cur, rlim.rlim_max);
    ret = mlockall(MCL_CURRENT|MCL_FUTURE);
    if (ret == 0)
    {
      fprintf(stderr, "Made all memory pinned!\n");
    }
  }
  else
  {
    fprintf(stderr, "Unable to set rlimit. Need root access\n");
  }

  mem_add = 0;
  mem_find = 0;
  binode_add = 0;
  binode_find = 0;
  binode_table = malloc(sizeof(nhash_table));
  nhash_init_table (binode_table, 100000);

  memory_table = malloc(sizeof(nhash_table));
  nhash_init_table (memory_table, 10000);

  binode_hash_table = malloc(sizeof(nhash_table));
  nhash_init_table (binode_hash_table, 100000);

  binode_populate_table();
  return FUSEXMP_DATA;
}

void
xmp_destroy ()
{

  float binode_percent,memory_percent;
/*HASH_TABLE_MEMORY_CACHE *temp2;
HASH_TABLE_INODE_N_BLOCK *temp1;
fprintf(stderr,"Calling destroy..\n");
 fprintf(stderr, " mem_add = %lf\n mem_find = %lf\nbinode_add = %lf\ninode_find = %lf\n",
          mem_add,mem_find,binode_add,binode_find);

  while(block_inode)
         {
        temp1=block_inode->hh.next;
        free(block_inode);
        block_inode=temp1;
        }

  while(memory)
 {
        temp2=memory->hh.next;
        free(memory);
        memory=temp2;
 }

 */
  munlockall();
  fprintf (stderr, "\ncase1=%d\n", case1);
  fprintf (stderr, "\ncase2=%d\n", case2);
  fprintf (stderr, "\ncase3=%d\n", case3);
  fprintf (stderr, "\ncase4=%d\n", case4);
  fprintf (stderr, "\ncase5=%d\n", case5);
  fprintf (stderr, "\ncase6=%d\n", case6);

  binode_percent = ((float)(binode_cache_total-binode_cache_miss)/(float)binode_cache_total)*100.0;

  memory_percent = ((float)(memory_cache_total-memory_cache_miss)/(float)memory_cache_total)*100.0;

  fprintf (stderr, "\nbinode_cache  hit\/miss\/total %d\/%d\/%d \n Hit ratio %.2f\n Cache utilization %d\/%d\n", (binode_cache_total-binode_cache_miss), 
          binode_cache_miss, binode_cache_total, binode_percent,count_block_inode, MAX_BINODE_COUNT);

  fprintf (stderr, "\nmemory_cache  hit\/miss\/total %d\/%d\/%d\n Hit ratio %.2f\n Cache utilization %d\/%d\n", (memory_cache_total-memory_cache_miss), 
          memory_cache_miss, memory_cache_total, memory_percent, count_memory_cache, MAX_MEMORY_COUNT);

}

static int
xmp_getattr (const char *path, struct stat *stbuf)
{
  int res;

  res = lstat64 (path, stbuf);
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

int
binode_cache_add (uint64_t key, uint64_t hash_temp)
{
  int ret = 0;
  LINKED *temp;
  n_key_val kv;

  if(count_block_inode>MAX_BINODE_COUNT)
  {
    temp=binode_list->prev;
    nhash_delete_key(binode_table, binode_list->prev->kv.key);
    binode_list->prev=temp->prev;
    temp->prev->next=binode_list;
    free(temp);
//    fprintf(stderr, "flushing memory cache\n");

    binode_eviction_done = 1;
    count_block_inode--;
  }
  kv.key=key;
  kv.val = hash_temp;
  if(binode_list==NULL)
  {
    binode_list=malloc(sizeof(LINKED));
    binode_list->prev=binode_list;
    binode_list->next=binode_list;
    temp=binode_list;
  }
  else
  {
    temp=malloc(sizeof(LINKED));
    temp->prev=binode_list->prev;
    temp->next=binode_list;
    binode_list->prev=temp;
    binode_list=temp;
  }
  count_block_inode++;
  kv.pt=temp;
  temp->kv=kv;


#ifdef DEBUG_FUXEXMP
  fprintf(stderr, "binode_cache_add: k %llx v %llx hash %lx\n",kv.key,kv.val,hash_temp);
#endif
  nhash_insert_key(binode_table, &kv);
  return ret;

}


int
binode_cache_find (uint64_t key, n_key_val *temp_binode)
{
  int ret = 0;
  n_key_val *kv;
  LINKED *temp;

  binode_cache_total++;
#ifdef DEBUG_FUXEXMP
  fprintf(stderr, "binode_cache_find: key = %llx\n", key);
#ifdef DEBUG
  print_subtree(block_inode_tree, block_inode_tree->root);
#endif
#endif
  kv = nhash_search_key(binode_table,key);

  if (kv)
  {
    *temp_binode = *kv;
    ret = SUCCESS;
    temp=kv->pt;

    if(temp!=binode_list)
    {
      temp->prev->next=temp->next;
      temp->next->prev=temp->prev;
      temp->prev=binode_list->prev;
      temp->next=binode_list;
      binode_list->prev=temp;
      binode_list=temp;
    }
  }

  else
  {
    if (binode_eviction_done)  /* We dont assume cache miss if eviction never happened */
      binode_cache_miss++;
    ret = FAILURE;
  }

  return ret;
}

int
memory_cache_add (uint64_t hash_temp, unsigned char *data, int size)
{
  unsigned char *data_block = NULL;
  int ret = 0;
  LINKED *temp;

  n_key_val kv;
  if(count_memory_cache>MAX_MEMORY_COUNT)
  {
    temp=memory_list->prev;
    //while(temp->next)
    // temp=temp->next;

    nhash_delete_key(memory_table,memory_list->prev->kv.key);
    free(memory_list->prev->kv.val);
    memory_list->prev=temp->prev; 
    temp->prev->next=memory_list; 
    free(temp);
    //  fprintf(stderr, "flushing memory cache\n");
    memory_eviction_done = 1;
    count_memory_cache--;
  }

  kv.key = hash_temp;
  kv.val = 0;

  data_block = malloc (4096);
  kv.val = (uint64_t)data_block;
  memcpy (data_block, data, size);

  if(memory_list==NULL)
  {
    memory_list=malloc(sizeof(LINKED));
    memory_list->prev=memory_list;
    memory_list->next=memory_list;
    temp=memory_list;
  }
  else
  {
    temp=malloc(sizeof(LINKED));
    temp->prev=memory_list->prev;
    temp->next=memory_list;
    memory_list->prev=temp;
    memory_list=temp;
  }
  count_memory_cache++;
  kv.pt=temp;
  temp->kv=kv;

  nhash_insert_key(memory_table,&kv);
  return ret;
}

int
memory_cache_find (uint64_t hash_key, n_key_val  *temp_memory)
{

  int ret = 0;
  n_key_val *kv;
  LINKED *temp;

  memory_cache_total++;
  kv = nhash_search_key(memory_table,hash_key);

  if (kv)
  {
    *temp_memory = *kv;
    ret = SUCCESS;
    temp=kv->pt;


    if(temp!=memory_list) 
    {
      temp->prev->next=temp->next;
      temp->next->prev=temp->prev;
      temp->prev=memory_list->prev;
      temp->next=memory_list;
      memory_list->prev=temp;
      memory_list=temp;
    } 


  }
  else
  {
    if (memory_eviction_done)
      memory_cache_miss++;
    ret = FAILURE;
  }

  return ret;

}

int binode_populate_table()
{
  FILE *fp;
  int res;
  uint64_t binode_key,data_hash;
  n_key_val binode_kv;
  fp = fopen("/tmp/binode_hash", "r");

  if (fp == NULL)
	return 1;
  while (!feof(fp))
  {
    res = fscanf(fp, "%llu %llu", &binode_key, &data_hash);
    if (res == 2)
    {
      binode_kv.key = binode_key;
      binode_kv.val = data_hash;
      nhash_insert_key(binode_hash_table, &binode_kv);
//      fprintf(stderr, "key = %llu hash = %llu\n", binode_key, data_hash);
    }
  }
  fclose(fp);
}


static int
xmp_read (const char *path, char *buf, size_t size, off_t offset,
          struct fuse_file_info *fi)
{
  int fd, status;
  int res;
  uint32_t block_num;
  uint64_t key,hash_temp,key1;
  struct stat stbuf;

  //STATS case_1, case_2, case_3, case_4;

  n_key_val temp_binode1;
  n_key_val temp_memory1,*binode_hash_key,temp_memory;

#ifdef DEBUG_FUXEXMP
  fprintf (stderr, "Reading...\n");
  fprintf (stderr, "binode size = %d cache size = %d\n",
           HASH_COUNT (block_inode), HASH_COUNT (memory));
#endif

  (void) fi;
  fd = open (path, O_RDONLY);
  if (fd == -1)
    return -errno;

  lstat64 (path, &stbuf);
  block_num = offset / 4096;
  key = stbuf.st_ino;
  key = key<<32;
  key = key + block_num;

  key1 = libhashkit_murmur((unsigned char *)&key, 8);
  key = key1;

  //Check if block:inode is present in (block:inode,index) table]
  status = binode_cache_find (key, &temp_binode1);
#ifdef DEBUG_FUXEXMP
  fprintf (stderr, "status = %d\n", status);
#endif

  if (status == SUCCESS)        //If found, just copy the data from (md5,index) table to buffer 
    {
      status = memory_cache_find (temp_binode1.val, &temp_memory1);
      if (status == SUCCESS)
        {
          memcpy (buf, (void *)temp_memory1.val, size);
          res = size;
#ifdef DEBUG_FUXEXMP
          fprintf (stderr, "Found in cache_mem !\n");
#endif
          case1++;
        }
      else
        {
#ifdef DEBUG_FUXEXMP
          fprintf (stderr, "Not Found in cache_mem !\n");
          fprintf (stderr, "Doing read fd = %d size = %lu offset = %lu\n", fd,
                   size, offset);
#endif
          res = pread (fd, buf, size, offset);
          if (res < 4096)
            goto end;
          //Adding a new entry into (hash,data blocks) table
          hash_temp = libhashkit_murmur(buf,res);
          status = memory_cache_add (hash_temp, buf, res);
          if (status)
            goto end;
          case2++;

        }
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

/* Request for hash of inode+block */
      binode_hash_key = nhash_search_key(binode_hash_table, key);
      if (binode_hash_key != NULL)
      {
     
        status = memory_cache_find(binode_hash_key->val, &temp_memory);

        if (status == FAILURE)
        {
          res = pread(fd,buf,size, offset);
          if (res <4096)
            goto end;
          status = binode_cache_add (binode_hash_key->key, binode_hash_key->val);
          if (status)
            goto end;

          //Adding a new entry into (hash,data blocks) table
          status = memory_cache_add (binode_hash_key->val, buf, res);
          if (status)
            goto end;
          case5++;
        }
        else
        {
          memcpy (buf, (void *)temp_memory.val, size);
          res = size;
          status = binode_cache_add(binode_hash_key->key, binode_hash_key->val);
          if(status)
            goto end;
          case6++;
        }
      }
      else
      {

#ifdef DEBUG_FUXEXMP
      fprintf (stderr, "Doing read fd = %d size = %lu offset = %lu\n", fd, size,
               offset);
#endif
      res = pread (fd, buf, size, offset);
      if (res < 4096)
        goto end;
      //Use md5 as key to check if the read block is already in cache
//      libhashkit_md5_signature_wrap (buf, res, hash_key);
      hash_temp = libhashkit_murmur(buf,res);
      status = memory_cache_find (hash_temp, &temp_memory1);
#ifdef DEBUG_FUXEXMP
      fprintf (stderr, "memory_cache_find status = %d\n", status);
#endif

      if (status == FAILURE)    //if block is not present
        {
          //Adding a new entry into (inode:blocknumber,hash) table
          status = binode_cache_add (key, hash_temp);
          if (status)
            goto end;

          //Adding a new entry into (hash,data blocks) table
          status = memory_cache_add (hash_temp, buf, res);
          if (status)
            goto end;
          case3++;

        }

      else                      //if block is present, create a new entry in (block:inode,hash) table
        {
          //Adding a new entry into (inode:blocknumber,hash) table
          status = binode_cache_add (key, hash_temp);
          if (status)
            goto end;
          case4++;
        }
      }
    }
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

  n_key_val *ptr,*binode_hash_key = NULL;
  n_key_val temp_binode, temp_memory;
  uint64_t key,key1,data_hash,block_num;
  struct stat stbuf;
  int ret;
  char temp_buf[4096];

  
  (void) fi;
  fd = open (path, O_WRONLY);
  if (fd == -1)
    return -errno;
  lstat64 (path, &stbuf);
  block_num = offset / 4096;
  key = stbuf.st_ino;
  key = key<<32;
  key = key + block_num;
  key1 = libhashkit_murmur((unsigned char *)&key, 8);
  key = key1;


  if (offset + size <4096 && stbuf.st_size <4096)
    goto end;

  if (offset%4096 == 0 && size == 4096)
  {
    if (binode_cache_find(key,&temp_binode) == 0)
    {
      data_hash = libhashkit_murmur(buf, 4096);
      ptr=binode_list->kv.pt;

      ptr->val=data_hash;
      if (memory_cache_find(data_hash, &temp_memory) == FAILURE)
      {
        ret = memory_cache_add(data_hash, buf, 4096);
        if (ret)
            goto end;
      }
      binode_hash_key = nhash_search_key(binode_hash_table, key);
      if (binode_hash_key !=NULL)
        binode_hash_key->val = data_hash; 
    }
  }
  else if (offset%4096 == 0 && size <4096)
  {
    if (binode_cache_find(key,&temp_binode) == 0)
    {
      
      if (memory_cache_find(temp_binode.val, &temp_memory) == SUCCESS)
      {
        memcpy(temp_buf,temp_memory.val,4096);
        memcpy(temp_buf, buf, size);

        data_hash = libhashkit_murmur(temp_buf, 4096);
        ret = memory_cache_add(data_hash, temp_buf, 4096);
        if (ret)
            goto end;

        ptr=binode_list->kv.pt;
        ptr->val=data_hash;
        binode_hash_key = nhash_search_key(binode_hash_table, key);
        if (binode_hash_key !=NULL)
          binode_hash_key->val = data_hash;

      }
    }
  }
  else if ((offset+size)/4096 == offset/4096)
  {
   if (binode_cache_find(key,&temp_binode) == 0)
   {
    if (memory_cache_find(temp_binode.val, &temp_memory) == SUCCESS)
    {
        memcpy(temp_buf,temp_memory.val,4096);
        memcpy(temp_buf+(offset%4096), buf, size);

        data_hash = libhashkit_murmur(temp_buf, 4096);
        ret = memory_cache_add(data_hash, temp_buf, 4096);
        if (ret)
            goto end;

        ptr=binode_list->kv.pt;
        ptr->val=data_hash;
        binode_hash_key = nhash_search_key(binode_hash_table, key);
        if (binode_hash_key !=NULL)
          binode_hash_key->val = data_hash;

      }

    }
  }
  else
  {

   if (binode_cache_find(key,&temp_binode) == 0)
   {
    if (memory_cache_find(temp_binode.val, &temp_memory) == SUCCESS)
    {
        memcpy(temp_buf,temp_memory.val,4096);
        memcpy(temp_buf+(offset%4096), buf, 4096-(offset%4096));

        data_hash = libhashkit_murmur(temp_buf, 4096);
        ret = memory_cache_add(data_hash, temp_buf, 4096);
        if (ret)
            goto end;

        ptr=binode_list->kv.pt;
        ptr->val=data_hash;
        binode_hash_key = nhash_search_key(binode_hash_table, key);
        if (binode_hash_key !=NULL)
          binode_hash_key->val = data_hash;


        block_num = offset / 4096;
        key = stbuf.st_ino;
        key = key<<32;
        key = key + block_num+1;
        key1 = libhashkit_murmur((unsigned char *)&key, 8);
        key = key1;
        if (binode_cache_find(key,&temp_binode) == 0)
        {
          if (memory_cache_find(temp_binode.val, &temp_memory) == SUCCESS)
          {
              memcpy(temp_buf,temp_memory.val,4096);
              memcpy(temp_buf, buf+ 4096-(offset%4096), size - (4096-(offset%4096)));

              data_hash = libhashkit_murmur(temp_buf, 4096);
              ret = memory_cache_add(data_hash, temp_buf, 4096);
              if (ret)
                goto end;
              ptr=binode_list->kv.pt;
              ptr->val=data_hash;
              binode_hash_key = nhash_search_key(binode_hash_table, key);
              if (binode_hash_key !=NULL)
                binode_hash_key->val = data_hash;
            }

      }
     }   

    }




  }



end:
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
  .init = xmp_init,
  .destroy = xmp_destroy,
#ifdef HAVE_SETXATTR
  .setxattr = xmp_setxattr,
  .getxattr = xmp_getxattr,
  .listxattr = xmp_listxattr,
  .removexattr = xmp_removexattr,
#endif
};

int
main (int argc, char *argv[])
{
  umask (0);
  return fuse_main (argc, argv, &xmp_oper, NULL);
}
