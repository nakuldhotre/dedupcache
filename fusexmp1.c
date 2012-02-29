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
#include <sys/mman.h>
#include <time.h>
#include "uthash.h"
#include "fusexmp1.h"
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif
#include <libhashkit/hashkit.h>

static double mem_add, mem_find, binode_add, binode_find;
void libhashkit_md5_signature_wrap(const char *buf, int res, char *hash_key)
{
  libhashkit_md5_signature(buf, res, hash_key);
}

void *xmp_init(struct fuse_conn_info *conn)
{
  mem_add = 0;
  mem_find = 0;
  binode_add = 0;
  binode_find = 0;
  return FUSEXMP_DATA;
}

void xmp_destroy ()
{
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
fprintf(stderr,"\ncase1=%d\n",case1);
fprintf(stderr,"\ncase2=%d\n",case2);
fprintf(stderr,"\ncase3=%d\n",case3);
fprintf(stderr,"\ncase4=%d\n",case4);
}

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

int binode_cache_add(uint64_t key, unsigned char *hash_key)
{
HASH_TABLE_INODE_N_BLOCK *temp_binode=NULL,*temp;
clock_t start,end;
int ret = 0, i = 0;
start=clock();

temp_binode=(HASH_TABLE_INODE_N_BLOCK *)malloc(sizeof(HASH_TABLE_INODE_N_BLOCK));
mlock(temp_binode, sizeof(HASH_TABLE_INODE_N_BLOCK));


			if(temp_binode)
			{
				temp_binode->inode_block=key;
			        memcpy(temp_binode->hash_key, hash_key,16);
			        HASH_ADD(hh, block_inode,inode_block, 8,temp_binode);
				
				if(HASH_COUNT(block_inode)>=MAX_BINODE_COUNT)
				{
				          for (i=0;i<MAX_BINODE_COUNT/10; i++)
          				{
					  	HASH_ITER(hh,block_inode,temp_binode,temp)
					  	{
 						  HASH_DELETE(hh,block_inode,temp_binode);
						  free(temp_binode);
						  break;
    						}
          				}
					
			
				}
       
				ret =  SUCCESS;
        			goto exit;
			}
				
			else
			{
				fprintf(stderr,"Memory allocation failure\n");		
				ret = FAILURE;
			        goto exit;
			}

exit:

  end = clock();
  binode_add += ((double)(end - start))/CLOCKS_PER_SEC;
  return ret;

}


int binode_cache_find(uint64_t key,HASH_TABLE_INODE_N_BLOCK **temp_binode)
{
clock_t start,end;
int ret = 0;
start=clock();
HASH_FIND(hh, block_inode,&key, 8, (*temp_binode));

	if(*temp_binode) 
	{
		HASH_DELETE(hh,block_inode,(*temp_binode));		
		HASH_ADD(hh,block_inode,inode_block,8,(*temp_binode));
		ret = SUCCESS;	
	}
	else
  	ret = FAILURE;

  end = clock();
  binode_find += ((double)(end - start))/CLOCKS_PER_SEC;
  return ret; 
}

int memory_cache_add(unsigned char *hash_key, unsigned char *data,int size)
{
	HASH_TABLE_MEMORY_CACHE *temp_memory=NULL,*temp;
  unsigned char *data_block = NULL;
  clock_t start,end;
  int ret = 0,i;
  start=clock();
	temp_memory=(HASH_TABLE_MEMORY_CACHE* )malloc(sizeof(HASH_TABLE_MEMORY_CACHE ));	
  data_block = malloc (4096);
  mlock(temp_memory, sizeof(HASH_TABLE_MEMORY_CACHE));
  mlock(data_block, 4096);
			if(temp_memory)
			{
				memcpy(temp_memory->hash_key,hash_key,16);
        temp_memory->data_block = data_block;
				memcpy(temp_memory->data_block,data,size);
				HASH_ADD(hh,memory,hash_key,16,temp_memory);
				if(HASH_COUNT(memory)>=MAX_MEMORY_COUNT)
			  {
          for (i=0;i<MAX_MEMORY_COUNT/10;i++)
          {
            HASH_ITER(hh,memory,temp_memory,temp)
					  {
 						  HASH_DELETE(hh,memory,temp_memory);
              free(temp_memory->data_block);
						  free(temp_memory);
						  break;
					  }
          }            
        }
				ret =  SUCCESS;
        goto exit;
			}
			else
			{
				fprintf(stderr,"Memory allocation failure\n");		
				ret = FAILURE;
        goto exit;
			}
exit :
  end = clock();
    mem_add += ((double)(end - start))/CLOCKS_PER_SEC;
      return ret;
}

int memory_cache_find(char *hash_key, HASH_TABLE_MEMORY_CACHE **temp_memory)
{

 clock_t start,end;
   int ret = 0;
     start=clock();

  HASH_FIND(hh, memory,hash_key, 16, (*temp_memory));

	if((*temp_memory))
	{
		HASH_DELETE(hh,memory,(*temp_memory));		
		HASH_ADD(hh, memory,hash_key, 16, (*temp_memory));
		ret= SUCCESS;
    goto exit;
	}

	else
  {
	ret=FAILURE;
  goto exit;
  }
exit:  
  end = clock();
  mem_find += ((double)(end - start))/CLOCKS_PER_SEC;
  return ret;

}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
	int fd,status;
	int res,found_index;
	uint32_t block_num;
	unsigned char hash_key[16], *data_block;
	uint64_t key;
        struct stat stbuf;
	clock_t start,end;
#ifdef DEBUG_FUSEXMP
	FILE *fp=fopen("/home/ranjan/stats","r");
#endif
	STATS case_1,case_2,case_3,case_4;
#ifdef DEBUG_FUXEXMP
	fscanf(fp,"%d",&case_1.count);
	fscanf(fp,"%d",&case_2.count);
	fscanf(fp,"%d",&case_3.count);
	fscanf(fp,"%d",&case_4.count);
fclose(fp);	
#endif
	start=clock();
	HASH_TABLE_INODE_N_BLOCK *temp_binode1=NULL;
	HASH_TABLE_MEMORY_CACHE *temp_memory1=NULL;

#ifdef DEBUG_FUXEXMP
  fprintf(stderr,"Reading...\n");
  fprintf(stderr,"binode size = %d cache size = %d\n", HASH_COUNT(block_inode), HASH_COUNT(memory));
#endif

	(void) fi;
	fd = open(path, O_RDONLY);
	if (fd == -1)
		return -errno;
	
	lstat (path, &stbuf);
	block_num = offset / 4096;
	key=(stbuf.st_ino<<32)|block_num;

#ifdef DEBUG_FUXEXMP
  fprintf(stderr, "check if block %u %u is present in table\n",block_num,stbuf.st_ino);
#endif 
 
   //Check if block:inode is present in (block:inode,index) table]
  status=binode_cache_find(key,&temp_binode1);
#ifdef DEBUG_FUXEXMP
  fprintf(stderr, "status = %d\n", status);
#endif 
	
	if(status==SUCCESS) //If found, just copy the data from (md5,index) table to buffer 
	{
		status=memory_cache_find(temp_binode1->hash_key,&temp_memory1);
		if(status==SUCCESS)		
		{
			memcpy(buf,temp_memory1->data_block,size);
			res=size;
#ifdef DEBUG_FUXEXMP
			fprintf(stderr, "Found in cache_mem !\n");
#endif 
			case1++;
		}
		else 
    		{
#ifdef DEBUG_FUXEXMP
		  fprintf(stderr, "Not Found in cache_mem !\n");
		  fprintf(stderr, "Doing read fd = %d size = %lu offset = %lu\n", fd, size,offset);
#endif 
  		res = pread (fd, buf, size, offset);
		  if (res <4096)
	  	  goto end;
			//Adding a new entry into (hash,data blocks) table
			status=memory_cache_add(hash_key,buf,res);
			if(status)
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
#ifdef DEBUG_FUXEXMP
		fprintf(stderr, "Doing read fd = %d size = %lu offset = %lu\n", fd, size,offset);
#endif 
  		res = pread (fd, buf, size, offset);
		if (res <4096)
	  	goto end;
		//Use md5 as key to check if the read block is already in cache
		libhashkit_md5_signature_wrap(buf, res, hash_key);
		status=memory_cache_find(hash_key,&temp_memory1);
#ifdef DEBUG_FUXEXMP
    fprintf(stderr, "memory_cache_find status = %d\n", status);
#endif 

		if(status==FAILURE)//if block is not present
		{
			//Adding a new entry into (inode:blocknumber,hash) table
			status=binode_cache_add(key,hash_key);
			if(status)
			  goto end;
			count_block_inode++;

			//Adding a new entry into (hash,data blocks) table
			status=memory_cache_add(hash_key,buf,res);
			if(status)
			  goto end;
			case3++;
			
		}	

		else//if block is present, create a new entry in (block:inode,hash) table
		{
			//Adding a new entry into (inode:blocknumber,hash) table
			status=binode_cache_add(key,hash_key);
			if(status)
			  goto end;
			case4++;
		}
	}
end:
	if (res == -1)
		res = -errno;


	end=clock();
	close(fd);
#ifdef DEBUG_FUXEXMP
fp=fopen("/home/ranjan/stats","w");
	fprintf(fp,"%d\n",case_1.count);
	fprintf(fp,"%d\n",case_2.count);
	fprintf(fp,"%d\n",case_3.count);
	fprintf(fp,"%d\n",case_4.count);
fclose(fp);
#endif 
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
  .init     = xmp_init,
  .destroy  = xmp_destroy,
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
