/* CVS $Id: bread.c,v 1.5 2005/06/28 05:06:03 Michael Exp $ */
/* Copyright (c) Michael Lehotay, 2003. */
/* Bone Graft may be freely redistributed. See license.txt for details. */

/*
 * Portions of this file were derived from the following NetHack source files:
 * bones.c, Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985,1993.
 * engrave.c, Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985.
 * files.c, Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985.
 * light.c, Copyright (c) Dean Luick, 1994
 * makedefs.c, Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985.
 *     Copyright (c) M. Stephenson, 1990, 1991.
 *     Copyright (c) Dean Luick, 1990.
 * region.c, Copyright (c) 1996 by Jean-Christophe Collet
 * restore.c, Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985.
 * save.c, Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985.
 * timeout.c, Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985.
 * version.c, Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985.
 * worm.c, Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "graft.h"
#include "bones.h"
#include "file.h"

/* command line options */
boolean headeronly, forceversion;

/* info about the bones file */
boolean goldobj, invisibleobjects, nosignal, bigyou;
static boolean rlecomp;
bones_t bones;

/* some struct sizes */
static unsigned egdsz, eprisz, eshksz, eminsz, edogsz, monstsz;

/* private functions */
static void readmagicnumbers(void);
static void teststring(char *s, unsigned buflen);
static void readbonesid(void);
static void readfruit(void);
static void readlocation(struct rm *r);
static void readmap(void);
static void readdlevel(struct_t *parent, d_level *d);
static void readcoord(struct_t *parent, coord *c);
static void readstairs(stairway *s);
static void readdest(dest_area *d);
static void readdoors(void);
static struct mkroom *readroom(void);
static void freeroom(struct mkroom *rm);
static void readrooms(void);
static void readtimers(void);
static void readlights(void);
static void readmonsters(void);
static struct monst *readmonster(int32 xl, boolean readobjects);
static void freemonsters(struct monst *m);
static void structsizes(void);
static struct egd *readguard(void);
static struct epri *readpriest(void);
static struct eshk *readshopkeeper(void);
static struct emin *readminion(void);
static struct edog *readdog(void);
static struct obj *readobjects(boolean counting);
static void freeobjects(struct obj *obj);
static void readworms(void);
static void readtraps(void);
static void readengravings(void);
static void readdamage(void);
static void readregions(void);


/*****************************************************************************
 * readbones                                                                 *
 *****************************************************************************
 * read the whole bones file
 */
void readbones(char *fname) {
    bopen(fname);
    readmagicnumbers();

    if(headeronly) {
	bclose();
	return;
    }

    structsizes();
    readbonesid();
    readfruit();
    readmap();

    if(bones.header.incarnation >= 0x03030000) {
	readdoors();
	readrooms();
    }

    readtimers();
    readlights();
    readmonsters();
    readworms();
    readtraps();
    bones.objchn = readobjects(FALSE);
    bones.buriedobjs = readobjects(FALSE);
    bones.billobjs = readobjects(FALSE);
    readengravings();

    if(bones.header.incarnation < 0x03030000) {
	readrooms();
	readdoors();
    }

    readdamage();

    if(bones.header.incarnation >= 0x03030000)
	readregions();

    testeof();
    bclose();
}


/*****************************************************************************
 * freebones                                                                 *
 *****************************************************************************
 * free memory used by the bones data
 */
void freebones(void) {
    int i;

    /* free the bones id */
    if(bones.bonesid != NULL) {
	free(bones.bonesid);
	bones.bonesid = NULL;
    }

    /* free fruit */
    while(bones.fruitchain != NULL) {
	struct fruit *p = bones.fruitchain->nextf;
	free(bones.fruitchain);
	bones.fruitchain = p;
    }

    /* free rooms */
    for(i=0; i<bones.nroom; i++) {
	if(bones.rooms[i] != NULL) {
	    freeroom(bones.rooms[i]);
	    bones.rooms[i] = NULL;
	}
    }

    /* free timers */
    while(bones.timer_base != NULL) {
	timer_element *p = bones.timer_base->next;
	free(bones.timer_base);
	bones.timer_base = p;
    }

    /* free light sources */
    while(bones.light_base != NULL) {
	struct ls_t *p = bones.light_base->next;
	free(bones.light_base);
	bones.light_base = p;
    }

    /* free monsters */
    if(bones.monchn != NULL) {
	freemonsters(bones.monchn);
	bones.monchn = NULL;
    }

    /* free worms */
    for(i=0; i<MAX_NUM_WORMS; i++) {
	while(bones.wsegments[i] != NULL) {
	    struct wseg *p = bones.wsegments[i]->nseg;
	    free(bones.wsegments[i]);
	    bones.wsegments[i] = p;
	}
    }

    /* free traps */
    while(bones.trapchn != NULL) {
	struct trap *p = bones.trapchn->ntrap;
	free(bones.trapchn);
	bones.trapchn = p;
    }

    /* free objects */
    if(bones.objchn != NULL) {
	freeobjects(bones.objchn);
	bones.objchn = NULL;
    }
    if(bones.buriedobjs != NULL) {
	freeobjects(bones.buriedobjs);
	bones.buriedobjs = NULL;
    }
    if(bones.billobjs != NULL) {
	freeobjects(bones.billobjs);
	bones.billobjs = NULL;
    }

    /* free engravings */
    while(bones.engravings != NULL) {
	struct engr *p = bones.engravings->nxt_engr;
	if(bones.engravings->engr_txt != NULL)
	    free(bones.engravings->engr_txt);
	free(bones.engravings);
	bones.engravings = p;
    }

    /* free damage */
    while(bones.damage != NULL) {
	struct damage *p = bones.damage->next;
	free(bones.damage);
	bones.damage = p;
    }

    /* free regions */
    if(bones.regions != NULL) {
	for(i=0; i<bones.nregions; i++) {
	    if(bones.regions[i] != NULL) {
		if(bones.regions[i]->rects != NULL)
		    free(bones.regions[i]->rects);
		if(bones.regions[i]->enter_msg != NULL)
		    free(bones.regions[i]->enter_msg);
		if(bones.regions[i]->leave_msg != NULL)
		    free(bones.regions[i]->leave_msg);
		if(bones.regions[i]->monsters != NULL)
		    free(bones.regions[i]->monsters);
		free(bones.regions[i]);
	    }
	}
	free(bones.regions);
	bones.regions = NULL;
    }
}


/*****************************************************************************
 * readmagicnumbers                                                          *
 *****************************************************************************
 * read bones file header, check for recognized version of file format
 */
static void readmagicnumbers(void) {
    struct_t *st = startstruct(NULL, FALSE, LONGSZ);

    iread(&bones.header.incarnation, sizeof(bones.header.incarnation), LONGSZ, st);
    bones.vmajor = (uint8) ((bones.header.incarnation>>24) & 0xFF);
    bones.vminor = (uint8) ((bones.header.incarnation>>16) & 0xFF);
    bones.vpatch = (uint8) ((bones.header.incarnation>>8) & 0xFF);
    bones.vedit = (uint8) (bones.header.incarnation & 0xFF);

    /* Bones didn't have file headers before version 3.2.0. Test for this. */
    if(!headeronly && bones.header.incarnation < 0x03020000)
	bail(SEMANTIC_ERROR, "bones file pretending to be from version "
	    "%d.%d.%d", bones.vmajor, bones.vminor, bones.vpatch);

    /* don't process unsupported/untested versions unless explicitly told to */
    if(!headeronly && !forceversion && bones.header.incarnation>LATEST_VERSION)
	bail(SEMANTIC_ERROR, "bones file is from the future");

    /* Some (all?) of the features from the "flag bits" category (bits 15-26)
     * don't affect bones. Maybe allow them to be arbitrarily set or cleared
     * when converting bones? Read the source and think about it. */
    iread(&bones.header.feature_set, sizeof(bones.header.feature_set), LONGSZ, st);

    iread(&bones.header.entity_count, sizeof(bones.header.entity_count), LONGSZ, st);
    bones.nmonsters = (uint16) (bones.header.entity_count & 0xFFF);
    bones.nobjects = (uint16) ((bones.header.entity_count>>12) & 0xFFF);
    bones.nartifacts = (uint8) ((bones.header.entity_count>>24) & 0xFF);

    /* file header contains struct sizes since version 3.2.1 */
    if(bones.header.incarnation >= 0x03020100) {
	iread(&bones.header.struct_sizes, sizeof(bones.header.struct_sizes), LONGSZ, st);
	if(bigyou) {
	    bones.yousz = (uint16) (bones.header.struct_sizes & 0x7FF);
	    bones.monstsz = (uint8) ((bones.header.struct_sizes>>10) & 0x7F);
	    /* since struct you is so big, structs are probably not packed,
	     * it's unlikely that struct monst would be aligned on odd bytes */
	    bones.monstsz &= ~0x01; /* clear the lowest bit */
	} else {
	    bones.yousz = (uint16) (bones.header.struct_sizes & 0x3FF);
	    bones.monstsz = (uint8) ((bones.header.struct_sizes>>10) & 0x7F);
	}
	bones.objsz = (uint8) ((bones.header.struct_sizes>>17) & 0x7F);
	bones.flagsz = (uint8) ((bones.header.struct_sizes>>24) & 0xFF);
    }

    endstruct(st);

    /* remember the important options */
    goldobj = (bones.header.incarnation >= 0x03040000) &&
	((bones.header.feature_set & (1<<12)) != 0);
    zerocomp = (bones.header.feature_set & (1<<27)) != 0;
    rlecomp = (bones.header.feature_set & (1<<28)) != 0;
}


/*****************************************************************************
 * teststring                                                                *
 *****************************************************************************
 * makes sure a string terminated properly
 */
static void teststring(char *s, unsigned buflen) {
    unsigned i;

    assert(s != NULL);

    for(i=0; i<buflen && s[i]!='\0'; i++) {
	/* don't check for isprint(), what about 8 bit pet names? */
	/* if(!isprint(s[i]))
	    bail(SEMANTIC_ERROR, "nonprintable characters in string");
	 */
    }

    if(i == buflen)
	bail(SEMANTIC_ERROR, "unterminated string");
}


/*****************************************************************************
 * readbonesid                                                               *
 *****************************************************************************
 *  read bones id indicating the dungeon branch and level
 */
static void readbonesid(void) {
    uint8 len;

    iread(&len, sizeof(len), CHARSZ, NULL);
    if(len>10)
	bail(SEMANTIC_ERROR, "bonesid is too long");
    bones.bonesid = alloc(len);
    zread(bones.bonesid, 1, len, NULL);
    teststring(bones.bonesid, 10);
}


/*****************************************************************************
 * readfruit                                                                 *
 *****************************************************************************
 * read the names of all the fruits in the bones file
 */
static void readfruit(void) {
    struct fruit *f, *tail = NULL;

    do { /* keep reading structs until we find the sentinel with fid=0 */
	struct_t *st;
	uint32 grot;

	/* allocate a struct fruit and add it to the fruit chain */
	f = alloc(sizeof(struct fruit));
	f->nextf = NULL;
	if(bones.fruitchain == NULL)
	    bones.fruitchain = tail = f;
	else {
	    tail->nextf = f;
	    tail = f;
	}

	/* read fruit from file */
	st = startstruct(NULL, FALSE, MAXSZ);
	zread(f->fname, 1, PL_FSIZ, st);
	teststring(f->fname, PL_FSIZ);
	iread(&f->fid, sizeof(f->fid), intsz, st);
	zread(&grot, pointersz, 1, st); /* f->nextf */
	endstruct(st);
    } while(f->fid);
}


/*****************************************************************************
 * readlocation                                                              *
 *****************************************************************************
 * read a struct rm for the locations array
 */
static void readlocation(struct rm *r) {
    struct_t *st;

    assert(r!=NULL);
    st = startstruct(NULL, TRUE, intsz);
    zread(&r->glyph, intsz, 1, st);
    zread(&r->typ, CHARSZ, 1, st);
    /* todo: verify typ is within bounds */
    zread(&r->seenv, CHARSZ, 1, st);
    r->flags = bread(5, st);
    r->horizontal = bread(1, st);
    r->lit = bread(1, st);
    r->waslit = bread(1, st);
    r->roomno = bread(6, st);
    r->edge = bread(1, st);
    endstruct(st);
}


/*****************************************************************************
 * readmap                                                                   *
 *****************************************************************************
 * read the map of the dungeon terrain, stairs, levelflags, etc.
 */
static void readmap(void) {
    struct_t *st;
    int x, y;

    iread(&bones.hpid, sizeof(bones.hpid), intsz, NULL);
    iread(&bones.dlvl, sizeof(bones.dlvl), CHARSZ, NULL);/* ledger_no(&u.uz) */

    if(rlecomp) { /* run length encoding */
	uint8 len = 0; /* length of current run */
	struct rm r = {0};  /* initialized to make lint happy */

	for(y=0; y<ROWNO; y++) { /* note: rows are in the outer loop */
	    for(x=0; x<COLNO; x++) {
		if(!len) {
		    zread(&len, CHARSZ, 1, NULL); /* start a new run */
		    if(len > ROWNO*COLNO)
			bail(SEMANTIC_ERROR, "run length is too long");
		    readlocation(&r);
		}

		bones.locations[x][y] = r;
		len--;
	    }
	}

	if(len)
	    bail(SEMANTIC_ERROR, "RLE stops in middle of run");

    } else {
	for(x=0; x<COLNO; x++) { /* here columns are in the outer loop */
	    for(y=0; y<ROWNO; y++) {
		readlocation(&bones.locations[x][y]);
	    }
	}
    }

    iread(&bones.monstermoves, sizeof(bones.monstermoves), LONGSZ, NULL);
    readstairs(&bones.upstair);
    readstairs(&bones.dnstair);
    readstairs(&bones.upladder);
    readstairs(&bones.dnladder);
    readstairs(&bones.sstairs);
    readdest(&bones.updest);
    readdest(&bones.dndest);

    st = startstruct(NULL, TRUE, CHARSZ);
    iread(&bones.flags.nfountains, sizeof(bones.flags.nfountains), CHARSZ, st);
    iread(&bones.flags.nsinks, sizeof(bones.flags.nsinks), CHARSZ, st);
    bones.flags.has_shop = bread(1, st);
    bones.flags.has_vault = bread(1, st);
    bones.flags.has_zoo = bread(1, st);
    bones.flags.has_court = bread(1, st);
    bones.flags.has_morgue = bread(1, st);
    bones.flags.has_beehive = bread(1, st);
    bones.flags.has_barracks = bread(1, st);
    bones.flags.has_temple = bread(1, st);
    bones.flags.has_swamp = bread(1, st);
    bones.flags.noteleport = bread(1, st);
    bones.flags.hardfloor = bread(1, st);
    bones.flags.nommap = bread(1, st);
    bones.flags.hero_memory = bread(1, st);
    bones.flags.shortsighted = bread(1, st);
    bones.flags.graveyard = bread(1, st);
    bones.flags.is_maze_lev = bread(1, st);
    bones.flags.is_cavernous_lev = bread(1, st);
    if(bones.header.incarnation >= 0x03030000)
	bones.flags.arboreal = bread(1, st);
    endstruct(st);
}


/*****************************************************************************
 * readdlevel                                                                *
 *****************************************************************************
 * read a d_level struct from file
 */
static void readdlevel(struct_t *parent, d_level *d) {
    struct_t *st;

    assert(d!=NULL);
    st = startstruct(parent, FALSE, CHARSZ);
    iread(&d->dnum, sizeof(d->dnum), CHARSZ, st);
    iread(&d->dlevel, sizeof(d->dlevel), CHARSZ, st);
    endstruct(st);
}


/*****************************************************************************
 * readcoord                                                                 *
 *****************************************************************************
 * read a coord struct from file
 */
static void readcoord(struct_t *parent, coord *c) {
    struct_t *st;

    assert(c!=NULL);
    st = startstruct(parent, FALSE, CHARSZ);
    iread(&c->x, sizeof(c->x), CHARSZ, st);
    iread(&c->y, sizeof(c->y), CHARSZ, st);
    endstruct(st);
}


/*****************************************************************************
 * readstairs                                                                *
 *****************************************************************************
 * read info about a stairway
 */
static void readstairs(stairway *s) {
    struct_t *st;

    assert(s!=NULL);
    st = startstruct(NULL, FALSE, CHARSZ);
    iread(&s->sx, sizeof(s->sx), CHARSZ, st);
    iread(&s->sy, sizeof(s->sy), CHARSZ, st);
    readdlevel(st, &s->tolev);
    iread(&s->up, sizeof(s->up), CHARSZ, st);
    endstruct(st);
}


/*****************************************************************************
 * readdest                                                                  *
 *****************************************************************************
 * read info about a level change destination area
 */
static void readdest(dest_area *d) {
    struct_t *st;

    assert(d!=NULL);
    st = startstruct(NULL, FALSE, CHARSZ);
    iread(&d->lx, sizeof(d->lx), CHARSZ, st);
    iread(&d->ly, sizeof(d->ly), CHARSZ, st);
    iread(&d->hx, sizeof(d->hx), CHARSZ, st);
    iread(&d->hy, sizeof(d->hy), CHARSZ, st);
    iread(&d->nlx, sizeof(d->nlx), CHARSZ, st);
    iread(&d->nly, sizeof(d->nly), CHARSZ, st);
    iread(&d->nhx, sizeof(d->nhx), CHARSZ, st);
    iread(&d->nhy, sizeof(d->nhy), CHARSZ, st);
    endstruct(st);
}


/*****************************************************************************
 * readdoors                                                                 *
 *****************************************************************************
 * read coordinates of all the doors
 */
static void readdoors(void) {
    int i;

    for(i=0; i<DOORMAX; i++)
	readcoord(NULL, &bones.doors[i]);
}


/*****************************************************************************
 * readroom                                                                  *
 *****************************************************************************
 * read a struct mkroom from the file
 */
static struct mkroom *readroom(void) {
    struct_t *st;
    struct mkroom *r;
    uint32 grot;
    int i;

    r = alloc(sizeof(struct mkroom));
    r->resident = NULL;
    r->nsubrooms = 0;

    st = startstruct(NULL, FALSE, pointersz);
    iread(&r->lx, sizeof(r->lx), CHARSZ, st);
    iread(&r->hx, sizeof(r->hx), CHARSZ, st);
    iread(&r->ly, sizeof(r->ly), CHARSZ, st);
    iread(&r->hy, sizeof(r->hy), CHARSZ, st);
    iread(&r->rtype, sizeof(r->rtype), CHARSZ, st);
    iread(&r->rlit, sizeof(r->rlit), CHARSZ, st);
    iread(&r->doorct, sizeof(r->doorct), CHARSZ, st);
    iread(&r->fdoor, sizeof(r->fdoor), CHARSZ, st);
    if((r->fdoor + r->doorct) > DOORMAX)
	bail(SEMANTIC_ERROR, "too many doors");
    iread(&r->nsubrooms, sizeof(r->nsubrooms), CHARSZ, st);
    if(r->nsubrooms>MAX_SUBROOMS)
	bail(SEMANTIC_ERROR, "too many subrooms");
    iread(&r->irregular, sizeof(r->irregular), CHARSZ, st);

    /* pointer values from file are garbage */
    for(i=0; i<MAX_SUBROOMS; i++) {
	r->sbrooms[i] = NULL;
	zread(&grot, pointersz, 1, st);
    }
    zread(&grot, pointersz, 1, st); /* r->resident */
    endstruct(st);

    for(i=0; i<r->nsubrooms; i++)
	r->sbrooms[i] = readroom();

    return r;
}


/*****************************************************************************
 * freeroom                                                                  *
 *****************************************************************************
 * frees a struct mkroom and all of its subrooms
 */
static void freeroom(struct mkroom *rm) {
    int i;

    assert(rm!=NULL);
    for(i=0; i<rm->nsubrooms; i++)
	freeroom(rm->sbrooms[i]);
    free(rm);
}


/*****************************************************************************
 * readrooms                                                                 *
 *****************************************************************************
 * read all the rooms from the file
 */
static void readrooms(void) {
    int i;

    iread(&bones.nroom, sizeof(bones.nroom), intsz, NULL);
    if(bones.nroom > MAXNROFROOMS+1) { /* the +1 is for a vault, I think */
	bones.nroom = 0;
	bail(SEMANTIC_ERROR, "too many rooms");
    }

    for(i=0; i<bones.nroom; i++)
	bones.rooms[i] = readroom();
}


/*****************************************************************************
 * readtimers                                                                *
 *****************************************************************************
 * read timer info
 */
static void readtimers(void) {
    timer_element *te, *tail=NULL;
    struct_t *st;
    uint32 grot;
    int i;

    iread(&bones.ntimers, sizeof(bones.ntimers), intsz, NULL);
    for(i=0; i<bones.ntimers; i++) {
	te = alloc(sizeof(timer_element));
	te->next = te->arg = NULL;
	if(bones.timer_base==NULL)
	    bones.timer_base = tail = te;
	else {
	    tail->next = te;
	    tail = te;
	}

	st = startstruct(NULL, TRUE, LONGSZ);
	zread(&grot, pointersz, 1, st); /* te->next */
	iread(&te->timeout, sizeof(te->timeout), LONGSZ, st);
	iread(&te->tid, sizeof(te->tid), LONGSZ, st);
	iread(&te->kind, sizeof(te->kind), SHORTSZ, st);
	iread(&te->func_index, sizeof(te->func_index), SHORTSZ, st);
	zread(&grot, pointersz, 1, st); /* te->arg  */
	te->needs_fixup = bread(1, st);
	endstruct(st);

    }
}


/*****************************************************************************
 * readlights                                                                *
 *****************************************************************************
 * read light sources
 */
static void readlights(void) {
    light_source *ls, *tail=NULL;
    struct_t *st;
    uint32 grot;
    int i;

    iread(&bones.nlights, sizeof(bones.nlights), intsz, NULL);
    for(i=0; i<bones.nlights; i++) {
	ls = alloc(sizeof(light_source));
	ls->next = ls->id = NULL;
	if(bones.light_base==NULL)
	    bones.light_base = tail = ls;
	else {
	    tail->next = ls;
	    tail = ls;
	}

	st = startstruct(NULL, TRUE, pointersz);
	zread(&grot, pointersz, 1, st); /* ls->next */
	iread(&ls->x, sizeof(ls->x), CHARSZ, st);
	iread(&ls->y, sizeof(ls->y), CHARSZ, st);
	iread(&ls->range, sizeof(ls->range), SHORTSZ, st);
	iread(&ls->flags, sizeof(ls->flags), SHORTSZ, st);
	iread(&ls->type, sizeof(ls->type), SHORTSZ, st);
	zread(&grot, pointersz, 1, st); /* ls->id */
	endstruct(st);
    }
}


/*****************************************************************************
 * readmonsters                                                              *
 *****************************************************************************
 * read the monster chain
 */
static void readmonsters(void) {
    struct monst *m, *tail=NULL;
    int32 xl=0; /* number of extra bytes after struct monst */

    /* monbegin is a pointer, read it as an uint32 in case pointers have only
     * 16 bits on this machine. for each monster in the chain we have to
     * calculate m->data's offset from monbegin, which requires calculating
     * sizeof(struct permonst) from the bones data. */
    iread(&bones.monbegin, sizeof(bones.monbegin), pointersz, NULL);

    while(xl>=0) { /* non-constant expression to please lint */
	iread(&xl, sizeof(xl), intsz, NULL);
	if(xl == -1)
	    break;

	m = readmonster(xl, TRUE);
	if(bones.monchn==NULL)
	    bones.monchn = tail = m;
	else {
	    tail->nmon = m;
	    tail = m;
	}
    }
}


/*****************************************************************************
 * readmonster                                                               *
 *****************************************************************************
 * read a struct monst from file
 */
static struct monst *readmonster(int32 xl, boolean readinv) {
    struct monst *m;
    struct_t *st;
    boolean hasinventory = FALSE, hasweapon = FALSE;
    uint32 grot;
    int i;

    m = alloc(sizeof(struct monst));
    m->nmon = NULL;
    m->minvent = m->mw = NULL;
    m->egd = NULL;
    m->epri = NULL;
    m->eshk = NULL;
    m->emin = NULL;
    m->edog = NULL;
    m->ghostname = m->mname = NULL;

    st = startstruct(NULL, TRUE, LONGSZ);
    zread(&grot, pointersz, 1, st); /* m->nmon */
    iread(&m->data, sizeof(m->data), pointersz, st);
    iread(&m->m_id, sizeof(m->m_id), intsz, st);
    iread(&m->mnum, sizeof(m->mnum), SHORTSZ, st);

    if(bones.header.incarnation >= 0x03030000)
	iread(&m->movement, sizeof(m->movement), SHORTSZ, st);

    iread(&m->m_lev, sizeof(m->m_lev), CHARSZ, st);
    iread(&m->malign, sizeof(m->malign), CHARSZ, st);
    iread(&m->mx, sizeof(m->mx), CHARSZ, st);
    iread(&m->my, sizeof(m->my), CHARSZ, st);
    iread(&m->mux, sizeof(m->mux), CHARSZ, st);
    iread(&m->muy, sizeof(m->muy), CHARSZ, st);

    for(i=0; i<MTSZ; i++)
	readcoord(st, &m->mtrack[i]);

    iread(&m->mhp, sizeof(m->mhp), intsz, st);
    iread(&m->mhpmax, sizeof(m->mhpmax), intsz, st);
    iread(&m->mappearance, sizeof(m->mappearance), intsz, st);
    iread(&m->m_ap_type, sizeof(m->m_ap_type), CHARSZ, st);
    iread(&m->mtame, sizeof(m->mtame), CHARSZ, st);
    iread(&m->mintrinsics, sizeof(m->mintrinsics), SHORTSZ, st);
    iread(&m->mspec_used, sizeof(m->mspec_used), intsz, st);
    m->female = bread(1, st);
    m->minvis = bread(1, st);
    m->invis_blkd = bread(1, st);
    m->perminvis = bread(1, st);

    if(bones.header.incarnation < 0x03030000)
	m->cham = bread(1, st);
    else
	m->cham = bread(3, st);

    m->mundetected = bread(1, st);
    m->mcan = bread(1, st);
    m->mburied = bread(1, st);
    m->mspeed = bread(2, st);
    m->permspeed = bread(2, st);

    if(bones.header.incarnation >= 0x03030000) {
	m->mrevived = bread(1, st);
	m->mavenge = bread(1, st); /* was called "not_used" before 3.4.0 */
    }

    m->mflee = bread(1, st);
    m->mfleetim = bread(7, st);
    m->mcansee = bread(1, st);
    m->mblinded = bread(7, st);
    m->mcanmove = bread(1, st);
    m->mfrozen = bread(7, st);
    m->msleeping = bread(1, st); /* was called "msleep" before 3.4.0 */
    m->mstun = bread(1, st);
    m->mconf = bread(1, st);
    m->mpeaceful = bread(1, st);
    m->mtrapped = bread(1, st);
    m->mleashed = bread(1, st);
    m->isshk = bread(1, st);
    m->isminion = bread(1, st);
    m->isgd = bread(1, st);
    m->ispriest = bread(1, st);
    m->iswiz = bread(1, st);
    m->wormno = bread(5, st);
    iread(&m->mstrategy, sizeof(m->mstrategy), LONGSZ, st);
    iread(&m->mtrapseen, sizeof(m->mtrapseen), LONGSZ, st);
    iread(&m->mlstmv, sizeof(m->mlstmv), LONGSZ, st);

     /* GOLDOBJ feature first appeared in 3.4.0 */
    if(!goldobj || bones.header.incarnation < 0x03040000)
	iread(&m->mgold, sizeof(m->mgold), LONGSZ, st);

    zread(&grot, pointersz, 1, st); /* m->minvent */
    if(grot)
	hasinventory = TRUE;
    zread(&grot, pointersz, 1, st); /* m->mw */
    if(grot)
	hasweapon = TRUE;

    iread(&m->misc_worn_check, sizeof(m->misc_worn_check), LONGSZ, st);
    iread(&m->weapon_check, sizeof(m->weapon_check), CHARSZ, st);
    iread(&m->mnamelth, sizeof(m->mnamelth), CHARSZ, st);
    iread(&m->mxlth, sizeof(m->mxlth), SHORTSZ, st);

    if(m->mnamelth+m->mxlth != xl)
	bail(SEMANTIC_ERROR, "bogus mextra byte count");

    iread(&m->meating, sizeof(m->meating), intsz, st);
    align(st, LONGSZ); /* align for m->mextra, but it's not really here */

    /* In the nethack headers, mextra structs and mname are not members of
     * struct monst. They are appended to it and addressed using pointers to
     * m->mextra[0]. We will make them proper members. */

    /* read mextra info */
    if(m->mxlth) {
	assert(egdsz>0);
	assert(eprisz>0);
	assert(eshksz>0);
	assert(eminsz>0);
	assert(edogsz>0);

	if((unsigned) m->mxlth == egdsz) /* m->isgd */
	    m->egd = readguard();
	else if((unsigned) m->mxlth == eprisz) /* m->ispriest */
	    m->epri = readpriest();
	else if((unsigned) m->mxlth == eshksz) /* m->isshk */
	    m->eshk = readshopkeeper();
	else if((unsigned) m->mxlth == eminsz) /* m->isminion */
	    m->emin = readminion();
	else if((unsigned) m->mxlth == edogsz) /* m->mtame */
	    m->edog = readdog();
	else if(m->mxlth==PL_NSIZ && bones.header.incarnation<0x03030000) {
	    m->ghostname = alloc(PL_NSIZ);
	    zread(m->ghostname, 1, PL_NSIZ, NULL);
	    teststring(m->ghostname, PL_NSIZ);
	} else
	    bail(SEMANTIC_ERROR, "unexpected mxlth");
    }

    /* read monster's name */
    if(m->mnamelth) {
	m->mname = alloc(m->mnamelth);
	zread(m->mname, 1, m->mnamelth, NULL);
	teststring(m->mname, m->mnamelth);
    }

    /* read in mextra, value is unimportant, it was just a placeholder */
    iread(&m->mextra[0], sizeof(m->mextra[0]), LONGSZ, st);
    endstruct(st);

    if(hasinventory && readinv)
	m->minvent = readobjects(FALSE);
    if(hasweapon && readinv)
	xl = xl; /* lookup pointer from object chain */

    return m;
}


/*****************************************************************************
 * freemonsters                                                              *
 *****************************************************************************
 * free a list of struct monsts
 */
static void freemonsters(struct monst *m) {
    assert(m!=NULL);

    if(m->egd != NULL)
	free(m->egd);
    if(m->epri != NULL)
	free(m->epri);
    if(m->eshk != NULL)
	free(m->eshk);
    if(m->emin != NULL)
	free(m->emin);
    if(m->edog != NULL)
	free(m->edog);
    if(m->mname != NULL)
	free(m->mname);
    if(m->ghostname != NULL)
	free(m->ghostname);
    if(m->minvent != NULL)
	freeobjects(m->minvent);
    if(m->nmon != NULL)
	freemonsters(m->nmon);
    free(m);
}


/*****************************************************************************
 * structsizes                                                               *
 *****************************************************************************
 * calculate the size of some important structs
 */
static void structsizes(void) {
    coord c;
    void *p;

    /* sanity check for union in struct trap */
    startcount();
    readcoord(NULL, &c);
    assert(sizeof(int16) == getcount());

    startcount();
    p = readguard();
    egdsz = getcount();
    free(p);

    startcount();
    p = readpriest();
    eprisz = getcount();
    free(p);

    startcount();
    p = readshopkeeper();
    eshksz = getcount();
    free(p);

    startcount();
    p = readminion();
    eminsz = getcount();
    free(p);

    startcount();
    p = readdog();
    edogsz = getcount();
    free(p);

    startcount();
    p = readmonster(0, FALSE);
    monstsz = getcount();
    freemonsters(p);

    if(bones.header.incarnation >= 0x03020100) {
	unsigned objsz;

	if(monstsz != bones.monstsz)
	    bail(SEMANTIC_ERROR, "struct monst is wrong size");

	startcount();
	p = readobjects(TRUE);
	objsz = getcount();
	freeobjects(p);
	if(objsz != bones.objsz)
	    bail(SEMANTIC_ERROR, "struct obj is wrong size");
    }
}


/*****************************************************************************
 * readguard                                                                 *
 *****************************************************************************
 * read extra monster info for vault guards
 */
static struct egd *readguard(void) {
    struct egd *egd;
    struct_t *st;
    int i;

    egd = alloc(sizeof(struct egd));
    st = startstruct(NULL, TRUE, intsz);
    iread(&egd->fcbeg, sizeof(egd->fcbeg), intsz, st);
    iread(&egd->fcend, sizeof(egd->fcend), intsz, st);

    if(bones.header.incarnation >= 0x03020100)
	iread(&egd->vroom, sizeof(egd->vroom), intsz, st);

    iread(&egd->gdx, sizeof(egd->gdx), CHARSZ, st);
    iread(&egd->gdy, sizeof(egd->gdy), CHARSZ, st);
    iread(&egd->ogx, sizeof(egd->ogx), CHARSZ, st);
    iread(&egd->ogy, sizeof(egd->ogy), CHARSZ, st);
    readdlevel(st, &egd->gdlevel);
    iread(&egd->warncnt, sizeof(egd->warncnt), CHARSZ, st);

    if(bones.header.incarnation < 0x03020100)
	iread(&egd->vroom, sizeof(egd->vroom), intsz, st);

    egd->gddone = bread(1, st);
    egd->unused = bread(7, st);
    for(i=0; i<FCSIZ; i++) {
	struct_t *st2 = startstruct(st, FALSE, CHARSZ);
	iread(&egd->fakecorr[i].fx, sizeof(egd->fakecorr[i].fx), CHARSZ, st2);
	iread(&egd->fakecorr[i].fy, sizeof(egd->fakecorr[i].fy), CHARSZ, st2);
	iread(&egd->fakecorr[i].ftyp,sizeof(egd->fakecorr[i].ftyp),CHARSZ,st2);
	endstruct(st2);
    }
    endstruct(st);

    return egd;
}


/*****************************************************************************
 * readpriest                                                                *
 *****************************************************************************
 * read extra monster info for temple priests
 */
static struct epri *readpriest(void) {
    struct epri *epri;
    struct_t *st;

    epri = alloc(sizeof(struct epri));
    st = startstruct(NULL, FALSE, CHARSZ);
    iread(&epri->shralign, sizeof(epri->shralign), CHARSZ, st);
    iread(&epri->shroom, sizeof(epri->shroom), CHARSZ, st);
    readcoord(st, &epri->shrpos);
    readdlevel(st, &epri->shrlevel);
    endstruct(st);

    return epri;
}


/*****************************************************************************
 * readshopkeeper                                                            *
 *****************************************************************************
 * read extra monster info for shopkeepers
 */
static struct eshk *readshopkeeper(void) {
    struct eshk *eshk;
    struct_t *st, *st2;
    int32 grot;
    int i;

    eshk = alloc(sizeof(struct eshk));
    eshk->bill_p = NULL;

    st = startstruct(NULL, FALSE, LONGSZ);
    iread(&eshk->robbed, sizeof(eshk->robbed), LONGSZ, st);
    iread(&eshk->credit, sizeof(eshk->credit), LONGSZ, st);
    iread(&eshk->debit, sizeof(eshk->debit), LONGSZ, st);
    iread(&eshk->loan, sizeof(eshk->loan), LONGSZ, st);
    iread(&eshk->shoptype, sizeof(eshk->shoptype), intsz, st);
    iread(&eshk->shoproom, sizeof(eshk->shoproom), CHARSZ, st);

    if(bones.header.incarnation >= 0x03020100)
	iread(&eshk->unused, sizeof(eshk->unused), CHARSZ, st);

    iread(&eshk->following, sizeof(eshk->following), CHARSZ, st);
    iread(&eshk->surcharge, sizeof(eshk->surcharge), CHARSZ, st);
    readcoord(st, &eshk->shk);
    readcoord(st, &eshk->shd);
    readdlevel(st, &eshk->shoplevel);
    iread(&eshk->billct, sizeof(eshk->billct), intsz, st);
    for(i=0; i<BILLSZ; i++) {
	st2 = startstruct(st, FALSE, LONGSZ);
	iread(&eshk->bill[i].bo_id, sizeof(eshk->bill[i].bo_id), intsz, st2);
	iread(&eshk->bill[i].useup, sizeof(eshk->bill[i].useup), CHARSZ, st2);
	iread(&eshk->bill[i].price, sizeof(eshk->bill[i].price), LONGSZ, st2);
	iread(&eshk->bill[i].bquan, sizeof(eshk->bill[i].bquan), LONGSZ, st2);
	endstruct(st2);
    }
    /* Bizarrely, sometimes bill_p is set to -1000. Need to figure out why. */
    iread(&grot, sizeof(grot), pointersz, st); /* eshk->bill_p */
    iread(&eshk->visitct, sizeof(eshk->visitct), intsz, st);
    zread(eshk->customer, CHARSZ, PL_NSIZ, st);
    teststring(eshk->customer, PL_NSIZ);
    zread(eshk->shknam, CHARSZ, PL_NSIZ, st);
    teststring(eshk->shknam, PL_NSIZ);
    endstruct(st);

    return eshk;
}


/*****************************************************************************
 * readminion                                                                *
 *****************************************************************************
 * read extra monster info for minions (guardian angels)
 */
static struct emin *readminion(void) {
    struct emin *emin;
    struct_t *st;

    emin = alloc(sizeof(struct emin));
    st = startstruct(NULL, FALSE, CHARSZ);
    iread(&emin->min_align, sizeof(emin->min_align), CHARSZ, st);
    endstruct(st);

    return emin;
}


/*****************************************************************************
 * readdog                                                                   *
 *****************************************************************************
 * read extra monster info for dogs & other pets
 */
static struct edog *readdog(void) {
    struct edog *edog;
    struct_t *st;

    edog = alloc(sizeof(struct edog));
    st = startstruct(NULL, TRUE, LONGSZ);
    iread(&edog->droptime, sizeof(edog->droptime), LONGSZ, st);
    iread(&edog->dropdist, sizeof(edog->dropdist), intsz, st);
    iread(&edog->apport, sizeof(edog->apport), intsz, st);
    iread(&edog->whistletime, sizeof(edog->whistletime), LONGSZ, st);
    iread(&edog->hungrytime, sizeof(edog->hungrytime), LONGSZ, st);
    readcoord(st, &edog->ogoal);

    if(bones.header.incarnation >= 0x03030000) {
	iread(&edog->abuse, sizeof(edog->abuse), intsz, st);
	iread(&edog->revivals, sizeof(edog->revivals), intsz, st);
	if(bones.header.incarnation >= 0x03030100)
	    iread(&edog->mhpmax_penalty, sizeof(edog->mhpmax_penalty), intsz, st);
	edog->killed_by_u = bread(1, st);
    }
    endstruct(st);

    return edog;
}


/*****************************************************************************
 * readobjects                                                               *
 *****************************************************************************
 * read a list of objects from file
 */
static struct obj *readobjects(boolean counting) {
    struct obj *obj, *head=NULL, *tail=NULL;
    int32 xl=0; /* number of extra bytes after struct obj */

    while(xl>=0) { /* non-constant expression to please lint */
	struct_t *st;
	boolean hascontents = FALSE;
	uint32 grot;

	iread(&xl, sizeof(xl), intsz, NULL);
	if(xl == -1)
	    break;

	obj = alloc(sizeof(struct obj));
	obj->nobj = obj->vptr = obj->cobj = NULL;
	obj->monst = NULL;
	obj->oname = NULL;
	if(head==NULL)
	    head = tail = obj;
	else {
	    tail->nobj = obj;
	    tail = obj;
	}

	st = startstruct(NULL, TRUE, LONGSZ);
	zread(&grot, pointersz, 1, st); /* obj->nobj */
	zread(&grot, pointersz, 1, st); /* obj->vptr */
	zread(&grot, pointersz, 1, st); /* obj->cobj */
	if(grot)
	    hascontents = TRUE;
	iread(&obj->o_id, sizeof(obj->o_id), intsz, st);
	iread(&obj->ox, sizeof(obj->ox), CHARSZ, st);
	iread(&obj->oy, sizeof(obj->oy), CHARSZ, st);
	iread(&obj->otyp, sizeof(obj->otyp), SHORTSZ, st);
	iread(&obj->owt, sizeof(obj->owt), intsz, st);
	iread(&obj->quan, sizeof(obj->quan), LONGSZ, st);
	iread(&obj->spe, sizeof(obj->spe), CHARSZ, st);
	iread(&obj->oclass, sizeof(obj->oclass), CHARSZ, st);
	iread(&obj->invlet, sizeof(obj->invlet), CHARSZ, st);
	iread(&obj->oartifact, sizeof(obj->oartifact), CHARSZ, st);
	iread(&obj->where, sizeof(obj->where), CHARSZ, st);
	iread(&obj->timed, sizeof(obj->timed), CHARSZ, st);
	obj->cursed = bread(1, st);
	obj->blessed = bread(1, st);
	obj->unpaid = bread(1, st);
	obj->no_charge = bread(1, st);
	obj->known = bread(1, st);
	obj->dknown = bread(1, st);
	obj->bknown = bread(1, st);
	obj->rknown = bread(1, st);
	obj->oeroded = bread(2, st);

	if(bones.header.incarnation >= 0x03030000)
	    obj->oeroded2 = bread(2, st);

	obj->oerodeproof = bread(1, st);
	obj->olocked = bread(1, st);
	obj->obroken = bread(1, st);
	obj->otrapped = bread(1, st);

	if(!nosignal && bones.header.incarnation < 0x03030000)
	    obj->in_use = bread(1, st);

	if(bones.header.incarnation >= 0x03030000)
	    obj->recharged = bread(3, st);

	obj->lamplit = bread(1, st);

	if(invisibleobjects || bones.header.incarnation < 0x03030000)
	    obj->oinvis = bread(1, st);

	obj->greased = bread(1, st);

	if(bones.header.incarnation >= 0x03030000)
	    obj->oattached = bread(2, st);
	else if(bones.header.incarnation >= 0x03020100)
	    obj->oattached = bread(1, st); /* was called "mtraits" */
	else
	    obj->onamelth = (uint8) bread(6, st);

	if(bones.header.incarnation >= 0x03030000)
	    obj->in_use = bread(1, st);

	if(bones.header.incarnation >= 0x03040000)
	    obj->bypass = bread(1, st);

	iread(&obj->corpsenm, sizeof(obj->corpsenm), intsz, st);
	iread(&obj->oeaten, sizeof(obj->oeaten), intsz, st);
	iread(&obj->age, sizeof(obj->age), LONGSZ, st);

	if(bones.header.incarnation >= 0x03020100) {
	    iread(&obj->onamelth, sizeof(obj->onamelth), CHARSZ, st);
	    iread(&obj->oxlth, sizeof(obj->oxlth), SHORTSZ, st);
	}

	if(obj->onamelth+obj->oxlth != xl)
	    bail(SEMANTIC_ERROR, "bogus oextra byte count");

	iread(&obj->owornmask, sizeof(obj->owornmask), LONGSZ, st);
	align(st, LONGSZ); /* align for oextra, but it's not really here */

	/* read oextra data */
	if(obj->oxlth) {
	    assert(monstsz>0);
	    if((unsigned) obj->oxlth == intsz) /* OATTACHED_M_ID */
		iread(&obj->m_id, sizeof(obj->m_id), intsz, NULL);
	    else if((unsigned) obj->oxlth >= monstsz) { /* OATTACHED_MONST */
		obj->monst = readmonster(obj->oxlth - monstsz, FALSE);
		/* clear possible bogus pointers */
		obj->monst->minvent = obj->monst->mw = NULL;
	    } else
		bail(SEMANTIC_ERROR, "unexpected oxlth");
	}

	/* read name */
	if(obj->onamelth) {
	    obj->oname = alloc(obj->onamelth);
	    zread(obj->oname, 1, obj->onamelth, NULL);
	    teststring(obj->oname, obj->onamelth);
	}

	/* read in oextra, value is unimportant, it was just a placeholder */
	iread(&obj->oextra[0], sizeof(obj->oextra[0]), LONGSZ, st);
	endstruct(st);

	if(hascontents)
	    obj->cobj = readobjects(FALSE);

	if(counting)
	    break;
    }

    return head;
}


/*****************************************************************************
 * freeobjects                                                               *
 *****************************************************************************
 * free a list of struct objs
 */
static void freeobjects(struct obj *obj) {
    assert(obj != NULL);
    if(obj->monst != NULL)
	freemonsters(obj->monst);
    if(obj->oname !=NULL)
	free(obj->oname);
    if(obj->cobj != NULL)
	freeobjects(obj->cobj);
    if(obj->nobj != NULL)
	freeobjects(obj->nobj);
    free(obj);
}


/*****************************************************************************
 * readworms                                                                 *
 *****************************************************************************
 * read longworm segment info from file
 */
static void readworms(void) {
    unsigned i, nsegs;
    struct wseg *wseg, *tail = NULL;

    bones.wsegments[0] = NULL; /* set to NULL just to be tidy */
    for(i=1; i<MAX_NUM_WORMS; i++) { /* wsegments[0] is unused */
	bones.wsegments[i] = NULL;

	iread(&nsegs, sizeof(nsegs), intsz, NULL);
	while(nsegs--) {
	    wseg = alloc(sizeof(struct wseg));
	    wseg->nseg = NULL;
	    if(bones.wsegments[i] == NULL)
		bones.wsegments[i] = tail = wseg;
	    else {
		tail->nseg = wseg;
		tail = wseg;
	    }

	    iread(&wseg->wx, sizeof(wseg->wx), CHARSZ, NULL);
	    iread(&wseg->wy, sizeof(wseg->wy), CHARSZ, NULL);
	}
    }

    for(i=0; i<MAX_NUM_WORMS; i++) /* read whole array, even wgrowtime[0] */
	iread(&bones.wgrowtime[i], sizeof(bones.wgrowtime[i]), LONGSZ, NULL);
}


/*****************************************************************************
 * readtraps                                                                 *
 *****************************************************************************
 * read traps
 */
static void readtraps(void) {
    struct trap *t, *tail=NULL;
    struct_t *st;
    uint32 grot;

    do { /* read traps until a we find struct filled with zeros */
	t = alloc(sizeof(struct trap));
	t->ntrap = NULL;

	st = startstruct(NULL, TRUE, pointersz);
	zread(&grot, pointersz, 1, st); /* t->ntrap */
	iread(&t->tx, sizeof(t->tx), CHARSZ, st);
	iread(&t->ty, sizeof(t->tx), CHARSZ, st);
	readdlevel(st, &t->dst);
	readcoord(st, &t->launch);
	t->ttyp = bread(5, st);
	t->tseen = bread(1, st);
	t->once = bread(1, st);
	t->madeby_u = bread(1, st);

	/* launch2 and launch_otyp are the same size, see structsizes() */
	if(t->ttyp == ROLLING_BOULDER_TRAP)
	    readcoord(st, &t->launch2);
	else
	    iread(&t->launch_otyp, sizeof(t->launch_otyp), SHORTSZ, st);
	endstruct(st);

	if(t->tx) { /* not the sentinel, add it to the list */
	    if(bones.trapchn==NULL)
		bones.trapchn = tail = t;
	    else {
		tail->ntrap = t;
		tail = t;
	    }
	}
    } while(t->tx);

    free(t); /* free the sentinel struct */
}


/*****************************************************************************
 * readengravings                                                            *
 *****************************************************************************
 * read engravings
 */
static void readengravings(void) {
    struct engr *e, *tail=NULL;
    struct_t *st;
    uint32 grot, len=0;

    while(len>=0) { /* non-constant expression to please lint */
	iread(&len, sizeof(len), intsz, NULL);
	if(len == 0)
	    break;

	if(len > BUFSZ)
	    bail(SEMANTIC_ERROR, "engraving is too long");

	e = alloc(sizeof(struct engr));
	e->nxt_engr = NULL;
	e->engr_txt = NULL;
	if(bones.engravings==NULL)
	    bones.engravings = tail = e;
	else {
	    tail->nxt_engr = e;
	    tail = e;
	}

	st = startstruct(NULL, FALSE, LONGSZ);
	zread(&grot, pointersz, 1, st); /* e->nxt_engr */
	zread(&grot, pointersz, 1, st); /* e->engr_txt */
	iread(&e->engr_x, sizeof(e->engr_x), CHARSZ, st);
	iread(&e->engr_y, sizeof(e->engr_y), CHARSZ, st);
	iread(&e->engr_lth, sizeof(e->engr_lth), intsz, st);
	if(e->engr_lth != len)
	    bail(SEMANTIC_ERROR, "bogus engr_lth");
	iread(&e->engr_time, sizeof(e->engr_time), LONGSZ, st);
	iread(&e->engr_type, sizeof(e->engr_type), CHARSZ, st);
	endstruct(st);

	/* read engraving text */
	e->engr_txt = alloc(e->engr_lth);
	zread(e->engr_txt, 1, e->engr_lth, NULL);
	teststring(e->engr_txt, e->engr_lth);
    }
}


/*****************************************************************************
 * readdamage                                                                *
 *****************************************************************************
 * read damage to dungeon level
 */
static void readdamage(void) {
    struct damage *d, *tail=NULL;
    struct_t *st;
    uint32 n, grot;

    iread(&n, sizeof(n), intsz, NULL);
    while(n--) {
	d = alloc(sizeof(struct damage));
	d->next = NULL;
	if(bones.damage==NULL)
	    bones.damage = tail = d;
	else {
	    tail->next = d;
	    tail = d;
	}

	st = startstruct(NULL, FALSE, LONGSZ);
	zread(&grot, pointersz, 1, st); /* d->next */
	iread(&d->when, sizeof(d->when), LONGSZ, st);
	iread(&d->cost, sizeof(d->cost), LONGSZ, st);
	readcoord(st, &d->place);
	iread(&d->typ, sizeof(d->typ), CHARSZ, st);
	endstruct(st);

    }
}


/*****************************************************************************
 * readregions                                                               *
 *****************************************************************************
 * read regions
 */
static void readregions(void) {
    struct_t *st;
    uint32 n;
    int i, j;

    iread(&bones.moves, sizeof(bones.moves), LONGSZ, NULL);

    bones.regions = NULL;
    iread(&bones.nregions, sizeof(bones.nregions), intsz, NULL);
    if(bones.nregions)
	bones.regions = alloc(sizeof(NhRegion *) * bones.nregions);

    for(i=0; i<bones.nregions; i++) {
	bones.regions[i] = alloc(sizeof(NhRegion *));
	bones.regions[i]->rects = NULL;
	bones.regions[i]->enter_msg = NULL;
	bones.regions[i]->leave_msg = NULL;
	bones.regions[i]->monsters = NULL;

	st = startstruct(NULL, FALSE, CHARSZ);
	iread(&bones.regions[i]->bounding_box.lx, sizeof(int8), CHARSZ, st);
	iread(&bones.regions[i]->bounding_box.ly, sizeof(int8), CHARSZ, st);
	iread(&bones.regions[i]->bounding_box.hx, sizeof(int8), CHARSZ, st);
	iread(&bones.regions[i]->bounding_box.hy, sizeof(int8), CHARSZ, st);
	endstruct(st);

	iread(&bones.regions[i]->nrects, sizeof(bones.regions[i]->nrects), SHORTSZ, NULL);
	if(bones.regions[i]->nrects)
	    bones.regions[i]->rects = alloc(sizeof(NhRect)*bones.regions[i]->nrects);
	for(j=0; j<bones.regions[i]->nrects; j++) {
	    st = startstruct(NULL, FALSE, CHARSZ);
	    iread(&bones.regions[i]->rects[j].lx, sizeof(int8), CHARSZ, st);
	    iread(&bones.regions[i]->rects[j].ly, sizeof(int8), CHARSZ, st);
	    iread(&bones.regions[i]->rects[j].hx, sizeof(int8), CHARSZ, st);
	    iread(&bones.regions[i]->rects[j].hy, sizeof(int8), CHARSZ, st);
	    endstruct(st);
	}

	iread(&bones.regions[i]->attach_2_u,sizeof(bones.regions[i]->attach_2_u),CHARSZ,NULL);
	iread(&bones.regions[i]->attach_2_m,sizeof(bones.regions[i]->attach_2_m),intsz,NULL);

	iread(&n, sizeof(n), intsz, NULL);
	if(n) {
	    bones.regions[i]->enter_msg = alloc(n+1);
	    zread(bones.regions[i]->enter_msg, CHARSZ, n, NULL);
	    bones.regions[i]->enter_msg[n] = '\0';
	    teststring(bones.regions[i]->enter_msg, n+1);
	}

	iread(&n, sizeof(n), intsz, NULL);
	if(n) {
	    bones.regions[i]->leave_msg = alloc(n+1);
	    zread(bones.regions[i]->leave_msg, CHARSZ, n, NULL);
	    bones.regions[i]->leave_msg[n] = '\0';
	    teststring(bones.regions[i]->leave_msg, n+1);
	}

	iread(&bones.regions[i]->ttl, sizeof(int16), SHORTSZ, NULL);
	iread(&bones.regions[i]->expire_f, sizeof(int16), SHORTSZ, NULL);
	iread(&bones.regions[i]->can_enter_f, sizeof(int16), SHORTSZ, NULL);
	iread(&bones.regions[i]->enter_f, sizeof(int16), SHORTSZ, NULL);
	iread(&bones.regions[i]->can_leave_f, sizeof(int16), SHORTSZ, NULL);
	iread(&bones.regions[i]->leave_f, sizeof(int16), SHORTSZ, NULL);
	iread(&bones.regions[i]->inside_f, sizeof(int16), SHORTSZ, NULL);
	iread(&bones.regions[i]->player_flags, sizeof(int8), CHARSZ, NULL);

	iread(&bones.regions[i]->n_monst, sizeof(bones.regions[i]->n_monst),SHORTSZ,NULL);
	if(bones.regions[i]->n_monst)
	    bones.regions[i]->monsters = alloc(sizeof(uint32 *) * bones.regions[i]->n_monst);

	for(j=0; j<bones.regions[i]->n_monst; j++) {
	    iread(&bones.regions[i]->monsters[j], sizeof(uint32), intsz, NULL);
	}
	bones.regions[i]->max_monst = bones.regions[i]->n_monst;

	iread(&bones.regions[i]->visible, sizeof(bones.regions[i]->visible), CHARSZ, NULL);
	iread(&bones.regions[i]->glyph, sizeof(bones.regions[i]->glyph), intsz, NULL);

	/* arg is a genericptr_t, but is only ever used as (int) arg */
	iread(&bones.regions[i]->arg, sizeof(bones.regions[i]->arg), pointersz, NULL);
    }
}
