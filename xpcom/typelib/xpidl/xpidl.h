








































#ifndef __xpidl_h
#define __xpidl_h

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <string.h> 

#ifndef XP_MAC
#include <libIDL/IDL.h>
#else
#include <IDL.h>
#endif

#include <xpt_struct.h>






#if !(LIBIDL_MAJOR_VERSION == 0 && LIBIDL_MINOR_VERSION == 6 && \
      LIBIDL_MICRO_VERSION == 5) && !defined(DEBUG_shaver)







#define XPIDL_WARNING(x) IDL_tree_warning x
#else
extern void xpidl_tree_warning(IDL_tree p, int level, const char *fmt, ...);
#define XPIDL_WARNING(x) xpidl_tree_warning x
#endif




extern gboolean enable_debug;
extern gboolean enable_warnings;
extern gboolean verbose_mode;
extern gboolean emit_typelib_annotations;
extern gboolean explicit_output_filename;
extern FILE *deps;

extern PRUint8  major_version;
extern PRUint8  minor_version;

typedef struct TreeState TreeState;




typedef gboolean (*nodeHandler)(TreeState *);




typedef struct backend {
    nodeHandler *dispatch_table; 
    nodeHandler emit_prolog;     
    nodeHandler emit_epilog;     
} backend;


typedef backend *(*backendFactory)();
 
extern backend *xpidl_header_dispatch(void);
extern backend *xpidl_typelib_dispatch(void);
extern backend *xpidl_doc_dispatch(void);
extern backend *xpidl_java_dispatch(void);

typedef struct ModeData {
    char               *mode;
    char               *modeInfo;
    char               *suffix;
    backendFactory     factory;
} ModeData;

typedef struct IncludePathEntry {
    char                    *directory;
    struct IncludePathEntry *next;
} IncludePathEntry;

struct TreeState {
    FILE             *file;
    char             *filename;
    
    char             *basename;
    IDL_ns           ns;
    IDL_tree         tree;
    GSList           *base_includes;
    nodeHandler      *dispatch;
    void             *priv;     
};

struct java_priv_data {
    GHashTable *typedefTable;
    GHashTable *keywords;
    char       *filename;
    GHashTable *nonIDLIfaces;
};





int
xpidl_process_idl(char *filename, IncludePathEntry *include_path,
                  char *file_basename, ModeData *mode);




void
xpidl_list_foreach(IDL_tree p, IDL_tree_func foreach, gpointer user_data);




void *
xpidl_malloc(size_t nbytes);

char *
xpidl_strdup(const char *s);




const char *
xpidl_basename(const char * path);




gboolean
xpidl_process_node(TreeState *state);





void
xpidl_write_comment(TreeState *state, int indent);










#define UUID_LENGTH 37





gboolean
xpidl_sprint_iid(nsID *iid, char iidbuf[]);





gboolean
xpidl_parse_iid(nsID *id, const char *str);





#define UP_IS_AGGREGATE(node)                                                 \
    (IDL_NODE_UP(node) &&                                                     \
     (IDL_NODE_TYPE(IDL_NODE_UP(node)) == IDLN_INTERFACE ||                   \
      IDL_NODE_TYPE(IDL_NODE_UP(node)) == IDLN_FORWARD_DCL))

#define UP_IS_NATIVE(node)                                                    \
    (IDL_NODE_UP(node) &&                                                     \
     IDL_NODE_TYPE(IDL_NODE_UP(node)) == IDLN_NATIVE)


#define STARRED_TYPE(node) (IDL_NODE_TYPE(node) == IDLN_TYPE_STRING ||        \
                            IDL_NODE_TYPE(node) == IDLN_TYPE_WIDE_STRING ||   \
                            (IDL_NODE_TYPE(node) == IDLN_IDENT &&             \
                             UP_IS_AGGREGATE(node)))

#define DIPPER_TYPE(node)                                                     \
    (NULL != IDL_tree_property_get(node, "domstring")  ||                     \
     NULL != IDL_tree_property_get(node, "utf8string") ||                     \
     NULL != IDL_tree_property_get(node, "cstring")    ||                     \
     NULL != IDL_tree_property_get(node, "astring"))





IDL_tree 
find_underlying_type(IDL_tree typedef_ident);





gboolean
verify_const_declaration(IDL_tree const_tree);




gboolean
verify_attribute_declaration(IDL_tree method_tree);




gboolean
verify_method_declaration(IDL_tree method_tree);



 
gboolean
verify_interface_declaration(IDL_tree method_tree);





gboolean
check_native(TreeState *state);

void
printlist(FILE *outfile, GSList *slist);

#endif 
