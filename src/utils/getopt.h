#ifndef GETOPT__H_
#define GETOPT__H_

/* externs for getopt: */
extern int  optind;

extern char *optarg;

int xgetopt(int argc, char **argv, char *opts);

#endif

