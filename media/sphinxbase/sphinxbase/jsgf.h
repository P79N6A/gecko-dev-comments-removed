




































#ifndef __JSGF_H__
#define __JSGF_H__








#include <stdio.h>


#include <sphinxbase/sphinxbase_export.h>
#include <sphinxbase/hash_table.h>
#include <sphinxbase/fsg_model.h>
#include <sphinxbase/logmath.h>

#ifdef __cplusplus
extern "C" {
#endif
#if 0

}
#endif

typedef struct jsgf_s jsgf_t;
typedef struct jsgf_rule_s jsgf_rule_t;







SPHINXBASE_EXPORT
jsgf_t *jsgf_grammar_new(jsgf_t *parent);








SPHINXBASE_EXPORT
jsgf_t *jsgf_parse_file(const char *filename, jsgf_t *parent);








SPHINXBASE_EXPORT
jsgf_t *jsgf_parse_string(const char *string, jsgf_t *parent);




SPHINXBASE_EXPORT
char const *jsgf_grammar_name(jsgf_t *jsgf);




SPHINXBASE_EXPORT
void jsgf_grammar_free(jsgf_t *jsgf);




typedef hash_iter_t jsgf_rule_iter_t;




SPHINXBASE_EXPORT
jsgf_rule_iter_t *jsgf_rule_iter(jsgf_t *grammar);




#define jsgf_rule_iter_next(itor) hash_table_iter_next(itor)




#define jsgf_rule_iter_rule(itor) ((jsgf_rule_t *)(itor)->ent->val)




#define jsgf_rule_iter_free(itor) hash_table_iter_free(itor)




SPHINXBASE_EXPORT
jsgf_rule_t *jsgf_get_rule(jsgf_t *grammar, const char *name);



 
SPHINXBASE_EXPORT
jsgf_rule_t *jsgf_get_public_rule(jsgf_t *grammar);




SPHINXBASE_EXPORT
char const *jsgf_rule_name(jsgf_rule_t *rule);




SPHINXBASE_EXPORT
int jsgf_rule_public(jsgf_rule_t *rule);




SPHINXBASE_EXPORT
fsg_model_t *jsgf_build_fsg(jsgf_t *grammar, jsgf_rule_t *rule,
                            logmath_t *lmath, float32 lw);









SPHINXBASE_EXPORT
fsg_model_t *jsgf_build_fsg_raw(jsgf_t *grammar, jsgf_rule_t *rule,
                                logmath_t *lmath, float32 lw);







SPHINXBASE_EXPORT
fsg_model_t *jsgf_read_file(const char *file, logmath_t * lmath, float32 lw);






SPHINXBASE_EXPORT
fsg_model_t *jsgf_read_string(const char *string, logmath_t * lmath, float32 lw);








SPHINXBASE_EXPORT
int jsgf_write_fsg(jsgf_t *grammar, jsgf_rule_t *rule, FILE *outfh);

#ifdef __cplusplus
}
#endif


#endif
