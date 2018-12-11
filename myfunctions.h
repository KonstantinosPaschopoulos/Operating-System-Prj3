#include "mytypes.h"

void create_logfile();
void write_to_logfile(char *, int, long int);
void remove_logfile();
void print_statistics(void *);
void print_port(void *);
public_ledger * createPublicLedger();
void updatePublicLedger(public_ledger *, time_t, int, int, char *, int);
void printingPublicLedger(public_ledger *);
