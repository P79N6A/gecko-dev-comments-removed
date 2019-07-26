































#ifndef YY_YY_GLSLANG_TAB_H_INCLUDED
# define YY_YY_GLSLANG_TAB_H_INCLUDED

#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif



#define YYLTYPE TSourceLoc
#define YYLTYPE_IS_DECLARED 1





#ifndef YYTOKENTYPE
# define YYTOKENTYPE


   enum yytokentype {
     INVARIANT = 258,
     HIGH_PRECISION = 259,
     MEDIUM_PRECISION = 260,
     LOW_PRECISION = 261,
     PRECISION = 262,
     ATTRIBUTE = 263,
     CONST_QUAL = 264,
     BOOL_TYPE = 265,
     FLOAT_TYPE = 266,
     INT_TYPE = 267,
     BREAK = 268,
     CONTINUE = 269,
     DO = 270,
     ELSE = 271,
     FOR = 272,
     IF = 273,
     DISCARD = 274,
     RETURN = 275,
     BVEC2 = 276,
     BVEC3 = 277,
     BVEC4 = 278,
     IVEC2 = 279,
     IVEC3 = 280,
     IVEC4 = 281,
     VEC2 = 282,
     VEC3 = 283,
     VEC4 = 284,
     MATRIX2 = 285,
     MATRIX3 = 286,
     MATRIX4 = 287,
     IN_QUAL = 288,
     OUT_QUAL = 289,
     INOUT_QUAL = 290,
     UNIFORM = 291,
     VARYING = 292,
     STRUCT = 293,
     VOID_TYPE = 294,
     WHILE = 295,
     SAMPLER2D = 296,
     SAMPLERCUBE = 297,
     SAMPLER_EXTERNAL_OES = 298,
     SAMPLER2DRECT = 299,
     IDENTIFIER = 300,
     TYPE_NAME = 301,
     FLOATCONSTANT = 302,
     INTCONSTANT = 303,
     BOOLCONSTANT = 304,
     LEFT_OP = 305,
     RIGHT_OP = 306,
     INC_OP = 307,
     DEC_OP = 308,
     LE_OP = 309,
     GE_OP = 310,
     EQ_OP = 311,
     NE_OP = 312,
     AND_OP = 313,
     OR_OP = 314,
     XOR_OP = 315,
     MUL_ASSIGN = 316,
     DIV_ASSIGN = 317,
     ADD_ASSIGN = 318,
     MOD_ASSIGN = 319,
     LEFT_ASSIGN = 320,
     RIGHT_ASSIGN = 321,
     AND_ASSIGN = 322,
     XOR_ASSIGN = 323,
     OR_ASSIGN = 324,
     SUB_ASSIGN = 325,
     LEFT_PAREN = 326,
     RIGHT_PAREN = 327,
     LEFT_BRACKET = 328,
     RIGHT_BRACKET = 329,
     LEFT_BRACE = 330,
     RIGHT_BRACE = 331,
     DOT = 332,
     COMMA = 333,
     COLON = 334,
     EQUAL = 335,
     SEMICOLON = 336,
     BANG = 337,
     DASH = 338,
     TILDE = 339,
     PLUS = 340,
     STAR = 341,
     SLASH = 342,
     PERCENT = 343,
     LEFT_ANGLE = 344,
     RIGHT_ANGLE = 345,
     VERTICAL_BAR = 346,
     CARET = 347,
     AMPERSAND = 348,
     QUESTION = 349
   };
#endif


#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{


    struct {
        union {
            TString *string;
            float f;
            int i;
            bool b;
        };
        TSymbol* symbol;
    } lex;
    struct {
        TOperator op;
        union {
            TIntermNode* intermNode;
            TIntermNodePair nodePair;
            TIntermTyped* intermTypedNode;
            TIntermAggregate* intermAggregate;
        };
        union {
            TPublicType type;
            TPrecision precision;
            TQualifier qualifier;
            TFunction* function;
            TParameter param;
            TField* field;
            TFieldList* fieldList;
        };
    } interm;



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


#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else 
#if defined __STDC__ || defined __cplusplus
int yyparse (TParseContext* context);
#else
int yyparse ();
#endif
#endif 

#endif 
