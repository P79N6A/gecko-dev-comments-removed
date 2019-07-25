


































#ifndef YYTOKENTYPE
# define YYTOKENTYPE


   enum yytokentype {
     HASH = 258,
     HASH_UNDEF = 259,
     HASH_IF = 260,
     HASH_IFDEF = 261,
     HASH_IFNDEF = 262,
     HASH_ELSE = 263,
     HASH_ELIF = 264,
     HASH_ENDIF = 265,
     DEFINED = 266,
     HASH_ERROR = 267,
     HASH_PRAGMA = 268,
     HASH_EXTENSION = 269,
     HASH_VERSION = 270,
     HASH_LINE = 271,
     HASH_DEFINE_OBJ = 272,
     HASH_DEFINE_FUNC = 273,
     INT_CONSTANT = 274,
     FLOAT_CONSTANT = 275,
     IDENTIFIER = 276
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{


    int ival;
    std::string* sval;
    pp::Token* tval;
    pp::TokenVector* tlist;



} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif



#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
# define yyltype YYLTYPE /* obsolescent; will be withdrawn */
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif



