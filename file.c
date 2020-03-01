/* CVS $Id: file.c,v 1.3 2005/06/23 20:35:09 Michael Exp $ */
/* Copyright (c) Michael Lehotay, 2003. */
/* Bone Graft may be freely redistributed. See license.txt for details. */

/*
 * Portions of this file were derived from the following NetHack source files:
 * restore.c, Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985.
 * save.c, Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "graft.h"
#include "file.h"

/* command line options */
boolean switchbytes;      /* reverse byte ordering */
unsigned intsz = 4;       /* size of ints */
unsigned longsz = 4;       /* size of longs */
unsigned pointersz = 4;   /* size of pointers */
unsigned fieldsz = 2;     /* size of bitfield units (aka words) */
unsigned memberalign = 4; /* struct member alignment = min(x, member size) */
unsigned structalign = 0; /* alignment of structs w/o bitfields 0 = by member */
unsigned fieldalign = 0;  /* alignment of structs that contain bitfields */
boolean fieldMSB;         /* bitfields start at MSB, not LSB */
boolean fieldspan;        /* bitfields can span bitfield units */

boolean zerocomp;
static uint8 nzeroes = 0; /* zeros in the run (for zero compression) */

static boolean countonly;
static unsigned numbytes;
static FILE *fp;

/* private functions */
static void eread(void *buf, unsigned len, unsigned num);
static void clearfield(struct_t *st);


/*****************************************************************************
 * bopen                                                                     *
 *****************************************************************************
 * open the bones file
 */
void bopen(char *fname) {
    assert(fname != NULL);
    assert(fp == NULL);

    if((fp = fopen(fname, "rb")) == NULL)
        bail(SYSTEM_ERROR, "unable to open file: %s", fname);
}


/*****************************************************************************
 * bclose                                                                    *
 *****************************************************************************
 * close the bones file
 */
void bclose(void) {
    if(fp != NULL)
        fclose(fp);
    fp = NULL;
}


/*****************************************************************************
 * testeof                                                                   *
 *****************************************************************************
 * make sure we are at the end of the file
 */
void testeof(void) {
    char c;

    assert(fp != NULL);
    if(fread(&c, 1, 1, fp) || !feof(fp))
        bail(SEMANTIC_ERROR, "extra junk at end of file");

    if(nzeroes)
        bail(SEMANTIC_ERROR, "file ends in middle of zerocomp run");
}


/*****************************************************************************
 * startcount                                                                *
 *****************************************************************************
 * start counting struct sizes, don't actually read from file
 */
void startcount(void) {
    assert(!countonly);
    countonly = TRUE;
}

/*****************************************************************************
 * getcount                                                                  *
 *****************************************************************************
 * return the size of the most recent struct
 */
unsigned getcount(void) {
    assert(countonly);
    countonly = FALSE;
    return numbytes;
}


/*****************************************************************************
 * startstruct                                                               *
 *****************************************************************************
 * start reading a struct from file, attempt to deal with alignment issues
 *
 * We need to know the alignment of nested structs before they are read so we
 * can align it properly inside the parent struct. For unnested structs we
 * could figure it out at the end, but let's always require it up front to be
 * consistent.
 */
struct_t *startstruct(struct_t *parent, boolean hasfields, unsigned maxlen) {
    struct_t *st;

    if(debug) {
        printf("Starting %s struct maxlen=%u",
               (parent == NULL) ? "unnested" : "nested", maxlen);
    }

    assert(maxlen==1 || maxlen==2 || maxlen==4 || maxlen==8);
    assert(fieldsz==1 || fieldsz==2 || fieldsz==4);
    assert(memberalign==1 || memberalign==2 || memberalign==4 || memberalign==8);
    assert(fieldalign>=0 && fieldalign<=4 && fieldalign!=3);
    assert(structalign==0 || structalign==1 || structalign==2 ||
           structalign==4 || structalign==8);

    st = alloc(sizeof(struct_t));

    /* adjust alignment for fieldsz and memberalign */
    if(hasfields && fieldsz>maxlen) {
        maxlen = fieldsz;
        if(debug) {
            printf(", fieldsz=%u", fieldsz);
        }
    }

    if(memberalign<maxlen) {
        maxlen = memberalign;
        if(debug) {
            printf(", memberalign=%u", memberalign);
        }
    }

    /* adjust for structalign and fieldalign */
    /* probably need min and max values here. figure it out later */
    if(hasfields) {
        st->align = (fieldalign>maxlen) ? fieldalign : maxlen;
    } else {
        st->align = (structalign && structalign<maxlen) ? structalign : maxlen;
    }
    if(debug) {
        printf(", will align at %u bytes\n", st->align);
    }

    assert(st->align==1 || st->align==2 || st->align==4 || st->align==8);

    st->nbytes = st->nbits = 0;
    st->parent = parent;

    if(parent!=NULL) {
        assert(parent->align >= st->align);
        align(parent, st->align);
    }

    return st;
}


/*****************************************************************************
 * endstruct                                                                 *
 *****************************************************************************
 * finish reading a struct from file, attempt to deal with alignment issues
 */
void endstruct(struct_t *st) {
    assert(st != NULL);
    clearfield(st);
    align(st, st->align);
    numbytes = st->nbytes;
    if(st->parent != NULL) {
        st->parent->nbytes += st->nbytes;
    }
    free(st);

    if(debug) {
        printf("End of struct, read %u bytes...\n", numbytes);
    }
}


/*****************************************************************************
 * align                                                                     *
 *****************************************************************************
 * read some bytes from the file to align struct properly
 */
void align(struct_t *st, unsigned alignment) {
    uint8 foo;

    assert(st != NULL);
    assert(alignment==1 || alignment==2 || alignment==4 || alignment==8);

    while((st->nbytes) % alignment) {
        zread(&foo, 1, 1, NULL);
        st->nbytes++;
        if(debug) {
            printf("[%02x]", foo);
        }
        /*
           if(foo)
            bail(SEMANTIC_ERROR, "non-zero struct padding");
         */
    }
}


/*****************************************************************************
 * zread                                                                     *
 *****************************************************************************
 * read from file, do zero decompression, struct padding if necessary
 * zero decompression algorithm modified from NetHack save.c and restore.c
 */
void zread(void *buf, unsigned len, unsigned num, struct_t *st) {
    unsigned i;

    assert(buf != NULL);
    assert(len==1 || len==2 || len==4 || len==8);
    assert(num>0);
    assert(memberalign==1 || memberalign==2 || memberalign==4 || memberalign==8);

    /* keep track of struct padding */
    if(st != NULL) {
        clearfield(st);
        align(st, (memberalign<len) ? memberalign : len);
        st->nbytes += len*num;
    }

    if(debug) {
        printf("zread %u x %u bytes...\n", num, len);
    }

    /* read num items of len bytes */
    while(num--) {
        if(countonly) { /* don't change run length or file position */
            for(i=0; i<len; i++)
                *((char *)buf+i) = '\0';
        } else if(zerocomp) {
            for(i=0; i<len; i++) {
                if(nzeroes > 0) { /* in a run */
                    *((char *)buf+i) = '\0';
                    nzeroes--;
                } else {
                    eread((char *)buf+i, 1, 1);
                    if(*((char *)buf+i) == '\0') /* starting a new run */
                        eread(&nzeroes, 1, 1); /* read run length */
                }
            }
        } else
            eread(buf, len, 1);

        buf = (char *) buf + len; /* advance buf pointer to read next item */
    }
}


/*****************************************************************************
 * iread                                                                     *
 *****************************************************************************
 * read an integer from file, do byte re-ordering if necessary
 */
void iread(void *buf, unsigned buflen, unsigned len, struct_t *st) {
    uint8 buf1;
    uint16 buf2;
    uint64 buf4;
    uint64 buf8, val=0;

    assert(buf != NULL);
    assert(buflen==1 || buflen==2 || buflen==4 || buflen==8);
    assert(len==1 || len==2 || len==4 || len==8);
    assert(buflen >= len);

    switch(len) {
    case 1:
        zread(&buf1, 1, 1, st);
        val = buf1;
        break;
    case 2:
        zread(&buf2, 2, 1, st);
        if(switchbytes)
            buf2 = (uint16) (((buf2>>8)&0xff) | ((buf2&0xff)<<8));
        val = buf2;
        break;
    case 4:
        zread(&buf4, 4, 1, st);
        if(switchbytes)
            buf4 = (((buf4>>24)&0xff) | ((buf4&0xff)<<24) |
                    ((buf4>>8)&0xff00) | ((buf4&0xff00)<<8));
        val = buf4;
        break;
    case 8:
        zread(&buf8, 8, 1, st);
        if(switchbytes) {
            /* (uint64)(ChangeEndianness32(value & 0xffffffff)) << 32 |
                                ChangeEndianness32(value >> 32); */
            buf8 = (((buf8>>56)&0xff) | ((buf8&0xff)<<56) |
                    ((buf8>>40)&0xff00) | ((buf8&0xff00)<<40) |
                    ((buf8>>24)&0xff0000) | ((buf8&0xff0000)<<24) |
                    ((buf8>>8)&0xff000000) | ((buf8&0xff000000)<<8));
        }
        val = buf8;
    }

    switch(buflen) {
    case 1:
        *((uint8 *) buf) = (uint8) val;
        break;
    case 2:
        *((uint16 *) buf) = (uint16) val;
        break;
    case 4:
        *((uint64 *) buf) = (uint64) val;
        break;
    case 8:
        *((uint64 *) buf) = (uint64) val;
    }

    if(debug) {
        printf("iread %u bytes: %lx\n", len, val);
    }
}


/*****************************************************************************
 * bread                                                                     *
 *****************************************************************************
 * read a bitfield from the file
 */
uint64 bread(unsigned len, struct_t *st) {
    uint64 mask, val;
    unsigned i, spanned = 0;

    assert(len > 0);
    assert(len <= fieldsz*8);
    assert(st != NULL);

    if(st->nbits<len) {
        if(st->nbits && fieldspan) { /* this field spans units */
            spanned = len - st->nbits;
            len = st->nbits;
        } else { /* read in a new bitfield unit */
            iread(&st->fieldbuf, sizeof(st->fieldbuf), fieldsz, st);
            st->nbits = fieldsz*8;
        }
    }

    mask = 0;
    for(i=0; i<len; i++) {
        mask = (mask<<1) | 1;
    }

    if(fieldMSB)
        val = (st->fieldbuf >> (st->nbits - len)) & mask;
    else
        val = (st->fieldbuf >> (fieldsz*8 - st->nbits)) & mask;

    st->nbits -= len;

    if(spanned) {
        uint64 spanval = bread(spanned, st);
        if(fieldMSB)
            val = (val << spanned) | spanval;
        else
            val = (spanval << len) | val;
    }

    if(debug) {
        printf("bread %u bytes: %lx\n", len, val);
    }

    return val;
}


/*****************************************************************************
 * eread                                                                     *
 *****************************************************************************
 * read some data from file, terminate if unsuccessful
 */
static void eread(void *buf, unsigned len, unsigned num) {
    assert(buf != NULL);
    assert(len==1 || len==2 || len==4 || len==8);
    assert(num > 0);
    assert(fp != NULL);

    if(num != fread(buf, len, num, fp)) {
        if(feof(fp)) {
            bail(SEMANTIC_ERROR, "unexpected end of file");
        } else {
            bail(SYSTEM_ERROR, "error reading file");
        }
    }

    /*
     * uint64 word = 0x0A0B0C0D;
     *     Modern little-endian: 0d 0c 0b 0a
     *     Modern big-endian: 0a 0b 0c 0d
     *     PDP-11 (16-bit, little-endian word, big-endian order): 0b 0a 0d 0c
     *     PDP-11 (16-bit, big-endian word, little-endian order): 0c 0d 0a 0b
     */
    if(debug) {
        int i;
        printf("eread %u bytes: ", len);
        for(i=0; i<len; i++) {
            printf("%02x ", ((unsigned char *) buf)[i]);
        }
        printf("\n");
    }
}

/*****************************************************************************
 * clearfield                                                                *
 *****************************************************************************
 * make sure no unread bitfields are in the buffer
 */
static void clearfield(struct_t *st) {
    int64 foo;

    if(st!=NULL && st->nbits) {
        foo = bread(st->nbits, st);

        if(foo)
            bail(SEMANTIC_ERROR, "unread bitfields in buffer");

    }
}
