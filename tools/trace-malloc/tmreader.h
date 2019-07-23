






































#ifndef tmreader_h___
#define tmreader_h___

#include "prtypes.h"
#include "plhash.h"
#include "nsTraceMalloc.h"
#include "plarena.h"

PR_BEGIN_EXTERN_C

typedef struct tmreader     tmreader;
typedef struct tmevent      tmevent;
typedef struct tmcounts     tmcounts;
typedef struct tmallcounts  tmallcounts;
typedef struct tmgraphlink  tmgraphlink;
typedef struct tmgraphedge  tmgraphedge;
typedef struct tmgraphnode  tmgraphnode;
typedef struct tmcallsite   tmcallsite;
typedef struct tmmethodnode tmmethodnode;

struct tmevent {
    char            type;
    uint32          serial;
    union {
        char        *libname;
        char        *srcname;
        struct {
            uint32  library;
            uint32  filename;
            uint32  linenumber;
            char    *name;
        } method;
        struct {
            uint32  parent;
            uint32  method;
            uint32  offset;
        } site;
        struct {
            uint32  interval; 
            uint32  ptr;
            uint32  size;
            uint32  oldserial;
            uint32  oldptr;
            uint32  oldsize;
            uint32  cost;     
        } alloc;
        struct {
            nsTMStats tmstats;
            uint32  calltree_maxkids_parent;
            uint32  calltree_maxstack_top;
        } stats;
    } u;
};

struct tmcounts {
    uint32          direct;     
    uint32          total;      
};

struct tmallcounts {
    tmcounts        bytes;
    tmcounts        calls;
};

struct tmgraphnode {
    PLHashEntry     entry;      
    tmgraphlink     *in;
    tmgraphlink     *out;
    tmgraphnode     *up;        
    tmgraphnode     *down;      
    tmgraphnode     *next;      
    int             low;        
    tmallcounts     allocs;
    tmallcounts     frees;
    double          sqsum;      
    int             sort;       
};

struct tmmethodnode {
    tmgraphnode   graphnode;
    char          *sourcefile;
    uint32        linenumber;
};

#define tmgraphnode_name(node)  ((char*) (node)->entry.value)
#define tmmethodnode_name(node)  ((char*) (node)->graphnode.entry.value)

#define tmlibrary_serial(lib)   ((uint32) (lib)->entry.key)
#define tmcomponent_name(comp)  ((const char*) (comp)->entry.key)
#define filename_name(hashentry) ((char*)hashentry->value)


struct tmgraphlink {
    tmgraphlink     *next;      
    tmgraphnode     *node;      
};







struct tmgraphedge {
    tmgraphlink     links[2];
    tmallcounts     allocs;
    tmallcounts     frees;
};


#define TM_EDGE_OUT_LINK        0
#define TM_EDGE_IN_LINK         1

#define TM_LINK_TO_EDGE(link,which) ((tmgraphedge*) &(link)[-(which)])

struct tmcallsite {
    PLHashEntry     entry;      
    tmcallsite      *parent;    
    tmcallsite      *siblings;  
    tmcallsite      *kids;      
    tmmethodnode    *method;    
    uint32          offset;     
    tmallcounts     allocs;
    tmallcounts     frees;
    void            *data;      


};

struct tmreader {
    const char      *program;
    void            *data;
    PLHashTable     *libraries;
    PLHashTable     *filenames;
    PLHashTable     *components;
    PLHashTable     *methods;
    PLHashTable     *callsites;
    PLArenaPool     arena;
    tmcallsite      calltree_root;
    uint32          ticksPerSec;
};

typedef void (*tmeventhandler)(tmreader *tmr, tmevent *event);


extern tmreader     *tmreader_new(const char *program, void *data);
extern void         tmreader_destroy(tmreader *tmr);





extern int          tmreader_eventloop(tmreader *tmr, const char *filename,
                                       tmeventhandler eventhandler);


extern tmgraphnode  *tmreader_library(tmreader *tmr, uint32 serial);
extern tmgraphnode  *tmreader_filename(tmreader *tmr, uint32 serial);
extern tmgraphnode  *tmreader_component(tmreader *tmr, const char *name);
extern tmmethodnode  *tmreader_method(tmreader *tmr, uint32 serial);
extern tmcallsite   *tmreader_callsite(tmreader *tmr, uint32 serial);
















extern int tmgraphnode_connect(tmgraphnode *from, tmgraphnode *to,
                               tmcallsite *site);

PR_END_EXTERN_C

#endif 
