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
#include "myfunctions.h"

int main(int argc, char **argv){
  FILE *configfile = NULL, *charges = NULL;
  void *shm;
  shm_management *shared_mem;
  char whole_line[100], action[100], type[10], str_id[300], vessel_name[300], parkingtime_str[300], mantime_str[300];
  int value, id, err, i, status, temp, total_spaces;
  pid_t port_master, vessel, monitor;

  create_logfile();

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
      perror("Could not open configfile");
      exit(-1);
    }
  }
  else
  {
    printf("Usage: -l configfile\n");
    exit(-1);
  }

  //Finding how many parking spaces we will have in total
  //in order to allocate the correct amount of space in the shared memory
  total_spaces = 0;
  while (fgets(whole_line, 100, configfile))
  {
    if ((strlen(whole_line) > 0) && (whole_line[strlen(whole_line) - 1] == '\n'))
    {
      whole_line[strlen(whole_line) - 1] = '\0';
    }

    sscanf(whole_line, "%s\t%s\t%d", action, type, &value);

    if (strcmp(action, "spaces") == 0)
    {
      total_spaces += value;
    }
  }
  rewind(configfile);

  //We need to allocate enough space for the main struct + the array of parking space structs
  id = shmget(IPC_PRIVATE, sizeof(shm_management) + (total_spaces * sizeof(parking_space)), SEGMENTPERM);
  if (id == -1)
  {
    perror("Could not create the shared memory segment");
    exit(2);
  }

  //Attach the shared memory
  shm = shmat(id, (void *) 0, 0);
  if (shm == (void *) - 1)
  {
    perror("Could not attach the shared memory");
    exit(-1);
  }
  shared_mem = (shm_management *) shm;
  shared_mem->parking_spaces = (parking_space *)(shm + sizeof(shm_management));

  //Initializing the shared memory
  shared_mem->vessel_is_waiting = 0;
  shared_mem->total_spaces = total_spaces;
  shared_mem->small_spaces = 0;
  shared_mem->medium_spaces = 0;
  shared_mem->big_spaces = 0;
  shared_mem->small_type = 0;
  shared_mem->medium_type = 0;
  shared_mem->big_type = 0;
  shared_mem->small_cost = 0;
  shared_mem->medium_cost = 0;
  shared_mem->big_cost = 0;
  shared_mem->small_profit = 0;
  shared_mem->medium_profit = 0;
  shared_mem->big_profit = 0;
  shared_mem->small_waiting = 0;
  shared_mem->medium_waiting = 0;
  shared_mem->big_waiting = 0;
  shared_mem->small_vessels_count = 0;
  shared_mem->medium_vessels_count = 0;
  shared_mem->big_vessels_count = 0;
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
  if (sem_init(&shared_mem->stuck_vessel, 1, 0) != 0)
  {
    perror("Could not initialize semaphore");
    exit(3);
  }

  total_spaces = 0;
  charges = fopen("chargesfile", "w");
  if (charges == NULL)
  {
    perror("Couldn't create charges file");
    exit(-1);
  }
  while (fgets(whole_line, 100, configfile))
  {
    if ((strlen(whole_line) > 0) && (whole_line[strlen(whole_line) - 1] == '\n'))
    {
      whole_line[strlen(whole_line) - 1] = '\0';
    }

    sscanf(whole_line, "%s\t%s\t%d", action, type, &value);

    if (strcmp(action, "spaces") == 0)
    {
      if (strcmp(type, "S") == 0)
      {
        shared_mem->small_type = 1;
        shared_mem->small_spaces = value;
        for (i = 0; i < value; i++)
        {
          strcpy(shared_mem->parking_spaces[i + total_spaces].type, type);
          shared_mem->parking_spaces[i + total_spaces].empty = 1;
          shared_mem->parking_spaces[i + total_spaces].parking_space_id = i + total_spaces;
        }
        total_spaces += value;
      }
      else if (strcmp(type, "M") == 0)
      {
        shared_mem->medium_type = 1;
        shared_mem->medium_spaces = value;
        for (i = 0; i < value; i++)
        {
          strcpy(shared_mem->parking_spaces[i + total_spaces].type, type);
          shared_mem->parking_spaces[i + total_spaces].empty = 1;
          shared_mem->parking_spaces[i + total_spaces].parking_space_id = i + total_spaces;
        }
        total_spaces += value;
      }
      else
      {
        shared_mem->big_type = 1;
        shared_mem->big_spaces = value;
        for (i = 0; i < value; i++)
        {
          strcpy(shared_mem->parking_spaces[i + total_spaces].type, type);
          shared_mem->parking_spaces[i + total_spaces].empty = 1;
          shared_mem->parking_spaces[i + total_spaces].parking_space_id = i + total_spaces;
        }
        total_spaces += value;
      }
    }
    else if (strcmp(action, "cost") == 0)
    {
      if (strcmp(type, "S") == 0)
      {
        fprintf(charges, "S\t%d\n", value);
        shared_mem->small_cost = value;
      }
      else if (strcmp(type, "M") == 0)
      {
        fprintf(charges, "M\t%d\n", value);
        shared_mem->medium_cost = value;
      }
      else
      {
        fprintf(charges, "L\t%d\n", value);
        shared_mem->big_cost = value;
      }
    }
  }
  fclose(charges);
  fclose(configfile);

  //After the shared memory is set up myport prints
  //the id so that other processes can use it
  printf("Shared memory segment: %d\n\n", id);
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
    execl("portmaster", "portmaster", "-s", str_id, "-c", "chargesfile", NULL);

    perror("Port-master failed to exec");
    exit(-1);
  }

  monitor = fork();
  if (monitor < 0)
  {
    perror("Monitor fork failed");
    exit(-1);
  }
  if (monitor == 0)
  {
    execl("monitor", "monitor", "-s", str_id, "-d", "4", "-t", "6", NULL);

    perror("Monitor failed to exec");
    exit(-1);
  }

  for (i = 0; i < 5; i++)
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
      srand(getpid());
      sprintf(vessel_name, "%d", (int)getpid());
      sprintf(parkingtime_str, "%d", (rand() % 3 + 1));
      sprintf(mantime_str, "%d", (rand() % 2 + 1));
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

      //With upgrade or not
      if (rand() % 2)
      {
        if (strcmp(type, "L") == 0)
        {
          execl("vessel", "vessel", "-s", str_id, "-m", mantime_str, "-p", parkingtime_str, "-t", type, NULL);
        }
        else
        {
          execl("vessel", "vessel", "-s", str_id, "-u", "L", "-m", mantime_str, "-p", parkingtime_str, "-t", type, NULL);
        }
      }
      else
      {
        execl("vessel", "vessel", "-s", str_id, "-m", mantime_str, "-p", parkingtime_str, "-t", type, NULL);
      }

      perror("Failed to exec new vessel");
      exit(-1);
    }
  }
  for (i = 0; i < 5; i++)
  {
    wait(&status);
  }

  //Wait for a signal from the user before exiting
  printf(YEL "Press enter to start closing down the port facilites\n" RESET);
  getchar();

  //Signal the other processes to stop executing
  sem_wait(&shared_mem->mutex);
  shared_mem->vessel_action = -1;
  sem_post(&shared_mem->portmaster);
  sem_wait(&shared_mem->answer);
  sem_post(&shared_mem->mutex);
  sleep(1);

  printf(YEL "Press enter again when eveything has finished\n" RESET);
  getchar();

  //After everything is done remove the resources
  if (sem_destroy(&shared_mem->approaching) != 0)
  {
    perror("Couldn't destroy a semaphore");
    exit(3);
  }
  if (sem_destroy(&shared_mem->portmaster) != 0)
  {
    perror("Couldn't destroy a semaphore");
    exit(3);
  }
  if (sem_destroy(&shared_mem->port) != 0)
  {
    perror("Couldn't destroy a semaphore");
    exit(3);
  }
  if (sem_destroy(&shared_mem->mutex) != 0)
  {
    perror("Couldn't destroy a semaphore");
    exit(3);
  }
  if (sem_destroy(&shared_mem->answer) != 0)
  {
    perror("Couldn't destroy a semaphore");
    exit(3);
  }
  if (sem_destroy(&shared_mem->stuck_vessel) != 0)
  {
    perror("Couldn't destroy a semaphore");
    exit(3);
  }
  err = shmctl(id, IPC_RMID, 0);
  if (err == -1)
  {
    perror("Could not remove shared memory segment");
    exit(2);
  }
  if (remove("chargesfile") == -1)
  {
    perror("Couldn't remove charges file");
    exit(2);
  }

  return 0;
}
