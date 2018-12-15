//File: monitor.c
//It prints some statistics about the port and the status of the port
//with a user specified period.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <signal.h>
#include "myfunctions.h"
#include "mytypes.h"

int main(int argc, char **argv){
  int i, porttime, stattime, shmid, err;
  shm_management *shared_mem;
  void *shm;
  pid_t stat_child, port_child;

  //Parsing the input
  for (i = 1; i < argc; i++)
  {
    if (strcmp(argv[i], "-d") == 0)
    {
      porttime = atoi(argv[i + 1]);
      i++;
    }
    else if (strcmp(argv[i], "-t") == 0)
    {
      stattime = atoi(argv[i + 1]);
      i++;
    }
    else if (strcmp(argv[i], "-s") == 0)
    {
      shmid = atoi(argv[i + 1]);
      i++;
    }
    else
    {
      printf("Only use the following flags: -d, -t, or -s\n");
      exit(-1);
    }
  }

  //Attaching the shared memory
  shm = shmat(shmid, (void *) 0, 0);
  if (shm == (void *) - 1)
  {
    perror("Could not attach the shared memory");
    exit(-1);
  }
  shared_mem = (shm_management *)shm;

  stat_child = fork();
  if (stat_child < 0)
  {
    perror("Fork failed");
    exit(-1);
  }
  if (stat_child == 0)
  {
    while (1)
    {
      sleep(stattime);
      sem_wait(&shared_mem->mutex);
      print_statistics(shm);
      sem_post(&shared_mem->mutex);
    }

    exit(0);
  }

  port_child = fork();
  if (port_child < 0)
  {
    perror("Fork failed");
    exit(-1);
  }
  if (port_child == 0)
  {
    while (1)
    {
      sleep(porttime);
      sem_wait(&shared_mem->mutex);
      print_port(shm);
      sem_post(&shared_mem->mutex);
    }

    exit(0);
  }

  while (1)
  {
    //Works until a "signal", from the port-master, to stop arrives
    if (shared_mem->vessel_action == -1)
    {
      kill(stat_child, SIGKILL);
      kill(port_child, SIGKILL);
      break;
    }

    //Sleep for however long the longest period is and then wake up
    //to check if the monitor should close
    if (porttime > stattime)
    {
      sleep(porttime);
    }
    else
    {
      sleep(stattime);
    }
  }

  //Detaching the shared memory before exiting
  err = shmdt((void *)shm);
  if (err == -1)
  {
    perror("Could not detach shared memory");
    exit(-1);
  }

  printf(RED "Monitor has finished\n" RESET);

  return 0;
}
