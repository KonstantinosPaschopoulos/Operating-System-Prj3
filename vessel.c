//File: vessel.c
//The vessel executable. It can be used on its own or be called from another program

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
  int i, upgrade = 0, parkperiod, mantime, shmid, err;
  shm_management *shared_mem;
  char type[1];

  //Parsing the input
  for (i = 1; i < argc; i++)
  {
    if (strcmp(argv[i], "-t") == 0)
    {
      if ((strcmp(argv[i], "S") == 0) || (strcmp(argv[i], "M") == 0) || (strcmp(argv[i], "L") == 0))
      {
        strcpy(type, argv[i + 1]);
      }
      else
      {
        printf("type can only be S, M or L\n");
        exit(-1);
      }

      i++;
    }
    else if (strcmp(argv[i], "-u") == 0)
    {
      upgrade = 1;
    }
    else if (strcmp(argv[i], "-p") == 0)
    {
      parkperiod = atoi(argv[i + 1]);
      i++;
    }
    else if (strcmp(argv[i], "-m") == 0)
    {
      mantime = atoi(argv[i + 1]);
      i++;
    }
    else if (strcmp(argv[i], "-s") == 0)
    {
      shmid = atoi(argv[i + 1]);
      i++;
    }
    else
    {
      printf("Only use the following flags: -t, -u, -p, -m or -s\n");
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

  //The vessel stays in the open sea and is waiting in the FIFO queue
  sem_wait(&shared_mem->approaching);

  //The vessel can now wake up the port-master and ask him where to park
  sem_wait(&shared_mem->mutex);
  strcpy(shared_mem->waiting_type, type);
  shared_mem->waiting_upgrade = upgrade;
  shared_mem->vessel_action = 0;
  sem_post(&shared_mem->mutex);

  sem_post(&shared_mem->portmaster);

  //The post-master replied and the vessel can now move inside the port
  sem_wait(&shared_mem->port);




  sleep(mantime);

  //The vessel has parked and now noone is moving inside the port
  sem_post(&shared_mem->port);

  for (i = 0; i < parkperiod; i++)
  {
    sleep(0.5);

    //Asking for the current bill
  }

  //Asking the port-master to leave
  sem_wait(&shared_mem->mutex);
  shared_mem->vessel_action = 1;
  sem_post(&shared_mem->mutex);

  sem_post(&shared_mem->portmaster);

  //The vessel can now leave
  sem_wait(&shared_mem->port);
  sleep(mantime);
  sem_post(&shared_mem->port);

  //Detaching the shared memory before exiting
  err = shmdt((void *)shared_mem);
  if (err == -1)
  {
    perror("Could not detach shared memory");
    exit(-1);
  }

  return 0;
}
