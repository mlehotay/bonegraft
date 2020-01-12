/* CVS $Id: bones.h,v 1.4 2005/06/23 20:35:09 Michael Exp $ */
/* Copyright (c) Michael Lehotay, 2003. */
/* Bone Graft may be freely redistributed. See license.txt for details. */

/*
 * Portions of this file were derived from the following NetHack source files:
 * coord.h, Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985.
 * dungeon.h, Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985.
 * edog.h, Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985.
 * emin.h, Copyright (c) David Cohrs, 1990.
 * engrave.h, Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985.
 * epri.h, Copyright (c) Izchak Miller, 1989.
 * eshk.h, Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985.
 * global.h, Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985.
 * lev.h, Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985.
 * mkroom.h, Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985.
 * monst.h, Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985.
 * obj.h, Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985.
 * objclass.h, Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985.
 * rect.h, Copyright (c) 1990 by Jean-Christophe Collet
 * region.h, Copyright (c) 1996 by Jean-Christophe Collet
 * rm.h, Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985.
 * timeout.h, Copyright 1994, Dean Luick
 * trap.h, Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985.
 * vault.h, Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985.
 * worm.c, Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985.
 */

#ifndef BONES_H
#define BONES_H

#include "graft.h"

/*
 * structs from NetHack header files, modified as follows:
 * ints and longs changed to int32 or uint32
 * xchar, schar, uchar changed to int8 or uint8
 * boolean, aligntyp changed to int8
 * const qualifiers removed
 * bitfields declared without Bitfield() macros
 * void pointers declared without genericptr_t macros
 * pointers changed to uint32 when their values are important
 * unions replaced with their members (some of which are then unused)
 * crazy mextra and oextra data replaced with regular pointers
 *
 * some struct members depend on macros being defined, we can figure
 * some of the macro definitions from the magic numbers in the file header,
 * but the rest will have to be given as command line options.
 */

struct version_info {
    uint32 incarnation;	    /* actual version number */
    uint32 feature_set;	    /* bitmask of config settings */
    uint32 entity_count;    /* # of monsters and objects */
    uint32 struct_sizes;    /* size of key structs */
};

#define PL_FSIZ 32 /* fruit name */
struct fruit {
    char fname[PL_FSIZ];
    int32 fid;
    struct fruit *nextf;
};

#define COLNO 80
#define ROWNO 21
struct rm {
    int32 glyph;	    /* what the hero thinks is there */
    int8 typ;		    /* what is really there */
    uint8 seenv;	    /* seen vector */
    unsigned flags:5;	    /* extra information for typ */
    unsigned horizontal:1;  /* wall/door/etc is horiz. (more typ info) */
    unsigned lit:1;	    /* speed hack for lit rooms */
    unsigned waslit:1;	    /* remember if a location was lit */
    unsigned roomno:6;	    /* room # for special rooms */
    unsigned edge:1;	    /* marks boundaries for special rooms*/
};

typedef struct d_level {    /* basic dungeon level element */
    int8 dnum;		    /* dungeon number */
    int8 dlevel;	    /* level number */
} d_level;

typedef struct stairway {   /* basic stairway identifier */
    int8 sx, sy;	    /* x / y location of the stair */
    d_level tolev;	    /* where does it go */
    int8 up;		    /* what type of stairway (up/down) */
} stairway;

typedef struct dest_area {  /* non-stairway level change indentifier */
    int8 lx, ly;	    /* "lower" left corner (near [0,0]) */
    int8 hx, hy;	    /* "upper" right corner (near [COLNO,ROWNO]) */
    int8 nlx, nly;	    /* outline of invalid area */
    int8 nhx, nhy;	    /* opposite corner of invalid area */
} dest_area;

struct levelflags {
    uint8 nfountains;	    /* number of fountains on level */
    uint8 nsinks;	    /* number of sinks on the level */
    unsigned has_shop:1;
    unsigned has_vault:1;
    unsigned has_zoo:1;
    unsigned has_court:1;
    unsigned has_morgue:1;
    unsigned has_beehive:1;
    unsigned has_barracks:1;
    unsigned has_temple:1;
    unsigned has_swamp:1;
    unsigned noteleport:1;
    unsigned hardfloor:1;
    unsigned nommap:1;
    unsigned hero_memory:1; /* hero has memory */
    unsigned shortsighted:1;/* monsters are shortsighted */
    unsigned graveyard:1;   /* has_morgue, but remains set */
    unsigned is_maze_lev:1;
    unsigned is_cavernous_lev:1;
    unsigned arboreal:1;    /* Trees replace rock */
};

#define DOORMAX 120	    /* max number of doors per level */
typedef struct nhcoord {
    int8 x,y;
} coord;

#define MAXNROFROOMS 40	    /* max number of rooms per level */
#define MAX_SUBROOMS 24	    /* max # of subrooms in a given room */
struct mkroom {
    int8 lx,hx,ly,hy;	    /* usually xchar, but hx may be -1 */
    int8 rtype;		    /* type of room (zoo, throne, etc...) */
    int8 rlit;		    /* is the room lit ? */
    int8 doorct;	    /* door count */
    int8 fdoor;		    /* index for the first door of the room */
    int8 nsubrooms;	    /* number of subrooms */
    int8 irregular;	    /* true if room is non-rectangular */
    struct mkroom *sbrooms[MAX_SUBROOMS]; /* Subrooms pointers */
    struct monst *resident; /* priest/shopkeeper/guard for this room */
};

typedef struct fe {
    struct fe *next;	    /* next item in chain */
    int32 timeout;	    /* when we time out */
    uint32 tid;		    /* timer ID */
    int16 kind;		    /* kind of use */
    int16 func_index;	    /* what to call when we time out */
    void * arg;		    /* pointer to timeout argument */
    unsigned needs_fixup:1; /* does arg need to be patched? */
} timer_element;

typedef struct ls_t {
    struct ls_t *next;
    int8 x, y;		    /* source's position */
    int16 range;	    /* source's current range */
    int16 flags;
    int16 type;		    /* type of light source */
    void *id;		    /* source's identifier */
} light_source;

#define MTSZ	4
struct monst {
    struct monst *nmon;
    uint32 data;	    /* officially (struct permonst *) */
    uint32 m_id;
    int16 mnum;		    /* permanent monster index number */
    int16 movement;	    /* movement points  */
    uint8 m_lev;	    /* adjusted difficulty level of monster */
    int8 malign;	    /* alignment of this monster */
    int8 mx, my;
    int8 mux, muy;	    /* where the monster thinks you are */
    coord mtrack[MTSZ];	    /* monster track */
    int32 mhp, mhpmax;
    uint32 mappearance;	    /* for undetected mimics and the wiz */
    uint8 m_ap_type;	    /* what mappearance is describing: */
    int8 mtame;		    /* level of tameness, implies peaceful */
    uint16 mintrinsics;	    /* low 8 correspond to mresists */
    int32 mspec_used;	    /* monster's special ability attack timeout */
    unsigned female:1;	    /* is female */
    unsigned minvis:1;	    /* currently invisible */
    unsigned invis_blkd:1;  /* invisibility blocked */
    unsigned perminvis:1;   /* intrinsic minvis value */
    unsigned cham:3;	    /* shape-changer */
    unsigned mundetected:1; /* not seen in present hiding place */
    unsigned mcan:1;	    /* has been cancelled */
    unsigned mburied:1;	    /* has been buried */
    unsigned mspeed:2;	    /* current speed */
    unsigned permspeed:2;   /* intrinsic mspeed value */
    unsigned mrevived:1;    /* has been revived from the dead */
    unsigned mavenge:1;	    /* did something to deserve retaliation */
    unsigned mflee:1;	    /* fleeing */
    unsigned mfleetim:7;    /* timeout for mflee */
    unsigned mcansee:1;	    /* cansee 1, temp.blinded 0, blind 0 */
    unsigned mblinded:7;    /* cansee 0, temp.blinded n, blind 0 */
    unsigned mcanmove:1;    /* paralysis, similar to mblinded */
    unsigned mfrozen:7;
    unsigned msleeping:1;   /* asleep until woken */
    unsigned mstun:1;	    /* stunned (off balance) */
    unsigned mconf:1;	    /* confused */
    unsigned mpeaceful:1;   /* does not attack unprovoked */
    unsigned mtrapped:1;    /* trapped in a pit, web or bear trap */
    unsigned mleashed:1;    /* monster is on a leash */
    unsigned isshk:1;	    /* is shopkeeper */
    unsigned isminion:1;    /* is a minion */
    unsigned isgd:1;	    /* is guard */
    unsigned ispriest:1;    /* is a priest */
    unsigned iswiz:1;	    /* is the Wizard of Yendor */
    unsigned wormno:5;	    /* at most 31 worms on any level */
    int32 mstrategy;	    /* for monsters with mflag3: current strategy */
    int32 mtrapseen;	    /* bitmap of traps we've been trapped in */
    int32 mlstmv;	    /* for catching up with lost time */
    int32 mgold;	    /* ifndef GOLDOBJ */
    struct obj *minvent;
    struct obj *mw;
    int32 misc_worn_check;
    int8 weapon_check;
    uint8 mnamelth;	    /* length of name (following mxlth) */
    int16 mxlth;	    /* length of following data */
    int32 meating;	    /* monster is eating timeout */
    int32 mextra[1];	    /* monster dependent info */

    /* pointers to mextra data */
    struct egd *egd;
    struct epri *epri;
    struct eshk *eshk;
    struct emin *emin;
    struct edog *edog;
    char *ghostname; /* ghost names were stored at mextra prior to 3.3.0 */
    char *mname;
};

struct fakecorridor {
    int8 fx,fy,ftyp;
};

#define FCSIZ (ROWNO+COLNO)
struct egd {
    int32 fcbeg, fcend;	    /* fcend: first unused pos */
    int32 vroom;	    /* room number of the vault */
    int8 gdx, gdy;	    /* goal of guard's walk */
    int8 ogx, ogy;	    /* guard's last position */
    d_level gdlevel;	    /* level (& dungeon) guard was created in */
    int8 warncnt;	    /* number of warnings to follow */
    unsigned gddone:1;	    /* true iff guard has released player */
    unsigned unused:7;
    struct fakecorridor fakecorr[FCSIZ];
};

struct epri {
    int8 shralign;	    /* alignment of priest's shrine */
    int8 shroom;	    /* index in rooms */
    coord shrpos;	    /* position of shrine */
    d_level shrlevel;	    /* level (& dungeon) of shrine */
};

struct bill_x {
    uint32 bo_id;
    int8 useup;
    int32 price;	    /* price per unit */
    int32 bquan;	    /* amount used up */
};

#define BILLSZ 200
#define PL_NSIZ 32	    /* name of player, ghost, shopkeeper */
struct eshk {
    int32 robbed;	    /* amount stolen by most recent customer */
    int32 credit;	    /* amount credited to customer */
    int32 debit;	    /* amount of debt for using unpaid items */
    int32 loan;		    /* shop-gold picked (part of debit) */
    int32 shoptype;	    /* the value of rooms[shoproom].rtype */
    int8 shoproom;	    /* index in rooms; set by inshop() */
    int8 unused;	    /* to force alignment for stupid compilers */
    int8 following;	    /* following customer since he owes us sth */
    int8 surcharge;	    /* angry shk inflates prices */
    coord shk;		    /* usual position shopkeeper */
    coord shd;		    /* position shop door */
    d_level shoplevel;	    /* level (& dungeon) of his shop */
    int32 billct;	    /* no. of entries of bill[] in use */
    struct bill_x bill[BILLSZ];
    struct bill_x *bill_p;
    int32 visitct;	    /* nr of visits by most recent customer */
    char customer[PL_NSIZ]; /* most recent customer */
    char shknam[PL_NSIZ];
};

struct emin {
    int8 min_align;	    /* alignment of minion */
};

struct edog {
    int32 droptime;	    /* moment dog dropped object */
    uint32 dropdist;	    /* dist of drpped obj from @ */
    int32 apport;	    /* amount of training */
    int32 whistletime;	    /* last time he whistled */
    int32 hungrytime;	    /* will get hungry at this time */
    coord ogoal;	    /* previous goal location */
    int32 abuse;	    /* track abuses to this pet */
    int32 revivals;	    /* count pet deaths */
    int32 mhpmax_penalty;   /* while starving, points reduced */
    unsigned killed_by_u:1; /* you attempted to kill him */
};

#define OATTACHED_NOTHING 0
#define OATTACHED_MONST	1   /* monst struct in oextra */
#define OATTACHED_M_ID	2   /* monst id in oextra */
struct obj {
    struct obj *nobj;
    void *vptr;		    /* supposed to be a union of pointers */
    struct obj *cobj;	    /* contents list for containers */
    uint32 o_id;
    int8 ox, oy;
    int16 otyp;		    /* object class number */
    uint32 owt;
    int32 quan;		    /* number of items */
    int8 spe;		    /* meaning depends on type of object */
    int8 oclass;	    /* object class */
    int8 invlet;	    /* designation in inventory */
    int8 oartifact;	    /* artifact array index */
    int8 where;		    /* where the object thinks it is */
    int8 timed;		    /* # of fuses (timers) attached to this obj */
    unsigned cursed:1;
    unsigned blessed:1;
    unsigned unpaid:1;	    /* on some bill */
    unsigned no_charge:1;   /* if shk shouldn't charge for this */
    unsigned known:1;	    /* exact nature known */
    unsigned dknown:1;	    /* color or text known */
    unsigned bknown:1;	    /* blessing or curse known */
    unsigned rknown:1;	    /* rustproof or not known */
    unsigned oeroded:2;	    /* rusted/burnt weapon/armor */
    unsigned oeroded2:2;    /* corroded/rotted weapon/armor */
    unsigned oerodeproof:1; /* erodeproof weapon/armor */
    unsigned olocked:1;	    /* object is locked */
    unsigned obroken:1;	    /* lock has been broken */
    unsigned otrapped:1;    /* container is trapped */
    unsigned recharged:3;   /* number of times it's been recharged */
    unsigned lamplit:1;	    /* a light-source -- can be lit */
    unsigned oinvis:1;	    /* ifdef INVISIBLE_OBJECTS */
    unsigned greased:1;	    /* covered with grease */
    unsigned oattached:2;   /* obj struct has special attachment */
    unsigned in_use:1;	    /* ifndef NO_SIGNAL || incarnation>=3.3.0 */
    unsigned bypass:1;	    /* mark this as an object to be skipped by bhito() */
    int32 corpsenm;	    /* type of corpse is mons[corpsenm] */
    uint32 oeaten;	    /* nutrition left in food, if partly eaten */
    int32 age;		    /* creation date */
    uint8 onamelth;	    /* length of name (following oxlth) */
    int16 oxlth;	    /* length of following data */
    int32 owornmask;
    int32 oextra[1];

    /* oextra data */
    struct monst *monst;
    uint32 m_id;
    char *oname;
};

#define MAX_NUM_WORMS 32
struct wseg {
    struct wseg *nseg;
    int8 wx, wy;	    /* the segment's position */
};

#define ROLLING_BOULDER_TRAP 7
struct trap {
    struct trap *ntrap;
    int8 tx,ty;
    d_level dst;	    /* destination for portals */
    coord launch;
    unsigned ttyp:5;
    unsigned tseen:1;
    unsigned once:1;
    unsigned madeby_u:1;
    /* nethack defines the next two members as a union */
    int16 launch_otyp;	    /* type of object to be triggered */
    coord launch2;	    /* secondary launch point (for boulders) */
};

#define BUFSZ 256	    /* for getlin buffers */
struct engr {
    struct engr *nxt_engr;
    char *engr_txt;
    int8 engr_x, engr_y;
    uint32 engr_lth;	    /* for save & restore; not length of text */
    int32 engr_time;	    /* moment engraving was (will be) finished */
    int8 engr_type;
};

struct damage {
    struct damage *next;
    int32 when, cost;
    coord place;
    int8 typ;
};

typedef struct nhrect {
    int8 lx, ly;
    int8 hx, hy;
} NhRect;

typedef struct {
    NhRect bounding_box;    /* Bounding box of the region */
    NhRect *rects;	    /* Rectangles composing the region */
    int16 nrects;	    /* Number of rectangles  */
    int8 attach_2_u;	    /* Region attached to player ? */
    uint32 attach_2_m;	    /* Region attached to monster ? */
    char *enter_msg;	    /* Message when entering */
    char *leave_msg;	    /* Message when leaving */
    int16 ttl;		    /* Time to live. -1 is forever */
    int16 expire_f;	    /* Function to call when region's ttl expire */
    int16 can_enter_f;	    /* Function, can player enter? */
    int16 enter_f;	    /* Function to call when the player enters*/
    int16 can_leave_f;	    /* Function, can player leave? */
    int16 leave_f;	    /* Function to call when the player leaves */
    int16 inside_f;	    /* Function to call every turn player is inside */
    int8 player_flags;
    uint32 *monsters;	    /* Monsters currently inside this region */
    int16 n_monst;	    /* Number of monsters inside this region */
    int16 max_monst;	    /* Maximum number of monsters */
    int8 visible;	    /* Is the region visible ? */
    int32 glyph;	    /* Which glyph to use if visible */
    uint32 arg;		    /* Optional user argument */
} NhRegion;


/* contents of the bones file */
typedef struct {
    struct version_info header; /* aka "the magic numbers" */
    uint8 vmajor, vminor, vpatch, vedit; /* parsed header values */
    uint16 nmonsters, nobjects, yousz; /* parsed header values */
    uint8 nartifacts, monstsz, objsz, flagsz; /* parsed header values */
    char *bonesid;
    struct fruit *fruitchain;
    int32 hpid;
    int8 dlvl;
    struct rm locations[COLNO][ROWNO];
    int32 monstermoves;
    stairway upstair, dnstair, upladder, dnladder, sstairs;
    dest_area updest, dndest;
    struct levelflags flags;
    coord doors[DOORMAX];
    int32 nroom;
    struct mkroom *rooms[MAXNROFROOMS+1]; /* the +1 is for a vault */
    int32 ntimers;
    timer_element *timer_base;
    int32 nlights;
    light_source *light_base;
    uint32 monbegin;
    struct monst *monchn;
    struct wseg *wsegments[MAX_NUM_WORMS];
    int32 wgrowtime[MAX_NUM_WORMS];
    struct trap *trapchn;
    struct obj *objchn;
    struct obj *buriedobjs;
    struct obj *billobjs;
    struct engr *engravings;
    struct damage *damage;
    int32 moves;
    int32 nregions;
    NhRegion **regions;
} bones_t;

extern bones_t bones;

#endif
