
#include <sys/resource.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <time.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

#include <stdint.h>
#define NUM_FILES 1024
#define NUM_FILES1 5

#include "benchmark.h"
/* Usage
  test1 path read_size_mb <logfile>
*/
int main(int argc, char *argv[])
{

  int fd;
  uint64_t toread=0;
  int i;
  char *path = argv[1];
  int read_size_mb = atoi(argv[2]);
  int num_of_blocks;
  char *buf;
  uint32_t filenum1;
  int filenum;
  int ret;
  char fullpath[1024];
  FILE *log_fd;
  char logfile[1024];

  buf= malloc(1024*1024);
  if (argc==3)
    sprintf(logfile,"//tmp//log.out.%d", getpid());
  
  if (argc>=4)
  {
    strcpy(logfile,argv[3]);
  }
  log_fd = fopen(logfile,"w");
  if (log_fd == NULL)
  {
    fprintf(stderr,"unable to open log file %d!\n", logfile);
    exit(1);
  }

  fprintf(log_fd, "Test2: Path = %s read_size_mb = %d\n",path, read_size_mb);
  fflush(log_fd);
  toread = read_size_mb;

  num_of_blocks = (1024*1204*1024)/4096;

 fprintf(log_fd, "Test2: Num of blocks to read = %llu blocks per file = %d\n",toread,num_of_blocks);

  
  srand( time(NULL));
  while (toread)
  {
    filenum = rand() % NUM_FILES;
    filenum1 = rand() % NUM_FILES1;
    memset(fullpath,0,1024);
    sprintf(fullpath, "%s%s%d-%d",path,"test",filenum1,filenum);
    fd = open(fullpath, O_RDONLY);
    if (fd <0)
    {
      fprintf(stderr,"unable to open file %s!\n", fullpath);
      exit(1);
    }
    start_clock_hires();
    ret = pread(fd, buf, 1048576, 0);
    if (ret <0)
    {
      fprintf(stderr, "Error while reading %s offset %0 size %\n", fullpath, 0, 1048576);
      exit(1);
    }
      fprintf(log_fd, "%s ",fullpath);
      end_clock_hires(log_fd);
      toread--;
    close(fd);
  }
  fclose(log_fd);
}
  

