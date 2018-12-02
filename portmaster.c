//File: portmaster.c
//The port master acts as the guard of the port.
//Every vessel has to get a permission from him to enter
//and exit the port


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include "mytypes.h"

int main(int argc, char **argv){
  int i, shmid;
  shm_management *shared_mem;

  //Parsing the input
  for (i = 1; i < argc; i++)
  {
    if (strcmp(argv[i], "-s") == 0)
    {
      shmid = atoi(argv[i + 1]);
      i++;
    }
    else
    {
      printf("Only use the following flag: -s\n");
      exit(-1);
    }
  }

  //Attaching the shared memory
  shared_mem = (shm_management *)shmat(shmid, (void *) 0, 0);
  if (shared_mem == (void *) - 1)
  {
    perror("Could not attach the shared memory");
    exit(-1);
  }

  //The port-master is constantly working
  while (1)
  {
    sem_wait(&shared_mem->portmaster);


    sem_post(&shared_mem->approaching);
  }



  return 0;
}
