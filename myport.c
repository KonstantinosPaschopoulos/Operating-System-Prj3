//File: myport.c
//The job of this executable is to read the configfile,
//create all the resources the port needs to be able to operate
//and purge everything before exiting

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
  FILE *configfile = NULL;
  shm_management *shared_mem;
  char line[300], action[20], type[1];
  int value, id, err;

  //Parsing the input
  if (argc != 3)
  {
    printf("Usage: -l configfile\n");
    exit(-1);
  }
  if (strcmp(argv[1], "-l") == 0)
  {
    //Open the configfile for reading
    configfile = fopen(argv[2], "r");
    if (configfile == NULL)
    {
      printf("Could not open configfile\n");
      exit(-1);
    }
  }
  else
  {
    printf("Usage: -l configfile\n");
    exit(-1);
  }

  //Create the shared memory segment
  id = shmget(IPC_PRIVATE, SEGMENTSIZE, SEGMENTPERM);
  if (id == -1)
  {
    perror("Could not create the shared memory segment");
    exit(2);
  }

  //Attach the shared memory
  shared_mem = (shm_management *)shmat(id, (void *) 0, 0);
  if (shared_mem == (void *) - 1)
  {
    perror("Could not attach the shared memory");
    exit(-1);
  }

  //Loop through the configfile to set the correct parameters
  while (fgets(line, 300, configfile))
  {
    if ((strlen(line) > 0) && (line[strlen(line) - 1] == '\n'))
    {
      line[strlen(line) - 1] = '\0';
    }

    sscanf(line, "%s\t%s\t%d", action, type, &value);

    if (strcmp(action, "spaces") == 0)
    {
      if (strcmp(type, "S") == 0)
      {
        if (sem_init(&shared_mem->small_spaces, 1, value) != 0)
        {
          perror("Could not initialize semaphore");
          exit(3);
        }
      }
      else if (strcmp(type, "M") == 0)
      {
        if (sem_init(&shared_mem->medium_spaces, 1, value) != 0)
        {
          perror("Could not initialize semaphore");
          exit(3);
        }
      }
      else
      {
        if (sem_init(&shared_mem->big_spaces, 1, value) != 0)
        {
          perror("Could not initialize semaphore");
          exit(3);
        }
      }
    }
    else if (strcmp(action, "cost") == 0)
    {
      if (strcmp(type, "S") == 0)
      {
        shared_mem->small_cost = value;
      }
      else if (strcmp(type, "M") == 0)
      {
        shared_mem->medium_cost = value;
      }
      else
      {
        shared_mem->big_cost = value;
      }
    }
  }

  fclose(configfile);

  //After the shared memory is set up
  //myport prints the id so that other processes can use it
  printf("Shared memory segment: %d\n", id);



  //After everything is done remove the resources
  err = shmctl(id, IPC_RMID, 0);
  if (err == -1)
  {
    perror("Could not remove shared memory segment");
    exit(2);
  }


  /*

for(i = 0; i < atoi(argv[1]); i++) {
    pid = fork();
    if(pid < 0) {
        printf("Error");
        exit(1);
    } else if (pid == 0) {
        printf("Child (%d): %d\n", i + 1, getpid());
        exit(0);
    } else  {
        wait(NULL);
    }
}
*/

  return 0;
}
