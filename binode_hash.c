
#include <sys/resource.h>
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


int main(int argc, char *argv[])
{
  int fd = -1;
  int res;
  char *filename;
  struct stat stbuf;
  int offset=0;
	uint64_t block_num;
  uint64_t hash_key;
  char buf[4096];
  uint64_t hash_temp,key;

  filename = argv[1];
  
  lstat64(filename,&stbuf);

  if (stbuf.st_size <4096)
     exit(0);
  fd = open(filename,O_RDONLY);

  if (fd == -1)
     exit(1);
  for (offset=0;offset <stbuf.st_size; offset=offset+4096)
  {

     block_num = offset/4096;
     key = stbuf.st_ino;
     key = key <<32;
     key = key + block_num;

     key = libhashkit_murmur((unsigned char *)&key, 8);

     res = pread(fd, buf, 4096, offset);
     if (res == 4096)
     {
        hash_temp = libhashkit_murmur(buf,res);
        printf("%llu %llu\n", key, hash_temp);
     }
     else
       break;
   }
  close(fd);
  return 0;
}
