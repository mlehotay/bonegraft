/* CVS $Id: graft.h,v 1.3 2005/06/23 20:35:09 Michael Exp $ */
/* Copyright (c) Michael Lehotay, 2005. */
/* Bone Graft may be freely redistributed. See license.txt for details. */

#ifndef GRAFT_H
#define GRAFT_H

/* most recent version of NetHack we know about */
#define LATEST_VERSION 0x03070013 /* todo: update to 3.7.0-102, need to check representation */

/* exit codes */
#define SYSTEM_ERROR   1
#define USAGE_ERROR    2
#define SEMANTIC_ERROR 3

typedef enum {FALSE, TRUE} boolean;
typedef char int8;
typedef short int16;
typedef int int32;
typedef long int64;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long uint64;

/* graft.c */
void *alloc(unsigned len);
void bail(int status, char *message, ... );

/* bread.c */
extern boolean goldobj, invisibleobjects, nosignal, bigyou;
extern boolean headeronly, forceversion;
void readbones(char *fname);
void freebones(void);

/* bprint.c */
extern boolean verbose, debug;
void printbones(void);

#endif
