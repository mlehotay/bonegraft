/* CVS $Id: graft.c,v 1.5 2005/08/16 21:01:19 Michael Exp $ */
/* Copyright (c) Michael Lehotay, 2005. */
/* Bone Graft may be freely redistributed. See license.txt for details. */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <ctype.h>
#include <assert.h>
#include "getopt.h"
#include "graft.h"
#include "file.h"

boolean quiet;

/* private functions */
static void usage(void);
static void parseopts(int argc, char *argv[]);
static int readarg(char *arg, char opt);


/*****************************************************************************
 * main                                                                      *
 *****************************************************************************/
int main(int argc, char *argv[]) {
    parseopts(argc, argv);
    readbones(argv[argc-1]);
    if(!quiet) {
	printf("Bone Graft v0.01\n");
	printbones();
    } freebones();
    return 0;
}


/*****************************************************************************
 * alloc                                                                     *
 *****************************************************************************
 * try to allocate memory, bail if unsuccessful
 */
void *alloc(unsigned len) {
    void *p;

    assert(len>0);
    if((p = malloc(len)) == NULL)
	bail(SYSTEM_ERROR, "out of memory");
    return p;
}


/*****************************************************************************
 * bail                                                                      *
 *****************************************************************************/
void bail(int status, char *message, ... ) {
    const char *exclaim[] = { "Ack! Pfft!", "Bah!", "Whoops!", "Argh!",
	"Uh oh!", "Double plus ungood!", "Grrr!", "D'oh!", "Gah!" };

    if(!quiet) {
	if(message == NULL)
	    fprintf(stderr, "Ahh! The bugs! Get them off!\n");
	else {
	    va_list args;

	    assert(*message != '\0');
	    assert(*message != '%'); /* don't mess with printf specifiers */
	    srand(time(NULL));

	    va_start(args, message);
	    fprintf(stderr, "%s %c", exclaim[rand() % (sizeof(exclaim) /
		sizeof(exclaim[0]))], toupper(*(message++)));
	    vfprintf(stderr, message, args);
	    fprintf(stderr, "!\n");
	    va_end(args);
	}
    }

    bclose();
    freebones();
    exit(status);
}


/*****************************************************************************
 * usage                                                                     *
 *****************************************************************************/
static void usage(void) {
    fprintf(stderr, "Usage: graft [options] <filename>\n");
    fprintf(stderr, "  -b      switch byte ordering\n");
    fprintf(stderr, "  -i      use 16-bit ints\n");
    fprintf(stderr, "  -p      use 16-bit pointers\n");
    fprintf(stderr, "  -a <n>  align struct members on n-byte boundaries\n");
    fprintf(stderr, "  -s <n>  pad structs to multiples of n-bytes\n");
    fprintf(stderr, "  -f <n>  padding of structs containing bitfields\n");
    fprintf(stderr, "  -u <n>  store bitfields in n-byte units\n");
    fprintf(stderr, "  -m      pack bitfields from MSB to LSB\n");
    fprintf(stderr, "  -c      allow bitfields to cross unit boundaries\n");
    fprintf(stderr, "  -o      features include invisible objects\n");
    fprintf(stderr, "  -g      features include nosignal option\n");
    fprintf(stderr, "  -y      struct you is very large\n");
    fprintf(stderr, "  -n      just print magic numbers and exit\n");
    fprintf(stderr, "  -t      try to process unsupported bones versions\n");
    fprintf(stderr, "  -q      be quiet\n");
    fprintf(stderr, "  -v      be more verbose\n");
    exit(USAGE_ERROR);
}


/*****************************************************************************
 * parseopts                                                                 *
 *****************************************************************************
 * parse command line options, set global variables
 */
static void parseopts(int argc, char *argv[]) {
    int c;

    /* read command line options */
    while((c=getopt(argc, argv, "bipa:s:f:u:mcogynqvt")) != EOF) {
	switch (c) {
	case 'b':
	    switchbytes = TRUE;
	    break;
	case 'i':
	    intsz = 2;
	    break;
	case 'p':
	    pointersz = 2;
	    break;
	case 'a':
	    memberalign = readarg(optarg, 'a');
	    break;
	case 's':
	    structalign = readarg(optarg, 's');
	    break;
	case 'f':
	    fieldalign = readarg(optarg, 'f');
	    break;
	case 'u':
	    fieldsz = readarg(optarg, 'u');
	    break;
	case 'm':
	    fieldMSB = TRUE;
	    break;
	case 'c':
	    fieldspan = TRUE;
	    break;
	case 'o':
	    invisibleobjects = TRUE;
	    break;
	case 'g':
	    nosignal = TRUE;
	    break;
	case 'y':
	    bigyou = TRUE;
	    break;
	case 'n':
	    headeronly = TRUE;
	    break;
	case 't':
	    forceversion = TRUE;
	    break;
	case 'q':
	    if(verbose)
		bail(USAGE_ERROR, "can't be both quiet and verbose");
	    quiet = TRUE;
	    break;
	case 'v':
	    if(quiet)
		bail(USAGE_ERROR, "can't be both quiet and verbose");
	    verbose = TRUE;
	    break;
	default:
	    usage();
	}
    }

    /* make sure filename was specified, and nothing else afterwards */
    if(optind != argc-1)
	usage();
}


/*****************************************************************************
 * readarg                                                                   *
 *****************************************************************************
 * read an integer from the command line, check that its value is 1,2, or 4
 */
static int readarg(char *arg, char opt) {
    char *p;
    int i;

    /* check for missing argument */
    if(arg == NULL || *arg == '\0')
	usage();

    /* check for non-digits */
    for(p = arg; *p; p++) {
	if(!isdigit(*p))
	    bail(USAGE_ERROR, "non-numeric argument to -%c option", opt);
    }

    /* check for unexpected values */
    i = atoi(optarg);
    if(i!=1 && i!=2 && i!=4)
	bail(USAGE_ERROR, "-%c argument must be 1, 2 or 4", opt);

    return i;
}
