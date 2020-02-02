/* CVS $Id: file.h,v 1.2 2005/06/23 20:35:09 Michael Exp $ */
/* Copyright (c) Michael Lehotay, 2003. */
/* Bone Graft may be freely redistributed. See license.txt for details. */

#ifndef FILE_H
#define FILE_H

#include "graft.h"

#define CHARSZ    1
#define SHORTSZ   2
#define MAXSZ     ((intsz > pointersz) ? intsz : pointersz)

typedef struct st {
    unsigned nbytes;	/* number of bytes read so far */
    unsigned align;	/* alignment requirement */
    uint32 fieldbuf;	/* buffer for reading bitfields */
    unsigned nbits;	/* number of unread bits fieldbuf */
    struct st *parent;	/* enclosing struct, if this is a nested struct */
} struct_t;

extern boolean switchbytes, fieldMSB, fieldspan;
extern unsigned intsz, longsz, fieldsz, pointersz;
extern unsigned memberalign, structalign, fieldalign;
extern boolean zerocomp;

void bopen(char *fname);
void bclose(void);
void testeof(void);
void startcount(void);
unsigned getcount(void);
struct_t *startstruct(struct_t *parent, boolean hasfield, unsigned max);
void endstruct(struct_t *st);
void align(struct_t *st, unsigned align);
void zread(void *buf, unsigned len, unsigned num, struct_t *st);
void iread(void *buf, unsigned buflen, unsigned len, struct_t *st);
uint32 bread(unsigned len, struct_t *st);

#endif
