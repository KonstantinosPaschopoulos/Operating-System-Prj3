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
  int i, shmid, err, flag;
  void *shm;
  shm_management *shared_mem;

  //Parsing the input
  for (i = 1; i < argc; i++)
  {
    if (strcmp(argv[i], "-s") == 0)
    {
      shmid = atoi(argv[i + 1]);
      i++;
    }
    else if (strcmp(argv[i], "-c") == 0)
    {
      i++;
    }
    else
    {
      printf("Only use the following flags: -s or -c\n");
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
  shared_mem->parking_spaces = (parking_space *)(shm + sizeof(shm_management));

  for (i = 0; i < shared_mem->total_spaces; i++)
  {
    printf("%d %s\n", shared_mem->parking_spaces[i].parking_space_id, shared_mem->parking_spaces[i].type);
  }

  //The port-master is constantly working
  while (1)
  {
    //The port-master waits until a vessel talks to him
    sem_wait(&shared_mem->portmaster);

    printf("The port-master is deciding what to do\n");

    if (shared_mem->vessel_action == 0)
    {
      //The vessel asks where to park
      flag = 0;
      if (strcmp(shared_mem->waiting_type, "S") == 0)
      {
        //Checking if the port can support small vessels
        if (shared_mem->small_type == 0)
        {
          if (shared_mem->waiting_upgrade == 1)
          {
            if ((shared_mem->medium_type == 0) && (shared_mem->big_type == 0))
            {
              shared_mem->portmaster_action = 0;
              flag = 1;
            }
          }
          else
          {
            shared_mem->portmaster_action = 0;
            flag = 1;
          }
        }

        //Checking if there are availiable spots
        if ((shared_mem->small_spaces == 0) && (flag == 0))
        {
          if (shared_mem->waiting_upgrade == 0)
          {
            shared_mem->portmaster_action = 1;
            flag = 1;
          }
          else
          {
            if ((shared_mem->medium_spaces == 0) && (shared_mem->big_spaces == 0))
            {
              shared_mem->portmaster_action = 1;
              flag = 1;
            }
          }
        }

        //Finding a parking spot for the vessel
        if (flag == 0)
        {
          shared_mem->portmaster_action = 2;
          shared_mem->small_spaces--;
        }
      }
      else if (strcmp(shared_mem->waiting_type, "M") == 0)
      {
        //Checking if the port can support medium vessels
        if (shared_mem->medium_type == 0)
        {
          if (shared_mem->waiting_upgrade == 1)
          {
            if (shared_mem->big_type == 0)
            {
              shared_mem->portmaster_action = 0;
              flag = 1;
            }
          }
          else
          {
            shared_mem->portmaster_action = 0;
            flag = 1;
          }
        }

        //Checking if there are availiable spots
        if ((shared_mem->medium_spaces == 0) && (flag == 0))
        {
          if (shared_mem->waiting_upgrade == 0)
          {
            shared_mem->portmaster_action = 1;
            flag = 1;
          }
          else
          {
            if (shared_mem->big_spaces == 0)
            {
              shared_mem->portmaster_action = 1;
              flag = 1;
            }
          }
        }

        //Finding a parking spot for the vessel
        if (flag == 0)
        {
          shared_mem->portmaster_action = 2;
          shared_mem->medium_spaces--;
        }
      }
      else if (strcmp(shared_mem->waiting_type, "L") == 0)
      {
        //Checking if the port can support large vessels
        if (shared_mem->big_type == 0)
        {
          shared_mem->portmaster_action = 0;
          flag = 1;
        }

        //Checking if there are availiable spots
        if ((flag == 0) && (shared_mem->big_spaces == 0))
        {
          shared_mem->portmaster_action = 1;
          flag = 1;
        }

        if (flag == 0)
        {
          shared_mem->portmaster_action = 2;
          shared_mem->big_spaces--;
        }
      }

      //Answering the vessel
      if (shared_mem->portmaster_action == 2)
      {
        //The vessel can park somewhere but the port-master has to make
        //sure no one is moving in the port before answering
        for (i = 0; i < shared_mem->total_spaces; i++)
        {
          if ((strcmp(shared_mem->parking_spaces[i].type, shared_mem->waiting_type) == 0) && (shared_mem->parking_spaces[i].empty == 1))
          {
            //We found an empty space for the current vessel
            printf("WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW");
            printf(" Vessel %d will park on %d\n", shared_mem->vessel_id, shared_mem->parking_spaces[i].parking_space_id);
            shared_mem->parking_spaces[i].empty = 0;
            shared_mem->parking_spaces[i].vessel_id = shared_mem->vessel_id;
            break;
          }
        }
        sem_wait(&shared_mem->port);
      }
      sem_post(&shared_mem->answer);

      //The port-master will be unavailable until the vessel
      //has received the answer
      sem_wait(&shared_mem->portmaster);
    }
    else if (shared_mem->vessel_action == 1)
    {
      //Waits for when noone is moving inside the port
      for (i = 0; i < shared_mem->total_spaces; i++)
      {
        if (shared_mem->parking_spaces[i].vessel_id == shared_mem->vessel_id)
        {
          printf("Vessel %d will unpark from %d\n", shared_mem->vessel_id, shared_mem->parking_spaces[i].parking_space_id);
          shared_mem->parking_spaces[i].empty = 1;
          if (strcmp(shared_mem->parking_spaces[i].type, "S") == 0)
          {
            shared_mem->small_spaces--;
          }
          else if (strcmp(shared_mem->parking_spaces[i].type, "M") == 0)
          {
            shared_mem->medium_spaces--;
          }
          else if (strcmp(shared_mem->parking_spaces[i].type, "L") == 0)
          {
            shared_mem->big_spaces++;
          }
          break;
        }
      }
      sem_wait(&shared_mem->port);
      sem_post(&shared_mem->answer);
      sem_wait(&shared_mem->portmaster);
    }
    else if (shared_mem->vessel_action == -1)
    {
      break;
    }

    printf("The port-master is ready to assist the next vessel\n");

    //Ready to accept the next vessel
    sem_post(&shared_mem->approaching);
  }

  //Detaching the shared memory before exiting
  err = shmdt((void *)shared_mem);
  if (err == -1)
  {
    perror("Could not detach shared memory");
    exit(-1);
  }

  printf("Port-master has finished\n");

  return 0;
}
