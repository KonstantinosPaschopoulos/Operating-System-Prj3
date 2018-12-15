//File: mytypes.h
//I'm defining here all the types I will use

#include <semaphore.h>
#include <sys/types.h>

#ifndef MYTYPES_H
#define MYTYPES_H

#define SEGMENTPERM 0666

//Some colors to use when printing to make it clearer
#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"

//Each parking space is represented by this struct
typedef struct parking_space {
  int parking_space_id;
  int empty;
  char type[10];
  int vessel_id;
  time_t arrival;
} parking_space;

//A struct that holds a single page of the public ledger
typedef struct public_ledger {
  int status;
  time_t time_of_arrival;
  int vessel_id;
  int parking_space_id;
  char boat_type[10];
  int total_cost;
  time_t time_of_departure;
  struct public_ledger *next;
} public_ledger;

//The struct that holds everything I need to have in the shared memory segment
typedef struct shm_management {
  sem_t approaching;
  sem_t portmaster;
  sem_t mutex;
  sem_t port;
  sem_t answer;
  sem_t stuck_vessel;
  int vessel_is_waiting;
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
  int small_profit;
  int medium_profit;
  int big_profit;
  int small_waiting;
  int medium_waiting;
  int big_waiting;
  int small_vessels_count;
  int medium_vessels_count;
  int big_vessels_count;
  long int waiting_time;
  char waiting_type[10];
  int waiting_upgrade;
  char waiting_upgrade_type[10];
  int vessel_action;
  int vessel_id;
  int portmaster_action;
  int closing_time;
  parking_space *parking_spaces;
} shm_management;

#endif
