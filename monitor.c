#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include "mytypes.h"

void print_statistics(void *shm){
  int profit, count, waiting;
  shm_management *shared_mem;

  shared_mem = (shm_management *)shm;

  printf("\nPort Statistics\n\n");

  profit = shared_mem->small_profit + shared_mem->medium_profit + shared_mem->big_profit;
  count = shared_mem->small_vessels_count + shared_mem->medium_vessels_count + shared_mem->big_vessels_count;

  printf("Total profit: %d\n", profit);
  if (count == 0)
  {
    return;
  }
  printf("Average profit: %f\n", (profit / count));
  if (shared_mem->small_vessels_count != 0)
  {
    printf("Average profit of small vessels: %f\n", (shared_mem->small_profit / shared_mem->small_vessels_count));
  }
  if (shared_mem->medium_vessels_count != 0)
  {
    printf("Average profit of medium vessels: %f\n", (shared_mem->medium_profit / shared_mem->medium_vessels_count));
  }
  if (shared_mem->big_vessels_count != 0)
  {
    printf("Average profit of large vessels: %f\n", (shared_mem->big_profit / shared_mem->big_vessels_count));
  }

  waiting = shared_mem->small_waiting + shared_mem->medium_waiting + shared_mem->big_waiting;
  printf("Total waiting time: %d\n", waiting);
  printf("Average waiting time: %f\n", (waiting / count));
  if (shared_mem->small_vessels_count != 0)
  {
    printf("Average waiting time of small vessels: %f\n", (shared_mem->small_waiting / shared_mem->small_vessels_count));
  }
  if (shared_mem->medium_vessels_count != 0)
  {
    printf("Average waiting time of medium vessels: %f\n", (shared_mem->medium_waiting / shared_mem->medium_vessels_count));
  }
  if (shared_mem->big_vessels_count != 0)
  {
    printf("Average waiting time of medium vessels: %f\n", (shared_mem->big_waiting / shared_mem->big_vessels_count));
  }
}

void print_port(void *shm){
  int i;
  shm_management *shared_mem;

  printf("\nPort Status\n\n");

  shared_mem = (shm_management *)shm;
  parking_space *public_ledger = (parking_space *)(shm + sizeof(shm_management));

  for (i = 0; i < shared_mem->total_spaces; i++)
  {
    //Printing which vessels are currently in the port
    if (public_ledger[i].empty == 0)
    {
      printf("Parking Space ID: %d\n", public_ledger[i].parking_space_id);
      printf("Type: %s\n", public_ledger[i].type);
      printf("Vessel ID: %d\n", public_ledger[i].vessel_id);
      printf("Time of Arrival: %ld\n\n", public_ledger[i].arrival);
    }
  }
}

int main(int argc, char **argv){
  int i, porttime, stattime, shmid, err;
  shm_management *shared_mem;
  void *shm;

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

  while (1)
  {
    //Works until a "signal" to stop arrives
    if (shared_mem->vessel_action == -1)
    {
      break;
    }

    //Print the results with the user specified period
    if (porttime > stattime)
    {
      sleep(stattime);
      print_statistics(shm);

      sleep(porttime - stattime);
      print_port(shm);
    }
    else
    {
      sleep(porttime);
      print_port(shm);

      sleep(stattime - porttime);
      print_statistics(shm);
    }
  }

  //Detaching the shared memory before exiting
  err = shmdt((void *)shm);
  if (err == -1)
  {
    perror("Could not detach shared memory");
    exit(-1);
  }

  printf("Monitor has finished\n");

  return 0;
}
