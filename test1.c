
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
#define NUM_FILES 10

#include "benchmark.h"
/* Usage
  test1 path read_size_mb <logfile>
*/
int main(int argc, char *argv[])
{

  int fd[NUM_FILES], toread[NUM_FILES];
  int i;
  char *path = argv[1];
  int read_size_mb = atoi(argv[2]);
  int num_of_blocks;
  char buf[4096];
  uint32_t blocknum;
  int filenum;
  int ret;
  char fullpath[1024];
  FILE *log_fd;
  char logfile[1024];

  
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

  fprintf(log_fd, "Test1: Path = %s read_size_mb = %d\n",path, read_size_mb);
  fflush(log_fd);
  for (i=0;i<NUM_FILES;i++)
  {
    sprintf(fullpath,"%s%s%d",path,"test",i);
    fd[i] = open(fullpath, O_RDONLY);
    if (fd[i] < 0)
    {
      fprintf(stderr, " Unable to open file %s %d\n",fullpath, i);
      exit(1);
    }
    memset(fullpath,0,1024);
    toread[i] = (read_size_mb*1204*1024)/4096;
  }

  num_of_blocks = (1024*1204*1024)/4096;

  srand( time(NULL));
  while (1)
  {
    filenum = rand() % NUM_FILES;
    if (toread[filenum] != 0)
    {
      blocknum = rand()% num_of_blocks;
      start_clock_hires();
      ret = pread(fd[filenum], buf, 4096, blocknum*4096);
      if (ret <0)
      {
        fprintf(stderr, "Error while reading %d offset %d size %d\n", filenum, blocknum*4096,4096);
        exit(1);
      }
      end_clock_hires(log_fd);
      toread[filenum]--;
      for(i=0;i<NUM_FILES; i++)
      {
        if(toread[i]!=0)
          break;
      }
      if (i == NUM_FILES)
      {
        break;
      }
    }
  }
  fclose(log_fd);
  for (i=0;i<NUM_FILES;i++)
    close(fd[i]);
}
  

