//File: myport.c
//The job of this executable is to read the configfile,
//create all the resources the port needs to be able to operate
//and purge everything before exiting

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <time.h>
#include "mytypes.h"

int main(int argc, char **argv){
  FILE *configfile = NULL;
  shm_management *shared_mem;
  char line[300], action[20], type[1], str_id[300], vessel_name[300], parkingtime_str[300], mantime_str[300];
  int value, id, err, i, status, temp;
  pid_t port_master, vessel;

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

  //Initializing the shared memory
  shared_mem->small_type = 0;
  shared_mem->medium_type = 0;
  shared_mem->big_type = 0;
  shared_mem->profit = 0;
  shared_mem->avg_profit = 0;
  shared_mem->small_waiting = 0;
  shared_mem->medium_waiting = 0;
  shared_mem->big_waiting = 0;
  shared_mem->closing_time = 0;
  if (sem_init(&shared_mem->approaching, 1, 1) != 0)
  {
    perror("Could not initialize semaphore");
    exit(3);
  }
  if (sem_init(&shared_mem->portmaster, 1, 0) != 0)
  {
    perror("Could not initialize semaphore");
    exit(3);
  }
  if (sem_init(&shared_mem->mutex, 1, 1) != 0)
  {
    perror("Could not initialize semaphore");
    exit(3);
  }
  if (sem_init(&shared_mem->port, 1, 1) != 0)
  {
    perror("Could not initialize semaphore");
    exit(3);
  }
  if (sem_init(&shared_mem->answer, 1, 0) != 0)
  {
    perror("Could not initialize semaphore");
    exit(3);
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
        shared_mem->small_type = 1;
        shared_mem->small_spaces = value;
        // if (sem_init(&shared_mem->small_spaces, 1, value) != 0)
        // {
        //   perror("Could not initialize semaphore");
        //   exit(3);
        // }
      }
      else if (strcmp(type, "M") == 0)
      {
        shared_mem->medium_type = 1;
        shared_mem->medium_spaces = value;
        // if (sem_init(&shared_mem->medium_spaces, 1, value) != 0)
        // {
        //   perror("Could not initialize semaphore");
        //   exit(3);
        // }
      }
      else
      {
        shared_mem->big_type = 1;
        shared_mem->big_spaces = value;
        // if (sem_init(&shared_mem->big_spaces, 1, value) != 0)
        // {
        //   perror("Could not initialize semaphore");
        //   exit(3);
        // }
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
  sprintf(str_id, "%d", id);

  port_master = fork();
  if (port_master < 0)
  {
    perror("Port-master fork failed");
    exit(-1);
  }
  if (port_master == 0)
  {
    //Calling the portmaster process to keep track of the port
    execl("portmaster", "portmaster", "-s", str_id, NULL);

    perror("Port-master failed to exec");
    exit(-1);
  }

  for (i = 0; i < 10; i++)
  {
    vessel = fork();
    if (vessel < 0)
    {
      perror("Vessel Fork failed");
      exit(-1);
    }
    if (vessel == 0)
    {
      //Creating new vessels
      sprintf(vessel_name, "%d", getpid());
      sprintf(parkingtime_str, "%d", (rand() % 3 + 1));
      sprintf(mantime_str, "%d", (rand() % 6 + 1));
      temp = rand() % 3;
      if (temp == 1)
      {
        strcpy(type, "S");
      }
      else if (temp == 2)
      {
        strcpy(type, "M");
      }
      else
      {
        strcpy(type, "L");
      }
      printf("%s\n", type);

      //With upgrade or not
      if (rand() % 2)
      {
        execl("vessel", "vessel", "-s", str_id, "-u", "-m", mantime_str, "-p", parkingtime_str, "-t", type, NULL);
      }
      else
      {
        execl("vessel", "vessel", "-s", str_id, "-m", mantime_str, "-p", parkingtime_str, "-t", type, NULL);
      }

      perror("Failed to exec new vessel");
      exit(-1);
    }
  }
  for (i = 0; i < 10; i++)
  {
    wait(&status);
  }

  //Wait for a signal from the user before exiting
  getchar();

  //Signal the other processes to stop working
  shared_mem->closing_time = 1;

  //After everything is done remove the resources
  err = shmctl(id, IPC_RMID, 0);
  if (err == -1)
  {
    perror("Could not remove shared memory segment");
    exit(2);
  }

  return 0;
}
