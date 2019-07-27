



































#ifndef YYTOKENTYPE
# define YYTOKENTYPE


   enum yytokentype {
     HEADER = 258,
     GRAMMAR = 259,
     IMPORT = 260,
     PUBLIC = 261,
     TOKEN = 262,
     RULENAME = 263,
     TAG = 264,
     WEIGHT = 265
   };
#endif

#define HEADER 258
#define GRAMMAR 259
#define IMPORT 260
#define PUBLIC 261
#define TOKEN 262
#define RULENAME 263
#define TAG 264
#define WEIGHT 265




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{


#line 65 "jsgf_parser.y"

       char *name;
       float weight;
       jsgf_rule_t *rule;
       jsgf_rhs_t *rhs;
       jsgf_atom_t *atom;




#line 82 "jsgf_parser.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif




