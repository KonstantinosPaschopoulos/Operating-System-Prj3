#include <stdio.h>
#include <stdlib.h>
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
