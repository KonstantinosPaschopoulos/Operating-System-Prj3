#include "mytypes.h"

//Either creates a new logfile or empties the old one
void create_logfile();
//Writes to the logfile the given string
void write_to_logfile(char *, int, long int);
//Removes the logfile from the system
void remove_logfile();
//Calculates the current statistics from the shared memory
void print_statistics(void *);
//Prints a snapshot of the port at the current time
void print_port(void *);
//Initializes the public ledger that is lives locally in the portmaster
public_ledger * createPublicLedger();
//Updates the public ledger after a vessel leaves
int updatePublicLedger(public_ledger *, time_t, int, int, char *, int);
//Prints only the history of the port
void printingPublicLedger(public_ledger *);
//Deleting all the memory that was allocated for the public ledger
void removePublicLedger(public_ledger *);
