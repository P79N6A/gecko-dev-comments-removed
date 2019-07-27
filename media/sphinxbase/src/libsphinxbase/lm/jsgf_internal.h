




































#ifndef __JSGF_INTERNAL_H__
#define __JSGF_INTERNAL_H__





#include <stdio.h>

#include <sphinxbase/hash_table.h>
#include <sphinxbase/glist.h>
#include <sphinxbase/fsg_model.h>
#include <sphinxbase/logmath.h>
#include <sphinxbase/strfuncs.h>
#include <sphinxbase/jsgf.h>



#if defined(_WIN32) || defined(_WIN32_WCE)
#define strdup _strdup
#endif

#ifdef __cplusplus
extern "C" {
#endif
#if 0

}
#endif

#define YY_NO_INPUT

typedef struct jsgf_rhs_s jsgf_rhs_t;
typedef struct jsgf_atom_s jsgf_atom_t;
typedef struct jsgf_link_s jsgf_link_t;
typedef struct jsgf_rule_stack_s jsgf_rule_stack_t;

struct jsgf_s {
    char *version;  
    char *charset;  
    char *locale;   
    char *name;     

    hash_table_t *rules;   
    hash_table_t *imports; 
    jsgf_t *parent;        
    glist_t searchpath;    

    
    int nstate;            
    glist_t links;	   
    glist_t rulestack;     
};


struct jsgf_rule_stack_s {
    jsgf_rule_t *rule;  
    int entry;          
};

struct jsgf_rule_s {
    int refcnt;      
    char *name;      
    int is_public;   
    jsgf_rhs_t *rhs; 
};

struct jsgf_rhs_s {
    glist_t atoms;   
    jsgf_rhs_t *alt; 
};

struct jsgf_atom_s {
    char *name;        
    glist_t tags;      
    float weight;      
};

struct jsgf_link_s {
    jsgf_atom_t *atom; 
    int from;          
    int to;            
};

#define jsgf_atom_is_rule(atom) ((atom)->name[0] == '<')

void jsgf_add_link(jsgf_t *grammar, jsgf_atom_t *atom, int from, int to);
jsgf_atom_t *jsgf_atom_new(char *name, float weight);
jsgf_atom_t *jsgf_kleene_new(jsgf_t *jsgf, jsgf_atom_t *atom, int plus);
jsgf_rule_t *jsgf_optional_new(jsgf_t *jsgf, jsgf_rhs_t *exp);
jsgf_rule_t *jsgf_define_rule(jsgf_t *jsgf, char *name, jsgf_rhs_t *rhs, int is_public);
jsgf_rule_t *jsgf_import_rule(jsgf_t *jsgf, char *name);

int jsgf_atom_free(jsgf_atom_t *atom);
int jsgf_rule_free(jsgf_rule_t *rule);
jsgf_rule_t *jsgf_rule_retain(jsgf_rule_t *rule);

#ifdef __cplusplus
}
#endif


#endif
