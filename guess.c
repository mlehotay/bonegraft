/* CVS $Id: guess.c,v 1.7 2005/08/16 21:01:19 Michael Exp $ */
/* Copyright (c) Michael Lehotay, 2005. */
/* Bone Graft may be freely redistributed. See license.txt for details. */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <process.h>

/* 
 * Usage: graft [options] <filename>
 *   -b      switch byte ordering
 *   -i      use 16-bit ints
 *   -p      use 16-bit pointers
 *   -a <n>  align struct members on n-byte boundaries
 *   -s <n>  pad structs to multiples of n-bytes
 *   -f <n>  padding of structs containing bitfields
 *   -u <n>  store bitfields in n-byte units
 *   -m      pack bitfields from MSB to LSB
 *   -c      allow bitfields to cross unit boundaries
 *   -o      features include invisible objects
 *   -g      features include nosignal option
 */

#define MAX_OPT 8
#define MAX_ARG 4
#define HIGHOPT (1<<MAX_OPT)
#define HIGHARG (1<<(MAX_ARG*2))
#define VSIZE	MAX_OPT + MAX_ARG

static char *opt[] = { "-y", "-b", "-m", "-i", "-p", "-c", "-o", "-g"};
static char *arg[] = { "-a?", "-s?", "-f?", "-u?" };
static char *vector[VSIZE + 4] = { "graft", "-q" };

int main(int argc, char *argv[]) {
    unsigned long argmask, optmask;
    unsigned long good=0, bad=0;
    int pos, i, status;
    FILE *fp;

    if(argc!=2) {
	fprintf(stderr, "Usage: guess <file>\n");
	exit(2);
    }
    if((fp = fopen(argv[1], "rb")) == NULL) {
	fprintf(stderr, "Unable to open file: %s", argv[1]);
	exit(1);
    }
    fclose(fp);

    printf("Guess v0.01 (a tool for Bone Graft)\n");
    printf("Trying %g combinations of options. This might take a little while...\n",
	pow(2, MAX_OPT) * pow(4, MAX_ARG));
    
    for(argmask=0; argmask<HIGHARG; argmask++) {
	for(optmask=0; optmask<HIGHOPT; optmask++) {
	    pos = 2;

	    for(i=0; i<MAX_ARG; i++) {
		int x = (argmask>>(i*2)) & 0x03;
		if(x!=2) {
		    arg[i][2] = (char) ('1' + x);
		    vector[pos++] = arg[i];
		}
	    }
	    
	    for(i=0; i<MAX_OPT; i++) {
		if(optmask & (1<<i))
		    vector[pos++] = opt[i];
	    }

	    vector[pos++] = argv[1];	    
	    vector[pos] = NULL;
	    
	    status = spawnv(P_WAIT, vector[0], vector);
	    switch(status) {
	    case 0:
		printf("%s", vector[0]);
		for(i=2; i<pos; i++)
		    printf(" %s", vector[i]);
		printf("\n");
		good++;
		break;
	    case 3:
		bad++;
		break;
	    default:
		printf("exit code %d, aborting\n", status);
		printf("graft options were:");
		for(i=2; i<pos; i++)
		    printf(" %s", vector[i]);
		exit(0);
		break;
	    }
	}
    }
    
    printf("\nFinished. %ld successes, %ld failures.\n", good, bad);
    return 0;
}
