#include <sys/times.h>
#include <stdio.h>

#include <time.h>


static struct timespec st_hires_time, en_hires_time;

void start_clock_hires ()
{
  clock_gettime(CLOCK_MONOTONIC, &st_hires_time);
}

struct timespec diff(struct timespec start, struct timespec end)
{
  struct timespec temp;
  if ((end.tv_nsec-start.tv_nsec)<0) {
    temp.tv_sec = end.tv_sec-start.tv_sec-1;
    temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
  } else {
    temp.tv_sec = end.tv_sec-start.tv_sec;
    temp.tv_nsec = end.tv_nsec-start.tv_nsec;
  }
  return temp;
}

void end_clock_hires(FILE *fd)
{
  struct timespec result;
  clock_gettime(CLOCK_MONOTONIC, &en_hires_time);
  result = diff(st_hires_time,en_hires_time);
  fprintf (fd, "%d %ld\n",result.tv_sec,
          result.tv_nsec);
}

