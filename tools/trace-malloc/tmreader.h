




#ifndef tmreader_h___
#define tmreader_h___

#include "plhash.h"
#include "nsTraceMalloc.h"
#include "plarena.h"

#ifdef __cplusplus
extern "C" {
#endif

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
    uint32_t        serial;
    union {
        char        *libname;
        char        *srcname;
        struct {
            uint32_t  library;
            uint32_t  filename;
            uint32_t  linenumber;
            char      *name;
        } method;
        struct {
            uint32_t  parent;
            uint32_t  method;
            uint32_t  offset;
        } site;
        struct {
            uint32_t  interval; 
            uint32_t  ptr;
            uint32_t  size;
            uint32_t  oldserial;
            uint32_t  oldptr;
            uint32_t  oldsize;
            uint32_t  cost;     
        } alloc;
        struct {
            nsTMStats tmstats;
            uint32_t  calltree_maxkids_parent;
            uint32_t  calltree_maxstack_top;
        } stats;
    } u;
};

struct tmcounts {
    uint32_t          direct;     
    uint32_t          total;      
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
    uint32_t      linenumber;
};

#define tmgraphnode_name(node)  ((char*) (node)->entry.value)
#define tmmethodnode_name(node)  ((char*) (node)->graphnode.entry.value)

#define tmlibrary_serial(lib)   ((uint32_t) (lib)->entry.key)
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
    uint32_t        offset;     
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
    uint32_t        ticksPerSec;
};

typedef void (*tmeventhandler)(tmreader *tmr, tmevent *event);


extern tmreader     *tmreader_new(const char *program, void *data);
extern void         tmreader_destroy(tmreader *tmr);





extern int          tmreader_eventloop(tmreader *tmr, const char *filename,
                                       tmeventhandler eventhandler);


extern tmgraphnode  *tmreader_library(tmreader *tmr, uint32_t serial);
extern tmgraphnode  *tmreader_filename(tmreader *tmr, uint32_t serial);
extern tmgraphnode  *tmreader_component(tmreader *tmr, const char *name);
extern tmmethodnode  *tmreader_method(tmreader *tmr, uint32_t serial);
extern tmcallsite   *tmreader_callsite(tmreader *tmr, uint32_t serial);
















extern int tmgraphnode_connect(tmgraphnode *from, tmgraphnode *to,
                               tmcallsite *site);

#ifdef __cplusplus
}
#endif

#endif
