//myport.c
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

#define SEGMENTSIZE sizeof(sem_t)
#define SEGMENTPERM 0666

int main(int argc, char **argv){
  FILE *configfile = NULL;
  char line[300], action[20], type[1];
  int value, id, err;

  //Create the shared memory segment
  id = shmget(IPC_PRIVATE, SEGMENTSIZE, SEGMENTPERM);
  if (id == -1)
  {
    perror("Could not create the shared memory segment");
    exit(2);
  }
  else
  {
    //Printing the id so that other processes can use it
    printf("Shared memory segment: %d\n", id);
  }

  //Checking the input
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

      }
      else if (strcmp(action, "cost") == 0)
      {

      }
    }

    fclose(configfile);
  }
  else
  {
    printf("Usage: -l configfile\n");
    exit(-1);
  }

  //After everything is done remove the resources
  err = shmctl(id, IPC_RMID, 0);
  if (err == -1)
  {
    perror("Could not remove shared memory segment");
    exit(2);
  }

  return 0;
}
