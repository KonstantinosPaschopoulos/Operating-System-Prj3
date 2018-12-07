//File: mytypes.h
//I'm defining here all the types I will use

#include <semaphore.h>
#include <sys/types.h>

#ifndef MYTYPES_H
#define MYTYPES_H

#define SEGMENTPERM 0666

//Each parking space is represented by this struct
typedef struct parking_space {
  int parking_space_id;
  int empty;
  char type[1];
  int vessel_id;
  double arrival;
} parking_space;

//A struct that holds a single page of the public ledger
typedef struct public_ledger {
  int status;
  double time_of_arrival;
  int vessel_id;
  int parking_space_id;
  char boat_type[1];
  int total_cost;
  double time_of_departure;
  struct public_ledger *next;
} public_ledger;

//The struct that holds everything I need to have in the shared memory segment
typedef struct shm_management {
  sem_t approaching;
  sem_t portmaster;
  sem_t port;
  sem_t mutex;
  sem_t answer;
  int total_spaces;
  int small_spaces;
  int medium_spaces;
  int big_spaces;
  int small_type;
  int medium_type;
  int big_type;
  int small_cost;
  int medium_cost;
  int big_cost;
  int profit;
  float avg_profit;
  float avg_waiting_time;
  int small_waiting;
  int medium_waiting;
  int big_waiting;
  char waiting_type[1];
  int waiting_upgrade;
  int vessel_action;
  int vessel_id;
  int portmaster_action;
  int closing_time;
  parking_space *parking_spaces;
} shm_management;

#endif
