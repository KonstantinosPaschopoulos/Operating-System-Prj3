#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include "myfunctions.h"
#include "mytypes.h"

int main(int argc, char **argv){
  int i, porttime, stattime, shmid, err;
  shm_management *shared_mem;
  void *shm;

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
