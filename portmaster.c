//File: portmaster.c
//The port master acts as the guard of the port.
//Every vessel has to get a permission from him to enter
//and exit the port

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
#include "myfunctions.h"

public_ledger * createPublicLedger(){
  public_ledger *head;

  head = (public_ledger *)malloc(sizeof(public_ledger));
  if (head == NULL)
  {
    perror("Public ledger malloc failed");
    exit(-1);
  }

  head->next = NULL;

  return head;
}

void updatePublicLedger(public_ledger *head, time_t arrival, int v_id, int ps_id, char *v_type, int cost_type){
  struct timeval time;
  public_ledger *tmp = head, *newPage;

  while (tmp->next != NULL)
  {
    tmp = tmp->next;
  }

  newPage = (public_ledger *)malloc(sizeof(public_ledger));
  if (newPage == NULL)
  {
    perror("New page malloc failed");
    exit(-1);
  }

  newPage->status = 0;
  newPage->time_of_arrival = arrival;
  newPage->vessel_id = v_id;
  newPage->parking_space_id = ps_id;
  strcpy(newPage->boat_type, v_type);
  gettimeofday(&time, NULL);
  newPage->time_of_departure = time.tv_sec;
  newPage->total_cost = (newPage->time_of_departure - newPage->time_of_arrival) * cost_type;
  newPage->next = NULL;

  tmp->next = newPage;
}

void printingPublicLedger(public_ledger *head, int shmid){
  public_ledger *tmp = head->next;
  int i, err;
  void *shm;
  shm_management *shared_mem;

  //Attaching the shared memory
  shm = shmat(shmid, (void *) 0, 0);
  if (shm == (void *) - 1)
  {
    perror("Could not attach the shared memory");
    exit(-1);
  }
  shared_mem = (shm_management *)shm;
  shared_mem->parking_spaces = (parking_space *)(shm + sizeof(shm_management));

  printf("\n**************PUBLIC LEDGER**************\n\n");
  printf("**********CURRENT STATE OF PORT**********\n");
  for (i = 0; i < shared_mem->total_spaces; i++)
  {
    printf("Parking Space ID: %d\n", shared_mem->parking_spaces[i].parking_space_id);
    printf("Empty or not: %d\n", shared_mem->parking_spaces[i].empty);
    printf("Type: %s\n", shared_mem->parking_spaces[i].type);
    printf("Vessel ID: %d\n", shared_mem->parking_spaces[i].vessel_id);
    printf("Time of Arrival of the Currently Parked Vessel: %ld\n\n", shared_mem->parking_spaces[i].arrival);
  }

  printf("***********HISTORY OF THE PORT***********\n");
  while (tmp != NULL)
  {
    printf("Parking Space ID: %d\n", tmp->parking_space_id);
    printf("Status of Vessel: %d\n", tmp->status);
    printf("Type: %s\n", tmp->boat_type);
    printf("Vessel ID: %d\n", tmp->vessel_id);
    printf("Time of Arrival: %ld\n", tmp->time_of_arrival);
    printf("Time of Departure: %ld\n", tmp->time_of_departure);
    printf("Total Cost: %d\n\n", tmp->total_cost);
    tmp = tmp->next;
  }

  //Detaching the shared memory before exiting
  err = shmdt((void *)shared_mem);
  if (err == -1)
  {
    perror("Could not detach shared memory");
    exit(-1);
  }
}

int main(int argc, char **argv){
  FILE *charges = NULL;
  char whole_line[100], c_type[1];
  int i, shmid, err, flag, small_cost = -1, medium_cost = -1, big_cost = -1, value, cost;
  void *shm;
  shm_management *shared_mem;
  public_ledger *head;
  struct timeval time;

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
      charges = fopen(argv[i + 1], "r");
      if (charges == NULL)
      {
        perror("Could not open charges file\n");
        exit(-1);
      }

      while (fgets(whole_line, 100, charges))
      {
        if ((strlen(whole_line) > 0) && (whole_line[strlen(whole_line) - 1] == '\n'))
        {
          whole_line[strlen(whole_line) - 1] = '\0';
        }

        sscanf(whole_line, "%s\t%d", c_type, &value);

        if (strcmp(c_type, "S") == 0)
        {
          small_cost = value;
        }
        else if (strcmp(c_type, "M") == 0)
        {
          medium_cost = value;
        }
        else if (strcmp(c_type, "L") == 0)
        {
          big_cost = value;
        }
      }
      i++;
    }
    else
    {
      printf("Only use the following flags: -s or -c\n");
      exit(-1);
    }
  }

  //Initializing the public ledger
  head = createPublicLedger();

  //Attaching the shared memory
  shm = shmat(shmid, (void *) 0, 0);
  if (shm == (void *) - 1)
  {
    perror("Could not attach the shared memory");
    exit(-1);
  }
  shared_mem = (shm_management *)shm;
  shared_mem->parking_spaces = (parking_space *)(shm + sizeof(shm_management));

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
          //Finding an empty space for the current vessel
          if ((strcmp(shared_mem->parking_spaces[i].type, shared_mem->waiting_type) == 0) && (shared_mem->parking_spaces[i].empty == 1))
          {
            //Waiting until noone is moving inside the port
            sem_wait(&shared_mem->port);

            //printf("Vessel %d will park on %d\n", shared_mem->vessel_id, shared_mem->parking_spaces[i].parking_space_id);
            gettimeofday(&time, NULL);
            shared_mem->parking_spaces[i].arrival = time.tv_sec;
            shared_mem->parking_spaces[i].empty = 0;
            shared_mem->parking_spaces[i].vessel_id = shared_mem->vessel_id;
            write_to_logfile("entered the port at:", shared_mem->vessel_id, time.tv_sec);
            break;
          }
        }
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
            cost = small_cost;
            shared_mem->small_spaces--;
          }
          else if (strcmp(shared_mem->parking_spaces[i].type, "M") == 0)
          {
            cost = medium_cost;
            shared_mem->medium_spaces--;
          }
          else if (strcmp(shared_mem->parking_spaces[i].type, "L") == 0)
          {
            cost = big_cost;
            shared_mem->big_spaces++;
          }

          //Updating the public ledger once a vessel departs
          updatePublicLedger(head, shared_mem->parking_spaces[i].arrival, shared_mem->parking_spaces[i].vessel_id, shared_mem->parking_spaces[i].parking_space_id, shared_mem->parking_spaces[i].type, cost);
          shared_mem->parking_spaces[i].vessel_id = 0;
          shared_mem->parking_spaces[i].arrival = 0;

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

  //Printing the whole public ledger before exiting
  printingPublicLedger(head, shmid);

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
