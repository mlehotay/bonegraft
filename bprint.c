/* CVS $Id: bprint.c,v 1.4 2005/06/23 23:08:43 Michael Exp $ */
/* Copyright (c) Michael Lehotay, 2003. */
/* Bone Graft may be freely redistributed. See license.txt for details. */

/*
 * Portions of this file were derived from the following NetHack source files:
 * display.c ,Copyright (c) Dean Luick, with acknowledgements to Kevin Darcy
 *     and Dave Cohrs, 1990.
 * drawing.c, Copyright (c) NetHack Development Team 1992.
 * makedefs.c, Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985.
 *     Copyright (c) M. Stephenson, 1990, 1991.
 *     Copyright (c) Dean Luick, 1990.
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "graft.h"
#include "bones.h"

/* command line options */
boolean verbose;

/* private functions */
static void printmagicnumbers(void);
static void printfruit(void);
static void printmap(void);
static void printlevelflags(void);


/*****************************************************************************
 * printbones                                                                *
 *****************************************************************************
 * print info from the bones file
 */
void printbones(void) {
    printmagicnumbers();

    if(headeronly)
	return;

    assert(bones.bonesid != NULL);
    printf("Bones ID: %s\n", bones.bonesid);
    printfruit();
    printlevelflags();
    printmap();
}


/*****************************************************************************
 * printmagicnumbers                                                         *
 *****************************************************************************
 * print info from the bones file header.
 */
static void printmagicnumbers(void) {
    const char *bits[] = {"MULDGN", "REINCARNATION", "SINKS",  "<bit 3>",
	"<bit 4>", "ARMY", "KOPS", "MAIL", "<bit 8>", "<bit 9>", "TOURIST",
	"STEED", "GOLDOBJ", "<bit 13>", "<bit 14>", "MUSE", "POLYSELF",
	"TEXTCOLOR", "INSURANCE", "ELBERETH", "EXP_ON_BOTL", "SCORE_ON_BOTL",
	"WEAPON_SKILLS", "TIMED_DELAY", "<bit 24>", "<bit 25>", "<bit 26>",
	"ZEROCOMP", "RLECOMP", "<bit 29>", "<bit 30>", "<bit 31>"};
	/* bit 11 (STEED) was TUTTI_FRUTTI prior to 3.3.0 */
	/* bit 12 (GOLDOBJ) was WALKIES prior to 3.4.0 */
    const char *category[] = {"Dungeon:", "Monsters:", "Objects:", "Flags:",
	"Format:"};
    boolean first = TRUE;
    int i, cat;

    /* if not verbose, just print the numbers and return */
    if(!verbose) {
	printf("Magic numbers: %08lx, %08lx, %08lx", bones.header.incarnation,
	    bones.header.feature_set, bones.header.entity_count);

	if(bones.header.incarnation >= 0x03020100)
	    printf(", %08lx", bones.header.struct_sizes);

	printf("\n");
	return;
    }

    /* print version */
    printf("NetHack version %d.%d.%d editlevel %d\n", bones.vmajor,
	bones.vminor, bones.vpatch, bones.vedit);

    /* print features */
    assert(sizeof(bones.header.feature_set) == 4);
    assert(sizeof(bits)/sizeof(bits[0]) == 32);
    printf("Features");
    cat = 0;
    for(i=0; i<32; i++) {
	switch(i) {
	case 0: case 5: case 10: case 15: case 27:
	    printf("\n %s", category[cat++]);
	    first = TRUE;
	}
	if((1<<i) & bones.header.feature_set) {
	    const char *feature;

	    /* hard wired exceptions. bleh. */
	    if(i==11 && bones.header.incarnation < 0x03030000)
		feature = "TUTTI_FRUTTI";
	    else if(i==12 && bones.header.incarnation < 0x03040000)
		feature = "WALKIES";
	    else
		feature = bits[i];

	    /* printf("%s%s", first ? " " : ", ", feature); */
	    printf(" %s", feature);
	    first = FALSE;
	}
	if(first && (i==4 || i==9 || i==14 || i==26 || i==31))
	    printf(" <none>");
    }
    printf("\n");

    /* print sanity 1 */
    printf("Entity counts\n");
    printf(" Monsters: %d\n", bones.nmonsters);
    printf(" Objects: %d\n", bones.nobjects);
    printf(" Artifacts: %d\n", bones.nartifacts);

    /* print sanity 2 */
    if(bones.header.incarnation >= 0x03020100) {
	printf("Structure sizes\n");
	printf(" you: %d\n", bones.yousz);
	printf(" monst: %d\n", bones.monstsz);
	printf(" obj: %d\n", bones.objsz);
	printf(" flag: %d\n", bones.flagsz);
    }
    printf("\n");
}


/*****************************************************************************
 * printfruit                                                                *
 *****************************************************************************
 * print the names of all the fruits in the bones file
 */
static void printfruit(void) {
    struct fruit *f;

    assert(bones.fruitchain != NULL);

    printf("Fruit:");
    for(f=bones.fruitchain; f->fid; f=f->nextf) {
	assert(f->fname != NULL);
	printf("%s%s", f==bones.fruitchain ? " " : ", ", f->fname);
	assert(f->nextf != NULL);
    }    
    assert(f->nextf == NULL); /* double check we've seen the whole list */
    
    if(f==bones.fruitchain)
	printf(" <none>");
    printf("\n");
}


/*****************************************************************************
 * printmap                                                                  *
 *****************************************************************************
 * print map of the dungeon level
 */
static void printmap(void) {
    /* There are 33 display characters for NetNack 3.2.x and 36 characters for
     * for NetHack 3.3.0 and later. Note that "tree" was moved after 3.3.0.
     * stone, vwall, hwall, tlcorner, trcorner, blcorner, brcorner, crosswall,
     * tuwall, tdwall, tlwall, trwall, dbwall, tree (3.3.1+), sdoor, scorr,
     * pool, moat, water, drawbridge_up, lavapool, ironbars (3.3.0+), door,
     * tree (3.3.0), corr, room, stairs, ladder, fountain, throne, sink,
     * grave (3.3.0+), altar, ice, drawbridge_down, air, cloud. */
    const char glyphs320[] = " |--------||#+#}}}#}+#.%%{\\#_.# #";
    const char glyphs330[] = " |--------||#+#}}}#}#+##.%%{\\#|_.# #";
    const char glyphs331[] = " |--------||##+#}}}#}#+#.%%{\\#|_.# #";
    const char *p;
    int x, y;

    if(bones.header.incarnation<0x03030000)
	p = glyphs320;
    else if(bones.header.incarnation==0x03030000)
	p = glyphs330;
    else
	p = glyphs331;

    for(y=0; y<ROWNO; y++) {
	/* Columns seem to be 1-based arrays, not 0-based. Hope I'm right. */
	for(x=1; x<COLNO; x++) {
	    assert((unsigned) bones.locations[x][y].typ < strlen(p));
	    printf("%c", p[(unsigned) bones.locations[x][y].typ]);
	}
	printf("\n");
    }
}


/*****************************************************************************
 * printlevelflags                                                            *
 *****************************************************************************
 * print the flags concerning the dungeon level
 */
static void printlevelflags(void) {
    printf("Level flags:");
    if(bones.flags.has_shop)
	printf(" has_shop");
    if(bones.flags.has_vault)
	printf(" has_vault");
    if(bones.flags.has_zoo)
	printf(" has_zoo");
    if(bones.flags.has_court)
	printf(" has_court");
    if(bones.flags.has_morgue)
	printf(" has_morgue");
    if(bones.flags.has_beehive)
	printf(" has_beehive");
    if(bones.flags.has_barracks)
	printf(" has_barracks");
    if(bones.flags.has_temple)
	printf(" has_temple");
    if(bones.flags.has_swamp)
	printf(" has_swamp");
    if(bones.flags.noteleport)
	printf(" noteleport");
    if(bones.flags.hardfloor)
	printf(" hardfloor");
    if(bones.flags.nommap)
	printf(" nommap");
    if(bones.flags.hero_memory)
	printf(" hero_memory");
    if(bones.flags.shortsighted)
	printf(" shortsighted");
    if(bones.flags.graveyard)
	printf(" graveyard");
    if(bones.flags.is_maze_lev)
	printf(" is_maze_lev");
    if(bones.flags.is_cavernous_lev)
	printf(" is_cavernous_lev");
    if(bones.flags.arboreal)
	printf(" arboreal");
    printf("\n");
}
