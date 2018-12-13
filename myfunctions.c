#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "mytypes.h"
#include "myfunctions.h"

void create_logfile(){
  FILE *logfile = NULL;

  logfile = fopen("logfile", "w");
  if (logfile == NULL)
  {
    perror("Couldn't create logfile");
    exit(-1);
  }

  fclose(logfile);
}

void write_to_logfile(char *message, int v_id, long int time){
  FILE *logfile = NULL;

  logfile = fopen("logfile", "a");
  if (logfile == NULL)
  {
    perror("Couldn't write to logfile");
    exit(-1);
  }

  fprintf(logfile, "Vessel %d %s %ld\n", v_id, message, time);

  fclose(logfile);
}

void remove_logfile(){
  if (remove("logfile") == -1)
  {
    perror("Couldn't remove logfile");
    exit(-1);
  }
}

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
  printf("Average profit: %f\n", ((float)profit / (float)count));
  if (shared_mem->small_vessels_count != 0)
  {
    printf("Average profit of small vessels: %f\n", ((float)shared_mem->small_profit / (float)shared_mem->small_vessels_count));
  }
  if (shared_mem->medium_vessels_count != 0)
  {
    printf("Average profit of medium vessels: %f\n", ((float)shared_mem->medium_profit / (float)shared_mem->medium_vessels_count));
  }
  if (shared_mem->big_vessels_count != 0)
  {
    printf("Average profit of large vessels: %f\n", ((float)shared_mem->big_profit / (float)shared_mem->big_vessels_count));
  }

  waiting = shared_mem->small_waiting + shared_mem->medium_waiting + shared_mem->big_waiting;
  printf("Total waiting time: %d\n", waiting);
  printf("Average waiting time: %f\n", ((float)waiting / (float)count));
  if (shared_mem->small_vessels_count != 0)
  {
    printf("Average waiting time of small vessels: %f\n", ((float)shared_mem->small_waiting / (float)shared_mem->small_vessels_count));
  }
  if (shared_mem->medium_vessels_count != 0)
  {
    printf("Average waiting time of medium vessels: %f\n", ((float)shared_mem->medium_waiting / (float)shared_mem->medium_vessels_count));
  }
  if (shared_mem->big_vessels_count != 0)
  {
    printf("Average waiting time of large vessels: %f\n", ((float)shared_mem->big_waiting / (float)shared_mem->big_vessels_count));
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

int updatePublicLedger(public_ledger *head, time_t arrival, int v_id, int ps_id, char *v_type, int cost_type){
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

  write_to_logfile("departed from the port at:", v_id, time.tv_sec);
  write_to_logfile("spent a total time of:", v_id, (newPage->time_of_departure - newPage->time_of_arrival));
  write_to_logfile("paid:", v_id, newPage->total_cost);

  tmp->next = newPage;

  return newPage->total_cost;
}

void printingPublicLedger(public_ledger *head){
  public_ledger *tmp = head->next;

  printf("\nHISTORY OF THE PORT\n\n");
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
}

void removePublicLedger(public_ledger *head){
  public_ledger *tmp, *current;

  current = head;
  while (current != NULL)
  {
    tmp = current;
    current = current->next;
    free(tmp);
  }
}
