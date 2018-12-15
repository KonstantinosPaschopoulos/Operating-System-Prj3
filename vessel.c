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
#include <sys/time.h>
#include "mytypes.h"
#include "myfunctions.h"

int main(int argc, char **argv){
  int i, j, upgrade = 0, parkperiod, mantime, shmid, err, cost;
  shm_management *shared_mem;
  void *shm;
  struct timeval time;
  char type[10], upgrade_type[10];
  strcpy(upgrade_type, "S");

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
      if ((strcmp(argv[i + 1], "M") == 0) || (strcmp(argv[i + 1], "L") == 0))
      {
        strcpy(upgrade_type, argv[i + 1]);
      }
      else
      {
        printf("The vessel can only upgrade to M or L\n");
        exit(-1);
      }
      upgrade = 1;
      i++;
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
  shm = shmat(shmid, (void *) 0, 0);
  if (shm == (void *) - 1)
  {
    perror("Could not attach the shared memory");
    exit(-1);
  }
  shared_mem = (shm_management *) shm;
  parking_space *public_ledger = (parking_space *)(shm + sizeof(shm_management));

  //The port is closing down, so it is pointless to ask where to park
  if (shared_mem->closing_time == 1)
  {
    //Detaching the shared memory before exiting
    err = shmdt((void *)shm);
    if (err == -1)
    {
      perror("Could not detach shared memory");
      exit(-1);
    }
    return 0;
  }

  printf("Vessel %d, of type %s, is approaching the port. Upgrade: %d\n", (int)getpid(), type, upgrade);

  gettimeofday(&time, NULL);

  //Every vessel gets stuck here until the port-master can assist it
  sem_wait(&shared_mem->approaching);

  while (1)
  {
    //Locks the shared memory to ask to park somewhere
    sem_wait(&shared_mem->mutex);

    //The vessel can now wake up the port-master and ask him where to park
    strcpy(shared_mem->waiting_type, type);
    shared_mem->vessel_id = (int)getpid();
    shared_mem->waiting_upgrade = upgrade;
    strcpy(shared_mem->waiting_upgrade_type, upgrade_type);
    shared_mem->vessel_action = 0;
    shared_mem->waiting_time = time.tv_sec;
    sem_post(&shared_mem->portmaster);

    //The port-master replied and the vessel can now move inside the port
    sem_wait(&shared_mem->answer);

    if (shared_mem->portmaster_action == 0)
    {
      //This vessel can't park in this port so it has to leave
      sem_post(&shared_mem->mutex);
      sem_post(&shared_mem->portmaster);
      //Detaching the shared memory before exiting
      err = shmdt((void *)shm);
      if (err == -1)
      {
        perror("Could not detach shared memory");
        exit(-1);
      }
      return 0;
    }
    else if (shared_mem->portmaster_action == 1)
    {
      //There are no availiable parking spots at this moment
      //so the vessel keeps asking until it gets a spot
      sem_post(&shared_mem->mutex);
      sem_post(&shared_mem->portmaster);

      //To avoid busy-waiting the vessel gets stuck here until
      //a parking space becomes free, then it asks the port-master again
      sem_wait(&shared_mem->stuck_vessel);
    }
    else if (shared_mem->portmaster_action == 2)
    {
      //The port-master found a place for the vessel to park
      break;
    }
  }

  sem_post(&shared_mem->mutex);

  //Free the port-master
  sem_post(&shared_mem->portmaster);

  sleep(mantime);

  //The vessel has parked and now no one is moving inside the port
  sem_post(&shared_mem->port);

  //The vessel stays parked in the port and at a random point asks for the current bill
  for (i = 0; i < parkperiod; i++)
  {
    sleep(1);

    //Ask for the bill so far
    if (i == 0)
    {
      sem_wait(&shared_mem->mutex);
      for (j = 0; j < shared_mem->total_spaces; j++)
      {
        if ((int)getpid() == public_ledger[j].vessel_id)
        {
          if (strcmp(public_ledger[j].type, "S") == 0)
          {
            cost = shared_mem->small_cost;
          }
          else if (strcmp(public_ledger[j].type, "M") == 0)
          {
            cost = shared_mem->medium_cost;
          }
          else if (strcmp(public_ledger[j].type, "L") == 0)
          {
            cost = shared_mem->big_cost;
          }
          gettimeofday(&time, NULL);
          printf("The current cost for the vessel %d is: %ld\n", (int)getpid(), (time.tv_sec - public_ledger[j].arrival) * cost);

          break;
        }
      }
      sem_post(&shared_mem->mutex);
    }
  }

  //Locks the shared memory to ask to leave
  sem_wait(&shared_mem->mutex);

  //Asking the port-master to leave
  shared_mem->vessel_action = 1;
  shared_mem->vessel_id = (int)getpid();
  sem_post(&shared_mem->portmaster);

  //Wait until the port-master says it's ok to leave
  sem_wait(&shared_mem->answer);

  sem_post(&shared_mem->mutex);

  //The portmaster can now help other vessels
  sem_post(&shared_mem->portmaster);

  sleep(mantime);

  //The vessel has left the port, so now noone is moving inside it
  sem_post(&shared_mem->port);

  //Detaching the shared memory before exiting
  err = shmdt((void *)shm);
  if (err == -1)
  {
    perror("Could not detach shared memory");
    exit(-1);
  }

  return 0;
}
