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
      if ((strcmp(argv[i + 1], "S") == 0) || (strcmp(argv[i + 1], "M") == 0) || (strcmp(argv[i + 1], "L") == 0))
      {
        strcpy(type, argv[i + 1]);
      }
      else
      {
        printf("Type can only be S, M or L\n");
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

  printf("Vessel %d is approaching the port\n", (int)getpid());

  //The vessel stays in the open sea and waits in the FIFO queue
  sem_wait(&shared_mem->approaching);

  while (1)
  {
    printf("Vessel %d can now talk with the port-master\n", getpid());

    //The vessel can now wake up the port-master and ask him where to park
    strcpy(shared_mem->waiting_type, type);
    shared_mem->vessel_id = (int)getpid();
    shared_mem->waiting_upgrade = upgrade;
    shared_mem->vessel_action = 0;
    sem_post(&shared_mem->portmaster);

    printf("Vessel %d is waiting for an answer from the port-master\n", getpid());

    //The port-master replied and the vessel can now move inside the port
    sem_wait(&shared_mem->answer);

    printf("Port-master answered vessel %d to %d\n", getpid(), shared_mem->portmaster_action);

    if (shared_mem->portmaster_action == 0)
    {
      //This vessel can't park in this port so it has to leave
      sem_post(&shared_mem->portmaster);
      return 0;
    }
    else if (shared_mem->portmaster_action == 1)
    {
      //There are no availiable parking spots at this moment
      sem_wait(&shared_mem->approaching);
    }
    else if (shared_mem->portmaster_action == 2)
    {
      //The port-master found a place for the vessel to park
      break;
    }
  }

  //Free the port-master
  sem_post(&shared_mem->portmaster);

  printf("Vessel %d moves to the assigned parking spot\n", getpid());

  sleep(mantime);

  //The vessel has parked and now no one is moving inside the port
  sem_post(&shared_mem->port);

  printf("Vessel %d will stay parked for %d seconds\n", getpid(), parkperiod);

  //The vessel stays parked in the port and at a random point asks for the current bill
  for (i = 0; i < parkperiod; i++)
  {
    sleep(1);

    if (i > (parkperiod / 2))
    {
      //Ask for the bill so far
    }
  }


  //Asking the port-master to leave
  shared_mem->vessel_action = 1;
  shared_mem->vessel_id = (int)getpid();
  sem_post(&shared_mem->portmaster);

  //Wait until the port-master says it's ok to leave
  sem_wait(&shared_mem->answer);

  //The portmaster can now help other vessels
  sem_post(&shared_mem->portmaster);

  sleep(mantime);

  //The vessel has left the port, so now noone is moving inside it
  sem_post(&shared_mem->port);

  //Detaching the shared memory before exiting
  err = shmdt((void *)shared_mem);
  if (err == -1)
  {
    perror("Could not detach shared memory");
    exit(-1);
  }

  printf("Vessel %d has left the port\n", getpid());

  return 0;
}
