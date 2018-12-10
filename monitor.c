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
  parking_space *public_ledger = (parking_space *)(shm + sizeof(shm_management));

  while (1)
  {
    if (shared_mem->vessel_action == -1)
    {
      break;
    }

    if (porttime > stattime)
    {
      sleep(stattime);

      printf("\nPort Statistics\n\n");

      sleep(porttime - stattime);

      printf("\nPort Status\n\n");
    }
    else
    {
      sleep(porttime);

      printf("\nPort Statistics\n\n");

      sleep(stattime - porttime);

      printf("\nPort Status\n\n");
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
