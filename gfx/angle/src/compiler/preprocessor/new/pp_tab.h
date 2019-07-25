



































#ifndef YYTOKENTYPE
# define YYTOKENTYPE


   enum yytokentype {
     HASH = 258,
     HASH_DEFINE_OBJ = 259,
     HASH_DEFINE_FUNC = 260,
     HASH_UNDEF = 261,
     HASH_IF = 262,
     HASH_IFDEF = 263,
     HASH_IFNDEF = 264,
     HASH_ELSE = 265,
     HASH_ELIF = 266,
     HASH_ENDIF = 267,
     DEFINED = 268,
     HASH_ERROR = 269,
     HASH_PRAGMA = 270,
     HASH_EXTENSION = 271,
     HASH_VERSION = 272,
     HASH_LINE = 273,
     SPACE = 274,
     INT_CONSTANT = 275,
     FLOAT_CONSTANT = 276,
     IDENTIFIER = 277
   };
#endif

#define HASH 258
#define HASH_DEFINE_OBJ 259
#define HASH_DEFINE_FUNC 260
#define HASH_UNDEF 261
#define HASH_IF 262
#define HASH_IFDEF 263
#define HASH_IFNDEF 264
#define HASH_ELSE 265
#define HASH_ELIF 266
#define HASH_ENDIF 267
#define DEFINED 268
#define HASH_ERROR 269
#define HASH_PRAGMA 270
#define HASH_EXTENSION 271
#define HASH_VERSION 272
#define HASH_LINE 273
#define SPACE 274
#define INT_CONSTANT 275
#define FLOAT_CONSTANT 276
#define IDENTIFIER 277




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE

{
    int ival;
    std::string* sval;
    std::vector<std::string*>* slist;
    pp::Token* tval;
    pp::TokenVector* tlist;
}


	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
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


