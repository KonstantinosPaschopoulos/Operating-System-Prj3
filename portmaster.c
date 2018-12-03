//File: portmaster.c
//The port master acts as the guard of the port.
//Every vessel has to get a permission from him to enter
//and exit the port


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
  int i, shmid;
  shm_management *shared_mem;

  //Parsing the input
  for (i = 1; i < argc; i++)
  {
    if (strcmp(argv[i], "-s") == 0)
    {
      shmid = atoi(argv[i + 1]);
      i++;
    }
    else
    {
      printf("Only use the following flag: -s\n");
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

  //The port-master is constantly working
  while (shared_mem->closing_time == 0)
  {
    printf("HERE\n");
    //The port-master waits until a vessel talks to him
    sem_wait(&shared_mem->portmaster);

    if (shared_mem->vessel_action == 0)
    {
      //The vessel asks where to park

      if (strcmp(shared_mem->waiting_type, "S") == 0)
      {
        //Checking if the port can support small vessels
        if (shared_mem->waiting_upgrade != 1)
        {
          if (shared_mem->small_type != 1)
          {
            shared_mem->portmaster_action = 0;
          }
        }
        else
        {
          if ((shared_mem->medium_type != 1) && (shared_mem->big_type != 1))
          {
            shared_mem->portmaster_action = 0;
          }
        }

        //Checking if there are availiable spots
        if (shared_mem->small_spaces == 0)
        {
          if (shared_mem->waiting_upgrade != 1)
          {
            shared_mem->portmaster_action = 1;
          }
          else
          {
            if ((shared_mem->medium_spaces == 0) && (shared_mem->big_spaces == 0))
            {
              shared_mem->portmaster_action = 1;
            }
          }
        }

        //Finding a parking spot for the vessel
        shared_mem->portmaster_action = 2;
      }

      if (strcmp(shared_mem->waiting_type, "M") == 0)
      {
        //Checking if the port can support medium vessels
        if (shared_mem->waiting_upgrade != 1)
        {
          if (shared_mem->medium_type != 1)
          {
            shared_mem->portmaster_action = 0;
          }
        }
        else
        {
          if (shared_mem->big_type != 1)
          {
            shared_mem->portmaster_action = 0;
          }
        }

        //Checking if there are availiable spots
        if (shared_mem->medium_spaces == 0)
        {
          if (shared_mem->waiting_upgrade != 1)
          {
            shared_mem->portmaster_action = 1;
          }
          else
          {
            if (shared_mem->big_spaces == 0)
            {
              shared_mem->portmaster_action = 1;
            }
          }
        }

        //Finding a parking spot for the vessel
        shared_mem->portmaster_action = 2;
      }

      if (strcmp(shared_mem->waiting_type, "L") == 0)
      {
        //Checking if the port can support large vessels
        if (shared_mem->big_type != 1)
        {
          shared_mem->portmaster_action = 0;
        }

        //Checking if there are availiable spots
        if (shared_mem->big_spaces == 0)
        {
          shared_mem->portmaster_action = 1;
        }

        //Finding a parking spot for the vessel
        shared_mem->portmaster_action = 2;
      }

      //Answering the vessel
      sem_post(&shared_mem->answer);
    }

    printf("PORTMASTER ONE MORE\n");

    //Ready to accept the next vessel
    sem_post(&shared_mem->approaching);
  }



  return 0;
}
