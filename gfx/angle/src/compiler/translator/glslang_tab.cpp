










































#define YYBISON 1


#define YYBISON_VERSION "2.7.12-4996"


#define YYSKELETON_NAME "yacc.c"


#define YYPURE 1


#define YYPUSH 0


#define YYPULL 1
















#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wswitch-enum"
#elif defined(_MSC_VER)
#pragma warning(disable: 4065)
#pragma warning(disable: 4189)
#pragma warning(disable: 4505)
#pragma warning(disable: 4701)
#endif

#include "angle_gl.h"
#include "compiler/translator/SymbolTable.h"
#include "compiler/translator/ParseContext.h"
#include "GLSLANG/ShaderLang.h"

#define YYENABLE_NLS 0

#define YYLEX_PARAM context->scanner




# ifndef YY_NULL
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULL nullptr
#  else
#   define YY_NULL 0
#  endif
# endif


#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif



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
     UINT_TYPE = 268,
     BREAK = 269,
     CONTINUE = 270,
     DO = 271,
     ELSE = 272,
     FOR = 273,
     IF = 274,
     DISCARD = 275,
     RETURN = 276,
     SWITCH = 277,
     CASE = 278,
     DEFAULT = 279,
     BVEC2 = 280,
     BVEC3 = 281,
     BVEC4 = 282,
     IVEC2 = 283,
     IVEC3 = 284,
     IVEC4 = 285,
     VEC2 = 286,
     VEC3 = 287,
     VEC4 = 288,
     UVEC2 = 289,
     UVEC3 = 290,
     UVEC4 = 291,
     MATRIX2 = 292,
     MATRIX3 = 293,
     MATRIX4 = 294,
     IN_QUAL = 295,
     OUT_QUAL = 296,
     INOUT_QUAL = 297,
     UNIFORM = 298,
     VARYING = 299,
     MATRIX2x3 = 300,
     MATRIX3x2 = 301,
     MATRIX2x4 = 302,
     MATRIX4x2 = 303,
     MATRIX3x4 = 304,
     MATRIX4x3 = 305,
     CENTROID = 306,
     FLAT = 307,
     SMOOTH = 308,
     STRUCT = 309,
     VOID_TYPE = 310,
     WHILE = 311,
     SAMPLER2D = 312,
     SAMPLERCUBE = 313,
     SAMPLER_EXTERNAL_OES = 314,
     SAMPLER2DRECT = 315,
     SAMPLER2DARRAY = 316,
     ISAMPLER2D = 317,
     ISAMPLER3D = 318,
     ISAMPLERCUBE = 319,
     ISAMPLER2DARRAY = 320,
     USAMPLER2D = 321,
     USAMPLER3D = 322,
     USAMPLERCUBE = 323,
     USAMPLER2DARRAY = 324,
     SAMPLER3D = 325,
     SAMPLER3DRECT = 326,
     SAMPLER2DSHADOW = 327,
     SAMPLERCUBESHADOW = 328,
     SAMPLER2DARRAYSHADOW = 329,
     LAYOUT = 330,
     IDENTIFIER = 331,
     TYPE_NAME = 332,
     FLOATCONSTANT = 333,
     INTCONSTANT = 334,
     UINTCONSTANT = 335,
     BOOLCONSTANT = 336,
     FIELD_SELECTION = 337,
     LEFT_OP = 338,
     RIGHT_OP = 339,
     INC_OP = 340,
     DEC_OP = 341,
     LE_OP = 342,
     GE_OP = 343,
     EQ_OP = 344,
     NE_OP = 345,
     AND_OP = 346,
     OR_OP = 347,
     XOR_OP = 348,
     MUL_ASSIGN = 349,
     DIV_ASSIGN = 350,
     ADD_ASSIGN = 351,
     MOD_ASSIGN = 352,
     LEFT_ASSIGN = 353,
     RIGHT_ASSIGN = 354,
     AND_ASSIGN = 355,
     XOR_ASSIGN = 356,
     OR_ASSIGN = 357,
     SUB_ASSIGN = 358,
     LEFT_PAREN = 359,
     RIGHT_PAREN = 360,
     LEFT_BRACKET = 361,
     RIGHT_BRACKET = 362,
     LEFT_BRACE = 363,
     RIGHT_BRACE = 364,
     DOT = 365,
     COMMA = 366,
     COLON = 367,
     EQUAL = 368,
     SEMICOLON = 369,
     BANG = 370,
     DASH = 371,
     TILDE = 372,
     PLUS = 373,
     STAR = 374,
     SLASH = 375,
     PERCENT = 376,
     LEFT_ANGLE = 377,
     RIGHT_ANGLE = 378,
     VERTICAL_BAR = 379,
     CARET = 380,
     AMPERSAND = 381,
     QUESTION = 382
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
            unsigned int u;
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
            TLayoutQualifier layoutQualifier;
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




extern int yylex(YYSTYPE* yylval, YYLTYPE* yylloc, void* yyscanner);
extern void yyerror(YYLTYPE* yylloc, TParseContext* context, const char* reason);

#define YYLLOC_DEFAULT(Current, Rhs, N)                      \
  do {                                                       \
      if (YYID(N)) {                                         \
        (Current).first_file = YYRHSLOC(Rhs, 1).first_file;  \
        (Current).first_line = YYRHSLOC(Rhs, 1).first_line;  \
        (Current).last_file = YYRHSLOC(Rhs, N).last_file;    \
        (Current).last_line = YYRHSLOC(Rhs, N).last_line;    \
      }                                                      \
      else {                                                 \
        (Current).first_file = YYRHSLOC(Rhs, 0).last_file;   \
        (Current).first_line = YYRHSLOC(Rhs, 0).last_line;   \
        (Current).last_file = YYRHSLOC(Rhs, 0).last_file;    \
        (Current).last_line = YYRHSLOC(Rhs, 0).last_line;    \
      }                                                      \
  } while (0)

#define VERTEX_ONLY(S, L) {  \
    if (context->shaderType != GL_VERTEX_SHADER) {  \
        context->error(L, " supported in vertex shaders only ", S);  \
        context->recover();  \
    }  \
}

#define FRAG_ONLY(S, L) {  \
    if (context->shaderType != GL_FRAGMENT_SHADER) {  \
        context->error(L, " supported in fragment shaders only ", S);  \
        context->recover();  \
    }  \
}

#define ES2_ONLY(S, L) {  \
    if (context->shaderVersion != 100) {  \
        context->error(L, " supported in GLSL ES 1.00 only ", S);  \
        context->recover();  \
    }  \
}

#define ES3_ONLY(TOKEN, LINE, REASON) {  \
    if (context->shaderVersion != 300) {  \
        context->error(LINE, REASON " supported in GLSL ES 3.00 only ", TOKEN);  \
        context->recover();  \
    }  \
}



#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> 
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> 
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef __attribute__

# if (! defined __GNUC__ || __GNUC__ < 2 \
      || (__GNUC__ == 2 && __GNUC_MINOR__ < 5))
#  define __attribute__(Spec)
# endif
#endif


#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E)
#endif



#ifndef lint
# define YYID(N) (N)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE



# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> 
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> 
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> 
      
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    



#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> 
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); 
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); 
#   endif
#  endif
# endif
#endif 


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
	     && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))


union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
  YYLTYPE yyls_alloc;
};


# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)



# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE) + sizeof (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1






# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED


# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (YYID (0))
#  endif
# endif
#endif 


#define YYFINAL  114

#define YYLAST   2375


#define YYNTOKENS  128

#define YYNNTS  91

#define YYNRULES  250

#define YYNSTATES  373


#define YYUNDEFTOK  2
#define YYMAXUTOK   382

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)


static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127
};

#if YYDEBUG


static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     5,     7,     9,    11,    13,    15,    17,
      19,    23,    25,    30,    32,    36,    39,    42,    44,    46,
      48,    52,    55,    58,    61,    63,    66,    70,    73,    75,
      77,    79,    82,    85,    88,    90,    92,    94,    96,   100,
     104,   106,   110,   114,   116,   118,   122,   126,   130,   134,
     136,   140,   144,   146,   148,   150,   152,   156,   158,   162,
     164,   168,   170,   176,   178,   182,   184,   186,   188,   190,
     192,   194,   198,   200,   203,   206,   209,   214,   220,   227,
     237,   240,   243,   245,   247,   250,   254,   258,   261,   267,
     271,   274,   278,   281,   282,   284,   286,   288,   290,   292,
     296,   302,   309,   315,   317,   320,   325,   331,   336,   339,
     341,   344,   346,   348,   350,   352,   354,   357,   359,   362,
     364,   366,   369,   371,   373,   375,   378,   381,   383,   385,
     388,   390,   392,   394,   399,   401,   405,   407,   411,   415,
     417,   422,   424,   426,   428,   430,   432,   434,   436,   438,
     440,   442,   444,   446,   448,   450,   452,   454,   456,   458,
     460,   462,   464,   466,   468,   470,   472,   474,   476,   478,
     480,   482,   484,   486,   488,   490,   492,   494,   496,   498,
     500,   502,   504,   506,   508,   510,   512,   513,   520,   521,
     527,   529,   532,   536,   541,   543,   547,   549,   554,   556,
     558,   560,   562,   564,   566,   568,   570,   572,   575,   576,
     577,   583,   585,   587,   588,   591,   592,   595,   598,   602,
     604,   607,   609,   612,   618,   622,   624,   626,   631,   632,
     639,   640,   649,   650,   658,   660,   662,   664,   665,   668,
     672,   675,   678,   681,   685,   688,   690,   693,   695,   697,
     698
};


static const yytype_int16 yyrhs[] =
{
     215,     0,    -1,    76,    -1,    77,    -1,    76,    -1,   130,
      -1,    79,    -1,    80,    -1,    78,    -1,    81,    -1,   104,
     157,   105,    -1,   131,    -1,   132,   106,   133,   107,    -1,
     134,    -1,   132,   110,   129,    -1,   132,    85,    -1,   132,
      86,    -1,   157,    -1,   135,    -1,   136,    -1,   132,   110,
     136,    -1,   138,   105,    -1,   137,   105,    -1,   139,    55,
      -1,   139,    -1,   139,   155,    -1,   138,   111,   155,    -1,
     140,   104,    -1,   182,    -1,    76,    -1,   132,    -1,    85,
     141,    -1,    86,   141,    -1,   142,   141,    -1,   118,    -1,
     116,    -1,   115,    -1,   141,    -1,   143,   119,   141,    -1,
     143,   120,   141,    -1,   143,    -1,   144,   118,   143,    -1,
     144,   116,   143,    -1,   144,    -1,   145,    -1,   146,   122,
     145,    -1,   146,   123,   145,    -1,   146,    87,   145,    -1,
     146,    88,   145,    -1,   146,    -1,   147,    89,   146,    -1,
     147,    90,   146,    -1,   147,    -1,   148,    -1,   149,    -1,
     150,    -1,   151,    91,   150,    -1,   151,    -1,   152,    93,
     151,    -1,   152,    -1,   153,    92,   152,    -1,   153,    -1,
     153,   127,   157,   112,   155,    -1,   154,    -1,   141,   156,
     155,    -1,   113,    -1,    94,    -1,    95,    -1,    96,    -1,
     103,    -1,   155,    -1,   157,   111,   155,    -1,   154,    -1,
      76,   108,    -1,   161,   114,    -1,   169,   114,    -1,     7,
     177,   181,   114,    -1,   174,   159,   186,   109,   114,    -1,
     174,   159,   186,   109,    76,   114,    -1,   174,   159,   186,
     109,    76,   106,   158,   107,   114,    -1,   174,   114,    -1,
     162,   105,    -1,   164,    -1,   163,    -1,   164,   166,    -1,
     163,   111,   166,    -1,   171,    76,   104,    -1,   176,   129,
      -1,   176,   129,   106,   158,   107,    -1,   173,   167,   165,
      -1,   167,   165,    -1,   173,   167,   168,    -1,   167,   168,
      -1,    -1,    40,    -1,    41,    -1,    42,    -1,   176,    -1,
     170,    -1,   169,   111,   129,    -1,   169,   111,   129,   106,
     107,    -1,   169,   111,   129,   106,   158,   107,    -1,   169,
     111,   129,   113,   190,    -1,   171,    -1,   171,   129,    -1,
     171,   129,   106,   107,    -1,   171,   129,   106,   158,   107,
      -1,   171,   129,   113,   190,    -1,     3,    76,    -1,   176,
      -1,   174,   176,    -1,    53,    -1,    52,    -1,     9,    -1,
       8,    -1,    44,    -1,     3,    44,    -1,   175,    -1,   172,
     175,    -1,   172,    -1,   178,    -1,   178,   175,    -1,     9,
      -1,    40,    -1,    41,    -1,    51,    40,    -1,    51,    41,
      -1,    43,    -1,   181,    -1,   177,   181,    -1,     4,    -1,
       5,    -1,     6,    -1,    75,   104,   179,   105,    -1,   180,
      -1,   179,   111,   180,    -1,    76,    -1,    76,   113,    79,
      -1,    76,   113,    80,    -1,   182,    -1,   182,   106,   158,
     107,    -1,    55,    -1,    11,    -1,    12,    -1,    13,    -1,
      10,    -1,    31,    -1,    32,    -1,    33,    -1,    25,    -1,
      26,    -1,    27,    -1,    28,    -1,    29,    -1,    30,    -1,
      34,    -1,    35,    -1,    36,    -1,    37,    -1,    38,    -1,
      39,    -1,    45,    -1,    46,    -1,    47,    -1,    48,    -1,
      49,    -1,    50,    -1,    57,    -1,    70,    -1,    58,    -1,
      61,    -1,    62,    -1,    63,    -1,    64,    -1,    65,    -1,
      66,    -1,    67,    -1,    68,    -1,    69,    -1,    72,    -1,
      73,    -1,    74,    -1,    59,    -1,    60,    -1,   183,    -1,
      77,    -1,    -1,    54,   129,   108,   184,   186,   109,    -1,
      -1,    54,   108,   185,   186,   109,    -1,   187,    -1,   186,
     187,    -1,   176,   188,   114,    -1,   174,   176,   188,   114,
      -1,   189,    -1,   188,   111,   189,    -1,   129,    -1,   129,
     106,   158,   107,    -1,   155,    -1,   160,    -1,   194,    -1,
     193,    -1,   191,    -1,   203,    -1,   204,    -1,   207,    -1,
     214,    -1,   108,   109,    -1,    -1,    -1,   108,   195,   202,
     196,   109,    -1,   201,    -1,   193,    -1,    -1,   199,   201,
      -1,    -1,   200,   193,    -1,   108,   109,    -1,   108,   202,
     109,    -1,   192,    -1,   202,   192,    -1,   114,    -1,   157,
     114,    -1,    19,   104,   157,   105,   205,    -1,   198,    17,
     198,    -1,   198,    -1,   157,    -1,   171,   129,   113,   190,
      -1,    -1,    56,   104,   208,   206,   105,   197,    -1,    -1,
      16,   209,   198,    56,   104,   157,   105,   114,    -1,    -1,
      18,   104,   210,   211,   213,   105,   197,    -1,   203,    -1,
     191,    -1,   206,    -1,    -1,   212,   114,    -1,   212,   114,
     157,    -1,    15,   114,    -1,    14,   114,    -1,    21,   114,
      -1,    21,   157,   114,    -1,    20,   114,    -1,   216,    -1,
     215,   216,    -1,   217,    -1,   160,    -1,    -1,   161,   218,
     201,    -1
};


static const yytype_uint16 yyrline[] =
{
       0,   206,   206,   207,   210,   234,   237,   242,   247,   252,
     257,   263,   266,   269,   272,   275,   285,   298,   306,   423,
     426,   434,   437,   443,   447,   454,   460,   469,   477,   480,
     490,   493,   503,   513,   535,   536,   537,   542,   543,   551,
     562,   563,   571,   582,   586,   587,   597,   607,   617,   630,
     631,   641,   654,   658,   662,   666,   667,   680,   681,   694,
     695,   708,   709,   726,   727,   740,   741,   742,   743,   744,
     748,   751,   762,   770,   778,   805,   811,   822,   826,   830,
     834,   841,   897,   900,   907,   915,   936,   957,   967,   995,
    1000,  1010,  1015,  1025,  1028,  1031,  1034,  1040,  1047,  1050,
    1054,  1058,  1062,  1069,  1073,  1077,  1084,  1088,  1092,  1099,
    1108,  1114,  1117,  1123,  1129,  1136,  1145,  1154,  1162,  1165,
    1172,  1176,  1183,  1186,  1190,  1194,  1203,  1212,  1220,  1230,
    1242,  1245,  1248,  1254,  1261,  1264,  1270,  1273,  1276,  1282,
    1285,  1300,  1304,  1308,  1312,  1316,  1320,  1325,  1330,  1335,
    1340,  1345,  1350,  1355,  1360,  1365,  1370,  1375,  1380,  1385,
    1390,  1395,  1400,  1405,  1410,  1415,  1420,  1425,  1429,  1433,
    1437,  1441,  1445,  1449,  1453,  1457,  1461,  1465,  1469,  1473,
    1477,  1481,  1485,  1493,  1501,  1505,  1518,  1518,  1521,  1521,
    1527,  1530,  1546,  1549,  1558,  1562,  1568,  1575,  1590,  1594,
    1598,  1599,  1605,  1606,  1607,  1608,  1609,  1613,  1614,  1614,
    1614,  1624,  1625,  1629,  1629,  1630,  1630,  1635,  1638,  1648,
    1651,  1657,  1658,  1662,  1670,  1674,  1684,  1689,  1706,  1706,
    1711,  1711,  1718,  1718,  1726,  1729,  1735,  1738,  1744,  1748,
    1755,  1762,  1769,  1776,  1787,  1796,  1800,  1807,  1810,  1816,
    1816
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0


static const char *const yytname[] =
{
  "$end", "error", "$undefined", "INVARIANT", "HIGH_PRECISION",
  "MEDIUM_PRECISION", "LOW_PRECISION", "PRECISION", "ATTRIBUTE",
  "CONST_QUAL", "BOOL_TYPE", "FLOAT_TYPE", "INT_TYPE", "UINT_TYPE",
  "BREAK", "CONTINUE", "DO", "ELSE", "FOR", "IF", "DISCARD", "RETURN",
  "SWITCH", "CASE", "DEFAULT", "BVEC2", "BVEC3", "BVEC4", "IVEC2", "IVEC3",
  "IVEC4", "VEC2", "VEC3", "VEC4", "UVEC2", "UVEC3", "UVEC4", "MATRIX2",
  "MATRIX3", "MATRIX4", "IN_QUAL", "OUT_QUAL", "INOUT_QUAL", "UNIFORM",
  "VARYING", "MATRIX2x3", "MATRIX3x2", "MATRIX2x4", "MATRIX4x2",
  "MATRIX3x4", "MATRIX4x3", "CENTROID", "FLAT", "SMOOTH", "STRUCT",
  "VOID_TYPE", "WHILE", "SAMPLER2D", "SAMPLERCUBE", "SAMPLER_EXTERNAL_OES",
  "SAMPLER2DRECT", "SAMPLER2DARRAY", "ISAMPLER2D", "ISAMPLER3D",
  "ISAMPLERCUBE", "ISAMPLER2DARRAY", "USAMPLER2D", "USAMPLER3D",
  "USAMPLERCUBE", "USAMPLER2DARRAY", "SAMPLER3D", "SAMPLER3DRECT",
  "SAMPLER2DSHADOW", "SAMPLERCUBESHADOW", "SAMPLER2DARRAYSHADOW", "LAYOUT",
  "IDENTIFIER", "TYPE_NAME", "FLOATCONSTANT", "INTCONSTANT",
  "UINTCONSTANT", "BOOLCONSTANT", "FIELD_SELECTION", "LEFT_OP", "RIGHT_OP",
  "INC_OP", "DEC_OP", "LE_OP", "GE_OP", "EQ_OP", "NE_OP", "AND_OP",
  "OR_OP", "XOR_OP", "MUL_ASSIGN", "DIV_ASSIGN", "ADD_ASSIGN",
  "MOD_ASSIGN", "LEFT_ASSIGN", "RIGHT_ASSIGN", "AND_ASSIGN", "XOR_ASSIGN",
  "OR_ASSIGN", "SUB_ASSIGN", "LEFT_PAREN", "RIGHT_PAREN", "LEFT_BRACKET",
  "RIGHT_BRACKET", "LEFT_BRACE", "RIGHT_BRACE", "DOT", "COMMA", "COLON",
  "EQUAL", "SEMICOLON", "BANG", "DASH", "TILDE", "PLUS", "STAR", "SLASH",
  "PERCENT", "LEFT_ANGLE", "RIGHT_ANGLE", "VERTICAL_BAR", "CARET",
  "AMPERSAND", "QUESTION", "$accept", "identifier", "variable_identifier",
  "primary_expression", "postfix_expression", "integer_expression",
  "function_call", "function_call_or_method", "function_call_generic",
  "function_call_header_no_parameters",
  "function_call_header_with_parameters", "function_call_header",
  "function_identifier", "unary_expression", "unary_operator",
  "multiplicative_expression", "additive_expression", "shift_expression",
  "relational_expression", "equality_expression", "and_expression",
  "exclusive_or_expression", "inclusive_or_expression",
  "logical_and_expression", "logical_xor_expression",
  "logical_or_expression", "conditional_expression",
  "assignment_expression", "assignment_operator", "expression",
  "constant_expression", "enter_struct", "declaration",
  "function_prototype", "function_declarator",
  "function_header_with_parameters", "function_header",
  "parameter_declarator", "parameter_declaration", "parameter_qualifier",
  "parameter_type_specifier", "init_declarator_list", "single_declaration",
  "fully_specified_type", "interpolation_qualifier",
  "parameter_type_qualifier", "type_qualifier", "storage_qualifier",
  "type_specifier", "precision_qualifier", "layout_qualifier",
  "layout_qualifier_id_list", "layout_qualifier_id",
  "type_specifier_no_prec", "type_specifier_nonarray", "struct_specifier",
  "$@1", "$@2", "struct_declaration_list", "struct_declaration",
  "struct_declarator_list", "struct_declarator", "initializer",
  "declaration_statement", "statement", "simple_statement",
  "compound_statement", "$@3", "$@4", "statement_no_new_scope",
  "statement_with_scope", "$@5", "$@6", "compound_statement_no_new_scope",
  "statement_list", "expression_statement", "selection_statement",
  "selection_rest_statement", "condition", "iteration_statement", "$@7",
  "$@8", "$@9", "for_init_statement", "conditionopt", "for_rest_statement",
  "jump_statement", "translation_unit", "external_declaration",
  "function_definition", "$@10", YY_NULL
};
#endif

# ifdef YYPRINT


static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
     365,   366,   367,   368,   369,   370,   371,   372,   373,   374,
     375,   376,   377,   378,   379,   380,   381,   382
};
# endif


static const yytype_uint8 yyr1[] =
{
       0,   128,   129,   129,   130,   131,   131,   131,   131,   131,
     131,   132,   132,   132,   132,   132,   132,   133,   134,   135,
     135,   136,   136,   137,   137,   138,   138,   139,   140,   140,
     141,   141,   141,   141,   142,   142,   142,   143,   143,   143,
     144,   144,   144,   145,   146,   146,   146,   146,   146,   147,
     147,   147,   148,   149,   150,   151,   151,   152,   152,   153,
     153,   154,   154,   155,   155,   156,   156,   156,   156,   156,
     157,   157,   158,   159,   160,   160,   160,   160,   160,   160,
     160,   161,   162,   162,   163,   163,   164,   165,   165,   166,
     166,   166,   166,   167,   167,   167,   167,   168,   169,   169,
     169,   169,   169,   170,   170,   170,   170,   170,   170,   171,
     171,   172,   172,   173,   174,   174,   174,   174,   174,   174,
     174,   174,   175,   175,   175,   175,   175,   175,   176,   176,
     177,   177,   177,   178,   179,   179,   180,   180,   180,   181,
     181,   182,   182,   182,   182,   182,   182,   182,   182,   182,
     182,   182,   182,   182,   182,   182,   182,   182,   182,   182,
     182,   182,   182,   182,   182,   182,   182,   182,   182,   182,
     182,   182,   182,   182,   182,   182,   182,   182,   182,   182,
     182,   182,   182,   182,   182,   182,   184,   183,   185,   183,
     186,   186,   187,   187,   188,   188,   189,   189,   190,   191,
     192,   192,   193,   193,   193,   193,   193,   194,   195,   196,
     194,   197,   197,   199,   198,   200,   198,   201,   201,   202,
     202,   203,   203,   204,   205,   205,   206,   206,   208,   207,
     209,   207,   210,   207,   211,   211,   212,   212,   213,   213,
     214,   214,   214,   214,   214,   215,   215,   216,   216,   218,
     217
};


static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     1,     1,     1,     1,     1,     1,
       3,     1,     4,     1,     3,     2,     2,     1,     1,     1,
       3,     2,     2,     2,     1,     2,     3,     2,     1,     1,
       1,     2,     2,     2,     1,     1,     1,     1,     3,     3,
       1,     3,     3,     1,     1,     3,     3,     3,     3,     1,
       3,     3,     1,     1,     1,     1,     3,     1,     3,     1,
       3,     1,     5,     1,     3,     1,     1,     1,     1,     1,
       1,     3,     1,     2,     2,     2,     4,     5,     6,     9,
       2,     2,     1,     1,     2,     3,     3,     2,     5,     3,
       2,     3,     2,     0,     1,     1,     1,     1,     1,     3,
       5,     6,     5,     1,     2,     4,     5,     4,     2,     1,
       2,     1,     1,     1,     1,     1,     2,     1,     2,     1,
       1,     2,     1,     1,     1,     2,     2,     1,     1,     2,
       1,     1,     1,     4,     1,     3,     1,     3,     3,     1,
       4,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     0,     6,     0,     5,
       1,     2,     3,     4,     1,     3,     1,     4,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     2,     0,     0,
       5,     1,     1,     0,     2,     0,     2,     2,     3,     1,
       2,     1,     2,     5,     3,     1,     1,     4,     0,     6,
       0,     8,     0,     7,     1,     1,     1,     0,     2,     3,
       2,     2,     2,     3,     2,     1,     2,     1,     1,     0,
       3
};




static const yytype_uint8 yydefact[] =
{
       0,     0,   130,   131,   132,     0,   114,   122,   145,   142,
     143,   144,   149,   150,   151,   152,   153,   154,   146,   147,
     148,   155,   156,   157,   158,   159,   160,   123,   124,   127,
     115,   161,   162,   163,   164,   165,   166,     0,   112,   111,
       0,   141,   167,   169,   182,   183,   170,   171,   172,   173,
     174,   175,   176,   177,   178,   168,   179,   180,   181,     0,
     185,   248,   249,     0,    83,    93,     0,    98,   103,   119,
       0,   117,   109,     0,   120,   128,   139,   184,     0,   245,
     247,   116,   108,     0,   125,   126,     2,     3,   188,     0,
       0,    74,     0,    81,    93,   113,    94,    95,    96,    84,
       0,    93,     0,    75,     2,   104,   118,     0,    80,     0,
     110,   129,   121,     0,     1,   246,     0,     0,   186,   136,
       0,   134,     0,   250,    85,    90,    92,    97,     0,    99,
      86,     0,     0,    73,     0,     0,     0,     0,   190,     4,
       8,     6,     7,     9,     0,     0,     0,    36,    35,    34,
       5,    11,    30,    13,    18,    19,     0,     0,    24,     0,
      37,     0,    40,    43,    44,    49,    52,    53,    54,    55,
      57,    59,    61,    72,     0,    28,    76,     0,     0,     0,
     133,     0,     0,     0,   230,     0,     0,     0,     0,     0,
     208,   217,   221,    37,    63,    70,     0,   199,     0,   139,
     202,   219,   201,   200,     0,   203,   204,   205,   206,    87,
      89,    91,     0,     0,   105,     0,   198,   107,     0,   196,
       0,   194,     0,   191,    31,    32,     0,    15,    16,     0,
       0,    22,    21,     0,    23,    25,    27,    33,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   140,   189,     0,   137,   138,   135,   241,   240,
     215,   232,     0,   244,   242,     0,   228,   207,     0,    66,
      67,    68,    69,    65,     0,     0,   222,   218,   220,     0,
     100,     0,   102,   106,     0,     0,     0,   192,     0,    77,
      10,     0,    17,     2,     3,    14,    20,    26,    38,    39,
      42,    41,    47,    48,    45,    46,    50,    51,    56,    58,
      60,     0,   187,     0,     0,     0,     0,     0,   243,     0,
     209,    64,    71,     0,   101,   193,     0,   195,     0,    78,
      12,     0,     0,   214,   216,   235,   234,   237,   215,   226,
       0,     0,     0,     0,    88,   197,     0,    62,     0,   236,
       0,     0,   225,   223,     0,     0,   210,     0,     0,   238,
       0,   215,     0,   212,   229,   211,    79,     0,   239,   233,
     224,   227,   231
};


static const yytype_int16 yydefgoto[] =
{
      -1,   219,   150,   151,   152,   291,   153,   154,   155,   156,
     157,   158,   159,   193,   161,   162,   163,   164,   165,   166,
     167,   168,   169,   170,   171,   172,   194,   195,   274,   196,
     174,   109,   197,   198,    63,    64,    65,   125,    99,   100,
     126,    66,    67,    68,    69,   101,    70,    71,    72,    73,
      74,   120,   121,    75,   175,    77,   178,   117,   137,   138,
     220,   221,   217,   200,   201,   202,   203,   268,   343,   364,
     313,   314,   315,   365,   204,   205,   206,   353,   342,   207,
     319,   260,   316,   337,   350,   351,   208,    78,    79,    80,
      92
};



#define YYPACT_NINF -309
static const yytype_int16 yypact[] =
{
    2013,   -27,  -309,  -309,  -309,   154,  -309,  -309,  -309,  -309,
    -309,  -309,  -309,  -309,  -309,  -309,  -309,  -309,  -309,  -309,
    -309,  -309,  -309,  -309,  -309,  -309,  -309,  -309,  -309,  -309,
    -309,  -309,  -309,  -309,  -309,  -309,  -309,    98,  -309,  -309,
     -40,  -309,  -309,  -309,  -309,  -309,  -309,  -309,  -309,  -309,
    -309,  -309,  -309,  -309,  -309,  -309,  -309,  -309,  -309,   -46,
    -309,  -309,   -42,   -23,     8,     5,   -87,  -309,    91,    14,
    1130,  -309,  -309,  2298,    14,  -309,    -1,  -309,  1938,  -309,
    -309,  -309,  -309,  2298,  -309,  -309,  -309,  -309,  -309,    13,
      47,  -309,    43,  -309,    39,  -309,  -309,  -309,  -309,  -309,
    2162,   124,    94,  -309,    51,   -14,  -309,    66,  -309,  2088,
    -309,  -309,  -309,  1491,  -309,  -309,    62,  2088,  -309,    48,
     -83,  -309,   358,  -309,  -309,  -309,  -309,    94,  2162,    -9,
    -309,  1200,  1491,  -309,   148,  2162,    94,  1683,  -309,    89,
    -309,  -309,  -309,  -309,  1491,  1491,  1491,  -309,  -309,  -309,
    -309,  -309,    10,  -309,  -309,  -309,    92,    20,  1586,    96,
    -309,  1491,    53,   -76,  -309,   -62,    90,  -309,  -309,  -309,
     104,   101,   -61,  -309,    95,  -309,  -309,  1768,  2088,   103,
    -309,    47,    82,    84,  -309,    97,    99,    93,  1298,   105,
     102,  -309,  -309,   -10,  -309,  -309,   -13,  -309,   -42,     2,
    -309,  -309,  -309,  -309,   474,  -309,  -309,  -309,  -309,   106,
    -309,  -309,  1393,  1491,  -309,   107,  -309,  -309,    94,   109,
       4,  -309,   -58,  -309,  -309,  -309,    22,  -309,  -309,  1491,
    2230,  -309,  -309,  1491,   112,  -309,  -309,  -309,  1491,  1491,
    1491,  1491,  1491,  1491,  1491,  1491,  1491,  1491,  1491,  1491,
    1491,  1491,  -309,  -309,  1853,  -309,  -309,  -309,  -309,  -309,
     100,  -309,  1491,  -309,  -309,    36,  -309,  -309,   590,  -309,
    -309,  -309,  -309,  -309,  1491,  1491,  -309,  -309,  -309,  1491,
    -309,   113,  -309,  -309,    42,  1491,    94,  -309,   -73,  -309,
    -309,   115,   108,    89,   119,  -309,  -309,  -309,  -309,  -309,
      53,    53,  -309,  -309,  -309,  -309,   -62,   -62,  -309,   104,
     101,    73,  -309,   169,    43,   822,   938,    25,  -309,  1035,
     590,  -309,  -309,   120,  -309,  -309,   121,  -309,  1491,  -309,
    -309,  1491,   122,  -309,  -309,  -309,  -309,  1035,   100,   108,
      94,  2162,   125,   123,  -309,  -309,   126,  -309,  1491,  -309,
     117,   129,   212,  -309,   139,   706,  -309,   141,    29,  1491,
     706,   100,  1491,  -309,  -309,  -309,  -309,   142,   108,  -309,
    -309,  -309,  -309
};


static const yytype_int16 yypgoto[] =
{
    -309,   -39,  -309,  -309,  -309,  -309,  -309,  -309,     7,  -309,
    -309,  -309,  -309,     1,  -309,   -54,  -309,  -101,   -57,  -309,
    -309,  -309,     9,   -11,     3,  -309,  -110,  -126,  -309,  -138,
    -122,  -309,    11,    16,  -309,  -309,  -309,   130,   165,   159,
     133,  -309,  -309,  -299,  -309,  -309,  -102,   -30,   -66,   257,
    -309,  -309,    83,    -6,     0,  -309,  -309,  -309,  -104,  -125,
      45,   -21,  -208,   -50,  -194,  -296,  -309,  -309,  -309,   -93,
    -308,  -309,  -309,   -90,     6,   -47,  -309,  -309,   -67,  -309,
    -309,  -309,  -309,  -309,  -309,  -309,  -309,  -309,   193,  -309,
    -309
};




#define YYTABLE_NINF -214
static const yytype_int16 yytable[] =
{
      76,    89,   123,   173,   110,   282,   216,   135,   226,   215,
     278,    61,   223,   177,    95,   135,    62,    81,   288,   334,
     340,   173,   180,     7,   102,   242,   243,   103,   181,   105,
     352,   250,   235,   328,   127,   135,    86,    87,   340,   106,
     240,   329,   241,   136,   112,    96,    97,    98,    95,    82,
     265,   136,   223,   370,    27,    28,   289,    29,    90,   363,
     244,   245,   127,   129,   363,    37,   251,   111,    88,   218,
      76,   136,    91,    76,   254,   135,   135,   116,    76,    96,
      97,    98,    93,    76,   269,   270,   271,   216,   209,    61,
     281,   292,   131,   272,    62,   227,   228,   212,   275,   132,
      76,   276,   173,   273,   213,   113,   -28,   297,   113,    76,
     -82,   136,   136,   311,   160,   286,   229,    76,   287,    94,
     230,   118,   199,   119,   317,   232,   278,   290,    76,   223,
     338,   233,   160,   275,   367,    76,   275,    76,    84,    85,
     275,   302,   303,   304,   305,   224,   225,   275,   321,   322,
     318,   122,   135,   286,   371,   130,   325,   323,     2,     3,
       4,   179,   237,   326,    96,    97,    98,   104,    87,   173,
      86,    87,   238,   239,   133,   173,   176,    76,    76,   246,
     247,   339,   255,   256,   275,   331,   300,   301,   136,   306,
     307,   295,    81,   -29,   249,   248,   258,   231,   259,   339,
     236,   261,   252,   262,   199,   347,   346,   263,  -213,   266,
     358,   267,   279,   160,   283,   285,  -141,   341,   173,   275,
     324,   368,   330,  -185,   333,   332,   348,   344,   345,   361,
     355,   359,   356,   357,   360,   341,   216,   296,   309,   298,
     299,   160,   160,   160,   160,   160,   160,   160,   160,   160,
     160,   160,   362,   310,    76,   366,   372,   308,   210,   124,
     128,   211,    83,   284,   257,   327,   335,   369,   199,   336,
     349,   115,     0,     0,   320,   110,     0,     0,     0,     0,
     160,     0,     0,     0,     0,     0,   160,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   354,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   199,   199,     0,     0,   199,
     199,     0,     0,     0,     0,     0,     0,     0,     0,   160,
       0,     0,     0,     0,     0,     0,     0,   199,     0,     0,
       0,    76,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   199,     0,     0,     0,     0,
     199,     1,     2,     3,     4,     5,     6,     7,     8,     9,
      10,    11,   182,   183,   184,     0,   185,   186,   187,   188,
       0,     0,     0,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
       0,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,   189,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,     0,
      56,    57,    58,    59,   139,    60,   140,   141,   142,   143,
       0,     0,     0,   144,   145,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   146,     0,     0,     0,   190,   191,     0,     0,
       0,     0,   192,   147,   148,     0,   149,     1,     2,     3,
       4,     5,     6,     7,     8,     9,    10,    11,   182,   183,
     184,     0,   185,   186,   187,   188,     0,     0,     0,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,     0,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
     189,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,     0,    56,    57,    58,    59,
     139,    60,   140,   141,   142,   143,     0,     0,     0,   144,
     145,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   146,     0,
       0,     0,   190,   277,     0,     0,     0,     0,   192,   147,
     148,     0,   149,     1,     2,     3,     4,     5,     6,     7,
       8,     9,    10,    11,   182,   183,   184,     0,   185,   186,
     187,   188,     0,     0,     0,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,     0,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,   189,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,     0,    56,    57,    58,    59,   139,    60,   140,   141,
     142,   143,     0,     0,     0,   144,   145,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   146,     0,     0,     0,   190,     0,
       0,     0,     0,     0,   192,   147,   148,     0,   149,     1,
       2,     3,     4,     5,     6,     7,     8,     9,    10,    11,
     182,   183,   184,     0,   185,   186,   187,   188,     0,     0,
       0,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,     0,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,   189,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,     0,    56,    57,
      58,    59,   139,    60,   140,   141,   142,   143,     0,     0,
       0,   144,   145,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     146,     0,     0,     0,   122,     0,     0,     0,     0,     0,
     192,   147,   148,     0,   149,     1,     2,     3,     4,     5,
       6,     7,     8,     9,    10,    11,   182,   183,   184,     0,
     185,   186,   187,   188,     0,     0,     0,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,     0,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,   189,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,     0,    56,    57,    58,    59,   139,    60,
     140,   141,   142,   143,     0,     0,     0,   144,   145,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   146,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   192,   147,   148,     0,
     149,     1,     2,     3,     4,     5,     6,     7,     8,     9,
      10,    11,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
       0,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,     0,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,     0,
      56,    57,    58,    59,   139,    60,   140,   141,   142,   143,
       0,     0,     0,   144,   145,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   134,     2,
       3,     4,   146,     6,     7,     8,     9,    10,    11,     0,
       0,     0,   192,   147,   148,     0,   149,     0,     0,     0,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,     0,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,     0,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,     0,    56,    57,    58,
      59,   139,    60,   140,   141,   142,   143,     0,     0,     0,
     144,   145,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     2,     3,     4,     0,     0,   146,
       8,     9,    10,    11,     0,     0,     0,     0,     0,     0,
     147,   148,     0,   149,     0,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
       0,     0,     0,     0,     0,    31,    32,    33,    34,    35,
      36,     0,     0,     0,    40,    41,     0,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,     0,    56,    57,    58,     0,   107,    60,     0,     0,
       8,     9,    10,    11,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
       0,     0,     0,     0,   108,    31,    32,    33,    34,    35,
      36,     0,     0,     0,    40,    41,     0,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,     0,    56,    57,    58,     0,   139,    60,   140,   141,
     142,   143,     0,     0,     0,   144,   145,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   146,     0,     0,   214,     8,     9,
      10,    11,     0,     0,     0,   147,   148,     0,   149,     0,
       0,     0,     0,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,     0,     0,
       0,     0,     0,    31,    32,    33,    34,    35,    36,     0,
       0,     0,    40,    41,     0,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,     0,
      56,    57,    58,     0,   139,    60,   140,   141,   142,   143,
       0,     0,     0,   144,   145,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   146,     8,     9,    10,    11,     0,     0,     0,
       0,     0,   264,   147,   148,     0,   149,     0,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,     0,     0,     0,     0,     0,    31,    32,
      33,    34,    35,    36,     0,     0,     0,    40,    41,     0,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,     0,    56,    57,    58,     0,   139,
      60,   140,   141,   142,   143,     0,     0,     0,   144,   145,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   146,     0,     0,
     280,     8,     9,    10,    11,     0,     0,     0,   147,   148,
       0,   149,     0,     0,     0,     0,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,     0,     0,     0,     0,     0,    31,    32,    33,    34,
      35,    36,     0,     0,     0,    40,    41,     0,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,     0,    56,    57,    58,     0,   139,    60,   140,
     141,   142,   143,     0,     0,     0,   144,   145,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   146,     8,     9,    10,    11,
       0,     0,     0,     0,     0,     0,   147,   148,     0,   149,
       0,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,     0,     0,     0,     0,
       0,    31,    32,    33,    34,    35,    36,     0,     0,     0,
      40,   234,     0,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,     0,    56,    57,
      58,     0,   139,    60,   140,   141,   142,   143,     0,     0,
       0,   144,   145,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   134,     2,     3,     4,
     146,     6,     7,     8,     9,    10,    11,     0,     0,     0,
       0,   147,   148,     0,   149,     0,     0,     0,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,     0,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,     0,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,     0,    56,    57,    58,    59,     0,
      60,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   134,     2,     3,     4,     0,     6,     7,     8,     9,
      10,    11,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   222,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
       0,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,     0,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,     0,
      56,    57,    58,    59,     0,    60,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   134,     2,     3,     4,
       0,     6,     7,     8,     9,    10,    11,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   253,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,     0,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,     0,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,     0,    56,    57,    58,    59,     0,
      60,     0,     0,     0,     0,     0,     0,     0,   114,     0,
       0,     1,     2,     3,     4,     5,     6,     7,     8,     9,
      10,    11,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   312,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
       0,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,     0,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,     0,
      56,    57,    58,    59,     0,    60,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,     0,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,     0,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,     0,    56,    57,    58,    59,     0,
      60,   134,     2,     3,     4,     0,     6,     7,     8,     9,
      10,    11,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
       0,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,     0,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,     0,
      56,    57,    58,    59,     0,    60,     2,     3,     4,     0,
       0,     0,     8,     9,    10,    11,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,     0,     0,     0,     0,     0,    31,    32,    33,
      34,    35,    36,     0,     0,     0,    40,    41,     0,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,     0,    56,    57,    58,     0,     0,    60,
       8,     9,    10,    11,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
       0,     0,     0,     0,     0,    31,    32,    33,    34,    35,
      36,     0,     0,     0,    40,    41,     0,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,     0,    56,    57,    58,     0,   293,   294,     8,     9,
      10,    11,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,     0,     0,
       0,     0,     0,    31,    32,    33,    34,    35,    36,     0,
       0,     0,    40,    41,     0,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,     0,
      56,    57,    58,     0,     0,    60
};

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-309)))

#define yytable_value_is_error(Yytable_value) \
  YYID (0)

static const yytype_int16 yycheck[] =
{
       0,    40,    92,   113,    70,   213,   132,   109,   146,   131,
     204,     0,   137,   117,     9,   117,     0,    44,    76,   315,
     319,   131,   105,     9,   111,    87,    88,   114,   111,    68,
     338,    92,   158,   106,   100,   137,    76,    77,   337,    69,
     116,   114,   118,   109,    74,    40,    41,    42,     9,    76,
     188,   117,   177,   361,    40,    41,   114,    43,   104,   355,
     122,   123,   128,   102,   360,    51,   127,    73,   108,   135,
      70,   137,   114,    73,   178,   177,   178,    83,    78,    40,
      41,    42,   105,    83,    94,    95,    96,   213,   127,    78,
     212,   229,   106,   103,    78,    85,    86,   106,   111,   113,
     100,   114,   212,   113,   113,   106,   104,   233,   106,   109,
     105,   177,   178,   251,   113,   111,   106,   117,   114,   111,
     110,   108,   122,    76,   262,   105,   320,   105,   128,   254,
     105,   111,   131,   111,   105,   135,   111,   137,    40,    41,
     111,   242,   243,   244,   245,   144,   145,   111,   274,   275,
     114,   108,   254,   111,   362,   104,   114,   279,     4,     5,
       6,   113,   161,   285,    40,    41,    42,    76,    77,   279,
      76,    77,   119,   120,   108,   285,   114,   177,   178,    89,
      90,   319,    79,    80,   111,   112,   240,   241,   254,   246,
     247,   230,    44,   104,    93,    91,   114,   105,   114,   337,
     104,   104,   107,   104,   204,   331,   328,   114,   108,   104,
     348,   109,   106,   212,   107,   106,   104,   319,   328,   111,
     107,   359,   107,   104,   314,    56,   104,   107,   107,    17,
     105,   114,   109,   107,   105,   337,   362,   230,   249,   238,
     239,   240,   241,   242,   243,   244,   245,   246,   247,   248,
     249,   250,   113,   250,   254,   114,   114,   248,   128,    94,
     101,   128,     5,   218,   181,   286,   316,   360,   268,   316,
     337,    78,    -1,    -1,   268,   341,    -1,    -1,    -1,    -1,
     279,    -1,    -1,    -1,    -1,    -1,   285,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   340,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   315,   316,    -1,    -1,   319,
     320,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   328,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   337,    -1,    -1,
      -1,   341,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   355,    -1,    -1,    -1,    -1,
     360,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    -1,    18,    19,    20,    21,
      -1,    -1,    -1,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      -1,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    -1,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      -1,    -1,    -1,    85,    86,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   104,    -1,    -1,    -1,   108,   109,    -1,    -1,
      -1,    -1,   114,   115,   116,    -1,   118,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    -1,    18,    19,    20,    21,    -1,    -1,    -1,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    -1,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    -1,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    -1,    -1,    -1,    85,
      86,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   104,    -1,
      -1,    -1,   108,   109,    -1,    -1,    -1,    -1,   114,   115,
     116,    -1,   118,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    -1,    18,    19,
      20,    21,    -1,    -1,    -1,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    -1,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    -1,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    -1,    -1,    -1,    85,    86,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   104,    -1,    -1,    -1,   108,    -1,
      -1,    -1,    -1,    -1,   114,   115,   116,    -1,   118,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    -1,    18,    19,    20,    21,    -1,    -1,
      -1,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    -1,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    -1,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    -1,    -1,
      -1,    85,    86,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     104,    -1,    -1,    -1,   108,    -1,    -1,    -1,    -1,    -1,
     114,   115,   116,    -1,   118,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    -1,
      18,    19,    20,    21,    -1,    -1,    -1,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    -1,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    -1,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    -1,    -1,    -1,    85,    86,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   104,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   114,   115,   116,    -1,
     118,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      -1,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    -1,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    -1,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      -1,    -1,    -1,    85,    86,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     3,     4,
       5,     6,   104,     8,     9,    10,    11,    12,    13,    -1,
      -1,    -1,   114,   115,   116,    -1,   118,    -1,    -1,    -1,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    -1,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    -1,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    -1,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    -1,    -1,    -1,
      85,    86,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,     4,     5,     6,    -1,    -1,   104,
      10,    11,    12,    13,    -1,    -1,    -1,    -1,    -1,    -1,
     115,   116,    -1,   118,    -1,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      -1,    -1,    -1,    -1,    -1,    45,    46,    47,    48,    49,
      50,    -1,    -1,    -1,    54,    55,    -1,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    -1,    72,    73,    74,    -1,    76,    77,    -1,    -1,
      10,    11,    12,    13,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      -1,    -1,    -1,    -1,   114,    45,    46,    47,    48,    49,
      50,    -1,    -1,    -1,    54,    55,    -1,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    -1,    72,    73,    74,    -1,    76,    77,    78,    79,
      80,    81,    -1,    -1,    -1,    85,    86,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   104,    -1,    -1,   107,    10,    11,
      12,    13,    -1,    -1,    -1,   115,   116,    -1,   118,    -1,
      -1,    -1,    -1,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    -1,    -1,
      -1,    -1,    -1,    45,    46,    47,    48,    49,    50,    -1,
      -1,    -1,    54,    55,    -1,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    -1,
      72,    73,    74,    -1,    76,    77,    78,    79,    80,    81,
      -1,    -1,    -1,    85,    86,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   104,    10,    11,    12,    13,    -1,    -1,    -1,
      -1,    -1,   114,   115,   116,    -1,   118,    -1,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    -1,    -1,    -1,    -1,    -1,    45,    46,
      47,    48,    49,    50,    -1,    -1,    -1,    54,    55,    -1,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    -1,    72,    73,    74,    -1,    76,
      77,    78,    79,    80,    81,    -1,    -1,    -1,    85,    86,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   104,    -1,    -1,
     107,    10,    11,    12,    13,    -1,    -1,    -1,   115,   116,
      -1,   118,    -1,    -1,    -1,    -1,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    -1,    -1,    -1,    -1,    -1,    45,    46,    47,    48,
      49,    50,    -1,    -1,    -1,    54,    55,    -1,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    -1,    72,    73,    74,    -1,    76,    77,    78,
      79,    80,    81,    -1,    -1,    -1,    85,    86,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   104,    10,    11,    12,    13,
      -1,    -1,    -1,    -1,    -1,    -1,   115,   116,    -1,   118,
      -1,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    -1,    -1,    -1,    -1,
      -1,    45,    46,    47,    48,    49,    50,    -1,    -1,    -1,
      54,    55,    -1,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    -1,    72,    73,
      74,    -1,    76,    77,    78,    79,    80,    81,    -1,    -1,
      -1,    85,    86,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,     3,     4,     5,     6,
     104,     8,     9,    10,    11,    12,    13,    -1,    -1,    -1,
      -1,   115,   116,    -1,   118,    -1,    -1,    -1,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    -1,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    -1,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    -1,    72,    73,    74,    75,    -1,
      77,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,     3,     4,     5,     6,    -1,     8,     9,    10,    11,
      12,    13,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   109,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      -1,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    -1,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    -1,
      72,    73,    74,    75,    -1,    77,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,     3,     4,     5,     6,
      -1,     8,     9,    10,    11,    12,    13,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   109,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    -1,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    -1,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    -1,    72,    73,    74,    75,    -1,
      77,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     0,    -1,
      -1,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   109,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      -1,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    -1,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    -1,
      72,    73,    74,    75,    -1,    77,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    -1,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    -1,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    -1,    72,    73,    74,    75,    -1,
      77,     3,     4,     5,     6,    -1,     8,     9,    10,    11,
      12,    13,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      -1,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    -1,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    -1,
      72,    73,    74,    75,    -1,    77,     4,     5,     6,    -1,
      -1,    -1,    10,    11,    12,    13,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    -1,    -1,    -1,    -1,    -1,    45,    46,    47,
      48,    49,    50,    -1,    -1,    -1,    54,    55,    -1,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    -1,    72,    73,    74,    -1,    -1,    77,
      10,    11,    12,    13,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      -1,    -1,    -1,    -1,    -1,    45,    46,    47,    48,    49,
      50,    -1,    -1,    -1,    54,    55,    -1,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    -1,    72,    73,    74,    -1,    76,    77,    10,    11,
      12,    13,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    -1,    -1,
      -1,    -1,    -1,    45,    46,    47,    48,    49,    50,    -1,
      -1,    -1,    54,    55,    -1,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    -1,
      72,    73,    74,    -1,    -1,    77
};



static const yytype_uint8 yystos[] =
{
       0,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    72,    73,    74,    75,
      77,   160,   161,   162,   163,   164,   169,   170,   171,   172,
     174,   175,   176,   177,   178,   181,   182,   183,   215,   216,
     217,    44,    76,   177,    40,    41,    76,    77,   108,   129,
     104,   114,   218,   105,   111,     9,    40,    41,    42,   166,
     167,   173,   111,   114,    76,   129,   175,    76,   114,   159,
     176,   181,   175,   106,     0,   216,   181,   185,   108,    76,
     179,   180,   108,   201,   166,   165,   168,   176,   167,   129,
     104,   106,   113,   108,     3,   174,   176,   186,   187,    76,
      78,    79,    80,    81,    85,    86,   104,   115,   116,   118,
     130,   131,   132,   134,   135,   136,   137,   138,   139,   140,
     141,   142,   143,   144,   145,   146,   147,   148,   149,   150,
     151,   152,   153,   154,   158,   182,   114,   186,   184,   113,
     105,   111,    14,    15,    16,    18,    19,    20,    21,    56,
     108,   109,   114,   141,   154,   155,   157,   160,   161,   182,
     191,   192,   193,   194,   202,   203,   204,   207,   214,   129,
     165,   168,   106,   113,   107,   158,   155,   190,   176,   129,
     188,   189,   109,   187,   141,   141,   157,    85,    86,   106,
     110,   105,   105,   111,    55,   155,   104,   141,   119,   120,
     116,   118,    87,    88,   122,   123,    89,    90,    91,    93,
      92,   127,   107,   109,   186,    79,    80,   180,   114,   114,
     209,   104,   104,   114,   114,   157,   104,   109,   195,    94,
      95,    96,   103,   113,   156,   111,   114,   109,   192,   106,
     107,   158,   190,   107,   188,   106,   111,   114,    76,   114,
     105,   133,   157,    76,    77,   129,   136,   155,   141,   141,
     143,   143,   145,   145,   145,   145,   146,   146,   150,   151,
     152,   157,   109,   198,   199,   200,   210,   157,   114,   208,
     202,   155,   155,   158,   107,   114,   158,   189,   106,   114,
     107,   112,    56,   201,   193,   191,   203,   211,   105,   157,
     171,   174,   206,   196,   107,   107,   158,   155,   104,   206,
     212,   213,   198,   205,   129,   105,   109,   107,   157,   114,
     105,    17,   113,   193,   197,   201,   114,   105,   157,   197,
     198,   190,   114
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab









#define YYFAIL		goto yyerrlab
#if defined YYFAIL
  



#endif

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
do                                                              \
  if (yychar == YYEMPTY)                                        \
    {                                                           \
      yychar = (Token);                                         \
      yylval = (Value);                                         \
      YYPOPSTACK (yylen);                                       \
      yystate = *yyssp;                                         \
      goto yybackup;                                            \
    }                                                           \
  else                                                          \
    {                                                           \
      yyerror (&yylloc, context, YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256






#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)                                \
    do                                                                  \
      if (YYID (N))                                                     \
        {                                                               \
          (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;        \
          (Current).first_column = YYRHSLOC (Rhs, 1).first_column;      \
          (Current).last_line    = YYRHSLOC (Rhs, N).last_line;         \
          (Current).last_column  = YYRHSLOC (Rhs, N).last_column;       \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).first_line   = (Current).last_line   =              \
            YYRHSLOC (Rhs, 0).last_line;                                \
          (Current).first_column = (Current).last_column =              \
            YYRHSLOC (Rhs, 0).last_column;                              \
        }                                                               \
    while (YYID (0))
#endif

#define YYRHSLOC(Rhs, K) ((Rhs)[K])






#ifndef YY_LOCATION_PRINT
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL



__attribute__((__unused__))
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static unsigned
yy_location_print_ (FILE *yyo, YYLTYPE const * const yylocp)
#else
static unsigned
yy_location_print_ (yyo, yylocp)
    FILE *yyo;
    YYLTYPE const * const yylocp;
#endif
{
  unsigned res = 0;
  int end_col = 0 != yylocp->last_column ? yylocp->last_column - 1 : 0;
  if (0 <= yylocp->first_line)
    {
      res += fprintf (yyo, "%d", yylocp->first_line);
      if (0 <= yylocp->first_column)
        res += fprintf (yyo, ".%d", yylocp->first_column);
    }
  if (0 <= yylocp->last_line)
    {
      if (yylocp->first_line < yylocp->last_line)
        {
          res += fprintf (yyo, "-%d", yylocp->last_line);
          if (0 <= end_col)
            res += fprintf (yyo, ".%d", end_col);
        }
      else if (0 <= end_col && yylocp->first_column < end_col)
        res += fprintf (yyo, "-%d", end_col);
    }
  return res;
 }

#  define YY_LOCATION_PRINT(File, Loc)          \
  yy_location_print_ (File, &(Loc))

# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif



#ifdef YYLEX_PARAM
# define YYLEX yylex (&yylval, &yylloc, YYLEX_PARAM)
#else
# define YYLEX yylex (&yylval, &yylloc)
#endif


#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> 
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value, Location, context); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))







#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, TParseContext* context)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp, context)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
    TParseContext* context;
#endif
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  if (!yyvaluep)
    return;
  YYUSE (yylocationp);
  YYUSE (context);
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  YYUSE (yytype);
}






#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, TParseContext* context)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep, yylocationp, context)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
    TParseContext* context;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  YY_LOCATION_PRINT (yyoutput, *yylocationp);
  YYFPRINTF (yyoutput, ": ");
  yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp, context);
  YYFPRINTF (yyoutput, ")");
}






#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))






#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, YYLTYPE *yylsp, int yyrule, TParseContext* context)
#else
static void
yy_reduce_print (yyvsp, yylsp, yyrule, context)
    YYSTYPE *yyvsp;
    YYLTYPE *yylsp;
    int yyrule;
    TParseContext* context;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       , &(yylsp[(yyi + 1) - (yynrhs)])		       , context);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, yylsp, Rule, context); \
} while (YYID (0))



int yydebug;
#else 
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif 



#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif








#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else


#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr







static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif









static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULL, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  
  const char *yyformat = YY_NULL;
  
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  

  int yycount = 0;

  


























  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          


          int yyxbegin = yyn < 0 ? -yyn : 0;
          
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULL, yytname[yyx]);
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  


  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif 






#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, YYLTYPE *yylocationp, TParseContext* context)
#else
static void
yydestruct (yymsg, yytype, yyvaluep, yylocationp, context)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
    YYLTYPE *yylocationp;
    TParseContext* context;
#endif
{
  YYUSE (yyvaluep);
  YYUSE (yylocationp);
  YYUSE (context);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YYUSE (yytype);
}








#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else 
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (TParseContext* context)
#else
int
yyparse (context)
    TParseContext* context;
#endif
#endif
{

int yychar;


#if defined __GNUC__ && 407 <= __GNUC__ * 100 + __GNUC_MINOR__

# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
    _Pragma ("GCC diagnostic pop")
#else


static YYSTYPE yyval_default;
# define YY_INITIAL_VALUE(Value) = Value
#endif
static YYLTYPE yyloc_default
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
  = { 1, 1, 1, 1 }
# endif
;
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value)
#endif


YYSTYPE yylval YY_INITIAL_VALUE(yyval_default);


YYLTYPE yylloc = yyloc_default;


    
    int yynerrs;

    int yystate;
    
    int yyerrstatus;

    







    
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    
    YYLTYPE yylsa[YYINITDEPTH];
    YYLTYPE *yyls;
    YYLTYPE *yylsp;

    
    YYLTYPE yyerror_range[3];

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  
  int yytoken = 0;
  

  YYSTYPE yyval;
  YYLTYPE yyloc;

#if YYERROR_VERBOSE
  
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  

  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yylsp = yyls = yylsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; 
  yylsp[0] = yylloc;
  goto yysetstate;




 yynewstate:
  

  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	


	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;
	YYLTYPE *yyls1 = yyls;

	



	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yyls1, yysize * sizeof (*yylsp),
		    &yystacksize);

	yyls = yyls1;
	yyss = yyss1;
	yyvs = yyvs1;
      }
#else 
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
	YYSTACK_RELOCATE (yyls_alloc, yyls);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif 

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;




yybackup:

  


  
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  

  
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  

  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  

  if (yyerrstatus)
    yyerrstatus--;

  
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  
  yychar = YYEMPTY;

  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END
  *++yylsp = yylloc;
  goto yynewstate;





yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;





yyreduce:
  
  yylen = yyr2[yyn];

  







  yyval = yyvsp[1-yylen];

  
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 4:

    {
        
        const TVariable *variable = context->getNamedVariable((yylsp[(1) - (1)]), (yyvsp[(1) - (1)].lex).string, (yyvsp[(1) - (1)].lex).symbol);

        if (variable->getType().getQualifier() == EvqConst)
        {
            ConstantUnion* constArray = variable->getConstPointer();
            TType t(variable->getType());
            (yyval.interm.intermTypedNode) = context->intermediate.addConstantUnion(constArray, t, (yylsp[(1) - (1)]));
        }
        else
        {
            (yyval.interm.intermTypedNode) = context->intermediate.addSymbol(variable->getUniqueId(),
                                                 variable->getName(),
                                                 variable->getType(),
                                                 (yylsp[(1) - (1)]));
        }

        
        
    }
    break;

  case 5:

    {
        (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode);
    }
    break;

  case 6:

    {
        ConstantUnion *unionArray = new ConstantUnion[1];
        unionArray->setIConst((yyvsp[(1) - (1)].lex).i);
        (yyval.interm.intermTypedNode) = context->intermediate.addConstantUnion(unionArray, TType(EbtInt, EbpUndefined, EvqConst), (yylsp[(1) - (1)]));
    }
    break;

  case 7:

    {
        ConstantUnion *unionArray = new ConstantUnion[1];
        unionArray->setUConst((yyvsp[(1) - (1)].lex).u);
        (yyval.interm.intermTypedNode) = context->intermediate.addConstantUnion(unionArray, TType(EbtUInt, EbpUndefined, EvqConst), (yylsp[(1) - (1)]));
    }
    break;

  case 8:

    {
        ConstantUnion *unionArray = new ConstantUnion[1];
        unionArray->setFConst((yyvsp[(1) - (1)].lex).f);
        (yyval.interm.intermTypedNode) = context->intermediate.addConstantUnion(unionArray, TType(EbtFloat, EbpUndefined, EvqConst), (yylsp[(1) - (1)]));
    }
    break;

  case 9:

    {
        ConstantUnion *unionArray = new ConstantUnion[1];
        unionArray->setBConst((yyvsp[(1) - (1)].lex).b);
        (yyval.interm.intermTypedNode) = context->intermediate.addConstantUnion(unionArray, TType(EbtBool, EbpUndefined, EvqConst), (yylsp[(1) - (1)]));
    }
    break;

  case 10:

    {
        (yyval.interm.intermTypedNode) = (yyvsp[(2) - (3)].interm.intermTypedNode);
    }
    break;

  case 11:

    {
        (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode);
    }
    break;

  case 12:

    {
        (yyval.interm.intermTypedNode) = context->addIndexExpression((yyvsp[(1) - (4)].interm.intermTypedNode), (yylsp[(2) - (4)]), (yyvsp[(3) - (4)].interm.intermTypedNode));
    }
    break;

  case 13:

    {
        (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode);
    }
    break;

  case 14:

    {
        (yyval.interm.intermTypedNode) = context->addFieldSelectionExpression((yyvsp[(1) - (3)].interm.intermTypedNode), (yylsp[(2) - (3)]), *(yyvsp[(3) - (3)].lex).string, (yylsp[(3) - (3)]));
    }
    break;

  case 15:

    {
        if (context->lValueErrorCheck((yylsp[(2) - (2)]), "++", (yyvsp[(1) - (2)].interm.intermTypedNode)))
            context->recover();
        (yyval.interm.intermTypedNode) = context->intermediate.addUnaryMath(EOpPostIncrement, (yyvsp[(1) - (2)].interm.intermTypedNode), (yylsp[(2) - (2)]));
        if ((yyval.interm.intermTypedNode) == 0) {
            context->unaryOpError((yylsp[(2) - (2)]), "++", (yyvsp[(1) - (2)].interm.intermTypedNode)->getCompleteString());
            context->recover();
            (yyval.interm.intermTypedNode) = (yyvsp[(1) - (2)].interm.intermTypedNode);
        }
    }
    break;

  case 16:

    {
        if (context->lValueErrorCheck((yylsp[(2) - (2)]), "--", (yyvsp[(1) - (2)].interm.intermTypedNode)))
            context->recover();
        (yyval.interm.intermTypedNode) = context->intermediate.addUnaryMath(EOpPostDecrement, (yyvsp[(1) - (2)].interm.intermTypedNode), (yylsp[(2) - (2)]));
        if ((yyval.interm.intermTypedNode) == 0) {
            context->unaryOpError((yylsp[(2) - (2)]), "--", (yyvsp[(1) - (2)].interm.intermTypedNode)->getCompleteString());
            context->recover();
            (yyval.interm.intermTypedNode) = (yyvsp[(1) - (2)].interm.intermTypedNode);
        }
    }
    break;

  case 17:

    {
        if (context->integerErrorCheck((yyvsp[(1) - (1)].interm.intermTypedNode), "[]"))
            context->recover();
        (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode);
    }
    break;

  case 18:

    {
        TFunction* fnCall = (yyvsp[(1) - (1)].interm).function;
        TOperator op = fnCall->getBuiltInOp();

        if (op != EOpNull)
        {
            
            
            
            
            
            TType type(EbtVoid, EbpUndefined);  
            if (context->constructorErrorCheck((yylsp[(1) - (1)]), (yyvsp[(1) - (1)].interm).intermNode, *fnCall, op, &type)) {
                (yyval.interm.intermTypedNode) = 0;
            } else {
                
                
                
                (yyval.interm.intermTypedNode) = context->addConstructor((yyvsp[(1) - (1)].interm).intermNode, &type, op, fnCall, (yylsp[(1) - (1)]));
            }

            if ((yyval.interm.intermTypedNode) == 0) {
                context->recover();
                (yyval.interm.intermTypedNode) = context->intermediate.setAggregateOperator(0, op, (yylsp[(1) - (1)]));
            }
            (yyval.interm.intermTypedNode)->setType(type);
        } else {
            
            
            
            const TFunction* fnCandidate;
            bool builtIn;
            fnCandidate = context->findFunction((yylsp[(1) - (1)]), fnCall, context->shaderVersion, &builtIn);
            if (fnCandidate) {
                
                
                
                if (builtIn && !fnCandidate->getExtension().empty() &&
                    context->extensionErrorCheck((yylsp[(1) - (1)]), fnCandidate->getExtension())) {
                    context->recover();
                }
                op = fnCandidate->getBuiltInOp();
                if (builtIn && op != EOpNull) {
                    
                    
                    
                    if (fnCandidate->getParamCount() == 1) {
                        
                        
                        
                        (yyval.interm.intermTypedNode) = context->intermediate.addUnaryMath(op, (yyvsp[(1) - (1)].interm).intermNode, (yylsp[(1) - (1)]));
                        const TType& returnType = fnCandidate->getReturnType();
                        if (returnType.getBasicType() == EbtBool) {
                            
                            
                            (yyval.interm.intermTypedNode)->setType(returnType);
                        } else {
                            
                            (yyval.interm.intermTypedNode)->setTypePreservePrecision(returnType);
                        }
                        if ((yyval.interm.intermTypedNode) == 0)  {
                            std::stringstream extraInfoStream;
                            extraInfoStream << "built in unary operator function.  Type: " << static_cast<TIntermTyped*>((yyvsp[(1) - (1)].interm).intermNode)->getCompleteString();
                            std::string extraInfo = extraInfoStream.str();
                            context->error((yyvsp[(1) - (1)].interm).intermNode->getLine(), " wrong operand type", "Internal Error", extraInfo.c_str());
                            YYERROR;
                        }
                    } else {
                        TIntermAggregate *aggregate = context->intermediate.setAggregateOperator((yyvsp[(1) - (1)].interm).intermAggregate, op, (yylsp[(1) - (1)]));
                        aggregate->setType(fnCandidate->getReturnType());
                        aggregate->setPrecisionFromChildren();
                        (yyval.interm.intermTypedNode) = aggregate;
                    }
                } else {
                    

                    TIntermAggregate *aggregate = context->intermediate.setAggregateOperator((yyvsp[(1) - (1)].interm).intermAggregate, EOpFunctionCall, (yylsp[(1) - (1)]));
                    aggregate->setType(fnCandidate->getReturnType());

                    
                    
                    
                    if (!builtIn)
                        aggregate->setUserDefined();
                    aggregate->setName(fnCandidate->getMangledName());

                    
                    if (builtIn)
                        aggregate->setBuiltInFunctionPrecision();

                    (yyval.interm.intermTypedNode) = aggregate;

                    TQualifier qual;
                    for (size_t i = 0; i < fnCandidate->getParamCount(); ++i) {
                        qual = fnCandidate->getParam(i).type->getQualifier();
                        if (qual == EvqOut || qual == EvqInOut) {
                            if (context->lValueErrorCheck((yyval.interm.intermTypedNode)->getLine(), "assign", (*((yyval.interm.intermTypedNode)->getAsAggregate()->getSequence()))[i]->getAsTyped())) {
                                context->error((yyvsp[(1) - (1)].interm).intermNode->getLine(), "Constant value cannot be passed for 'out' or 'inout' parameters.", "Error");
                                context->recover();
                            }
                        }
                    }
                }
            } else {
                
                
                ConstantUnion *unionArray = new ConstantUnion[1];
                unionArray->setFConst(0.0f);
                (yyval.interm.intermTypedNode) = context->intermediate.addConstantUnion(unionArray, TType(EbtFloat, EbpUndefined, EvqConst), (yylsp[(1) - (1)]));
                context->recover();
            }
        }
        delete fnCall;
    }
    break;

  case 19:

    {
        (yyval.interm) = (yyvsp[(1) - (1)].interm);
    }
    break;

  case 20:

    {
        context->error((yylsp[(3) - (3)]), "methods are not supported", "");
        context->recover();
        (yyval.interm) = (yyvsp[(3) - (3)].interm);
    }
    break;

  case 21:

    {
        (yyval.interm) = (yyvsp[(1) - (2)].interm);
    }
    break;

  case 22:

    {
        (yyval.interm) = (yyvsp[(1) - (2)].interm);
    }
    break;

  case 23:

    {
        (yyval.interm).function = (yyvsp[(1) - (2)].interm.function);
        (yyval.interm).intermNode = 0;
    }
    break;

  case 24:

    {
        (yyval.interm).function = (yyvsp[(1) - (1)].interm.function);
        (yyval.interm).intermNode = 0;
    }
    break;

  case 25:

    {
        TParameter param = { 0, new TType((yyvsp[(2) - (2)].interm.intermTypedNode)->getType()) };
        (yyvsp[(1) - (2)].interm.function)->addParameter(param);
        (yyval.interm).function = (yyvsp[(1) - (2)].interm.function);
        (yyval.interm).intermNode = (yyvsp[(2) - (2)].interm.intermTypedNode);
    }
    break;

  case 26:

    {
        TParameter param = { 0, new TType((yyvsp[(3) - (3)].interm.intermTypedNode)->getType()) };
        (yyvsp[(1) - (3)].interm).function->addParameter(param);
        (yyval.interm).function = (yyvsp[(1) - (3)].interm).function;
        (yyval.interm).intermNode = context->intermediate.growAggregate((yyvsp[(1) - (3)].interm).intermNode, (yyvsp[(3) - (3)].interm.intermTypedNode), (yylsp[(2) - (3)]));
    }
    break;

  case 27:

    {
        (yyval.interm.function) = (yyvsp[(1) - (2)].interm.function);
    }
    break;

  case 28:

    {
        (yyval.interm.function) = context->addConstructorFunc((yyvsp[(1) - (1)].interm.type));
    }
    break;

  case 29:

    {
        if (context->reservedErrorCheck((yylsp[(1) - (1)]), *(yyvsp[(1) - (1)].lex).string))
            context->recover();
        TType type(EbtVoid, EbpUndefined);
        TFunction *function = new TFunction((yyvsp[(1) - (1)].lex).string, type);
        (yyval.interm.function) = function;
    }
    break;

  case 30:

    {
        (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode);
    }
    break;

  case 31:

    {
        if (context->lValueErrorCheck((yylsp[(1) - (2)]), "++", (yyvsp[(2) - (2)].interm.intermTypedNode)))
            context->recover();
        (yyval.interm.intermTypedNode) = context->intermediate.addUnaryMath(EOpPreIncrement, (yyvsp[(2) - (2)].interm.intermTypedNode), (yylsp[(1) - (2)]));
        if ((yyval.interm.intermTypedNode) == 0) {
            context->unaryOpError((yylsp[(1) - (2)]), "++", (yyvsp[(2) - (2)].interm.intermTypedNode)->getCompleteString());
            context->recover();
            (yyval.interm.intermTypedNode) = (yyvsp[(2) - (2)].interm.intermTypedNode);
        }
    }
    break;

  case 32:

    {
        if (context->lValueErrorCheck((yylsp[(1) - (2)]), "--", (yyvsp[(2) - (2)].interm.intermTypedNode)))
            context->recover();
        (yyval.interm.intermTypedNode) = context->intermediate.addUnaryMath(EOpPreDecrement, (yyvsp[(2) - (2)].interm.intermTypedNode), (yylsp[(1) - (2)]));
        if ((yyval.interm.intermTypedNode) == 0) {
            context->unaryOpError((yylsp[(1) - (2)]), "--", (yyvsp[(2) - (2)].interm.intermTypedNode)->getCompleteString());
            context->recover();
            (yyval.interm.intermTypedNode) = (yyvsp[(2) - (2)].interm.intermTypedNode);
        }
    }
    break;

  case 33:

    {
        if ((yyvsp[(1) - (2)].interm).op != EOpNull) {
            (yyval.interm.intermTypedNode) = context->intermediate.addUnaryMath((yyvsp[(1) - (2)].interm).op, (yyvsp[(2) - (2)].interm.intermTypedNode), (yylsp[(1) - (2)]));
            if ((yyval.interm.intermTypedNode) == 0) {
                const char* errorOp = "";
                switch((yyvsp[(1) - (2)].interm).op) {
                case EOpNegative:   errorOp = "-"; break;
                case EOpPositive:   errorOp = "+"; break;
                case EOpLogicalNot: errorOp = "!"; break;
                default: break;
                }
                context->unaryOpError((yylsp[(1) - (2)]), errorOp, (yyvsp[(2) - (2)].interm.intermTypedNode)->getCompleteString());
                context->recover();
                (yyval.interm.intermTypedNode) = (yyvsp[(2) - (2)].interm.intermTypedNode);
            }
        } else
            (yyval.interm.intermTypedNode) = (yyvsp[(2) - (2)].interm.intermTypedNode);
    }
    break;

  case 34:

    { (yyval.interm).op = EOpPositive; }
    break;

  case 35:

    { (yyval.interm).op = EOpNegative; }
    break;

  case 36:

    { (yyval.interm).op = EOpLogicalNot; }
    break;

  case 37:

    { (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode); }
    break;

  case 38:

    {
        (yyval.interm.intermTypedNode) = context->intermediate.addBinaryMath(EOpMul, (yyvsp[(1) - (3)].interm.intermTypedNode), (yyvsp[(3) - (3)].interm.intermTypedNode), (yylsp[(2) - (3)]));
        if ((yyval.interm.intermTypedNode) == 0) {
            context->binaryOpError((yylsp[(2) - (3)]), "*", (yyvsp[(1) - (3)].interm.intermTypedNode)->getCompleteString(), (yyvsp[(3) - (3)].interm.intermTypedNode)->getCompleteString());
            context->recover();
            (yyval.interm.intermTypedNode) = (yyvsp[(1) - (3)].interm.intermTypedNode);
        }
    }
    break;

  case 39:

    {
        (yyval.interm.intermTypedNode) = context->intermediate.addBinaryMath(EOpDiv, (yyvsp[(1) - (3)].interm.intermTypedNode), (yyvsp[(3) - (3)].interm.intermTypedNode), (yylsp[(2) - (3)]));
        if ((yyval.interm.intermTypedNode) == 0) {
            context->binaryOpError((yylsp[(2) - (3)]), "/", (yyvsp[(1) - (3)].interm.intermTypedNode)->getCompleteString(), (yyvsp[(3) - (3)].interm.intermTypedNode)->getCompleteString());
            context->recover();
            (yyval.interm.intermTypedNode) = (yyvsp[(1) - (3)].interm.intermTypedNode);
        }
    }
    break;

  case 40:

    { (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode); }
    break;

  case 41:

    {
        (yyval.interm.intermTypedNode) = context->intermediate.addBinaryMath(EOpAdd, (yyvsp[(1) - (3)].interm.intermTypedNode), (yyvsp[(3) - (3)].interm.intermTypedNode), (yylsp[(2) - (3)]));
        if ((yyval.interm.intermTypedNode) == 0) {
            context->binaryOpError((yylsp[(2) - (3)]), "+", (yyvsp[(1) - (3)].interm.intermTypedNode)->getCompleteString(), (yyvsp[(3) - (3)].interm.intermTypedNode)->getCompleteString());
            context->recover();
            (yyval.interm.intermTypedNode) = (yyvsp[(1) - (3)].interm.intermTypedNode);
        }
    }
    break;

  case 42:

    {
        (yyval.interm.intermTypedNode) = context->intermediate.addBinaryMath(EOpSub, (yyvsp[(1) - (3)].interm.intermTypedNode), (yyvsp[(3) - (3)].interm.intermTypedNode), (yylsp[(2) - (3)]));
        if ((yyval.interm.intermTypedNode) == 0) {
            context->binaryOpError((yylsp[(2) - (3)]), "-", (yyvsp[(1) - (3)].interm.intermTypedNode)->getCompleteString(), (yyvsp[(3) - (3)].interm.intermTypedNode)->getCompleteString());
            context->recover();
            (yyval.interm.intermTypedNode) = (yyvsp[(1) - (3)].interm.intermTypedNode);
        }
    }
    break;

  case 43:

    { (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode); }
    break;

  case 44:

    { (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode); }
    break;

  case 45:

    {
        (yyval.interm.intermTypedNode) = context->intermediate.addBinaryMath(EOpLessThan, (yyvsp[(1) - (3)].interm.intermTypedNode), (yyvsp[(3) - (3)].interm.intermTypedNode), (yylsp[(2) - (3)]));
        if ((yyval.interm.intermTypedNode) == 0) {
            context->binaryOpError((yylsp[(2) - (3)]), "<", (yyvsp[(1) - (3)].interm.intermTypedNode)->getCompleteString(), (yyvsp[(3) - (3)].interm.intermTypedNode)->getCompleteString());
            context->recover();
            ConstantUnion *unionArray = new ConstantUnion[1];
            unionArray->setBConst(false);
            (yyval.interm.intermTypedNode) = context->intermediate.addConstantUnion(unionArray, TType(EbtBool, EbpUndefined, EvqConst), (yylsp[(2) - (3)]));
        }
    }
    break;

  case 46:

    {
        (yyval.interm.intermTypedNode) = context->intermediate.addBinaryMath(EOpGreaterThan, (yyvsp[(1) - (3)].interm.intermTypedNode), (yyvsp[(3) - (3)].interm.intermTypedNode), (yylsp[(2) - (3)]));
        if ((yyval.interm.intermTypedNode) == 0) {
            context->binaryOpError((yylsp[(2) - (3)]), ">", (yyvsp[(1) - (3)].interm.intermTypedNode)->getCompleteString(), (yyvsp[(3) - (3)].interm.intermTypedNode)->getCompleteString());
            context->recover();
            ConstantUnion *unionArray = new ConstantUnion[1];
            unionArray->setBConst(false);
            (yyval.interm.intermTypedNode) = context->intermediate.addConstantUnion(unionArray, TType(EbtBool, EbpUndefined, EvqConst), (yylsp[(2) - (3)]));
        }
    }
    break;

  case 47:

    {
        (yyval.interm.intermTypedNode) = context->intermediate.addBinaryMath(EOpLessThanEqual, (yyvsp[(1) - (3)].interm.intermTypedNode), (yyvsp[(3) - (3)].interm.intermTypedNode), (yylsp[(2) - (3)]));
        if ((yyval.interm.intermTypedNode) == 0) {
            context->binaryOpError((yylsp[(2) - (3)]), "<=", (yyvsp[(1) - (3)].interm.intermTypedNode)->getCompleteString(), (yyvsp[(3) - (3)].interm.intermTypedNode)->getCompleteString());
            context->recover();
            ConstantUnion *unionArray = new ConstantUnion[1];
            unionArray->setBConst(false);
            (yyval.interm.intermTypedNode) = context->intermediate.addConstantUnion(unionArray, TType(EbtBool, EbpUndefined, EvqConst), (yylsp[(2) - (3)]));
        }
    }
    break;

  case 48:

    {
        (yyval.interm.intermTypedNode) = context->intermediate.addBinaryMath(EOpGreaterThanEqual, (yyvsp[(1) - (3)].interm.intermTypedNode), (yyvsp[(3) - (3)].interm.intermTypedNode), (yylsp[(2) - (3)]));
        if ((yyval.interm.intermTypedNode) == 0) {
            context->binaryOpError((yylsp[(2) - (3)]), ">=", (yyvsp[(1) - (3)].interm.intermTypedNode)->getCompleteString(), (yyvsp[(3) - (3)].interm.intermTypedNode)->getCompleteString());
            context->recover();
            ConstantUnion *unionArray = new ConstantUnion[1];
            unionArray->setBConst(false);
            (yyval.interm.intermTypedNode) = context->intermediate.addConstantUnion(unionArray, TType(EbtBool, EbpUndefined, EvqConst), (yylsp[(2) - (3)]));
        }
    }
    break;

  case 49:

    { (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode); }
    break;

  case 50:

    {
        (yyval.interm.intermTypedNode) = context->intermediate.addBinaryMath(EOpEqual, (yyvsp[(1) - (3)].interm.intermTypedNode), (yyvsp[(3) - (3)].interm.intermTypedNode), (yylsp[(2) - (3)]));
        if ((yyval.interm.intermTypedNode) == 0) {
            context->binaryOpError((yylsp[(2) - (3)]), "==", (yyvsp[(1) - (3)].interm.intermTypedNode)->getCompleteString(), (yyvsp[(3) - (3)].interm.intermTypedNode)->getCompleteString());
            context->recover();
            ConstantUnion *unionArray = new ConstantUnion[1];
            unionArray->setBConst(false);
            (yyval.interm.intermTypedNode) = context->intermediate.addConstantUnion(unionArray, TType(EbtBool, EbpUndefined, EvqConst), (yylsp[(2) - (3)]));
        }
    }
    break;

  case 51:

    {
        (yyval.interm.intermTypedNode) = context->intermediate.addBinaryMath(EOpNotEqual, (yyvsp[(1) - (3)].interm.intermTypedNode), (yyvsp[(3) - (3)].interm.intermTypedNode), (yylsp[(2) - (3)]));
        if ((yyval.interm.intermTypedNode) == 0) {
            context->binaryOpError((yylsp[(2) - (3)]), "!=", (yyvsp[(1) - (3)].interm.intermTypedNode)->getCompleteString(), (yyvsp[(3) - (3)].interm.intermTypedNode)->getCompleteString());
            context->recover();
            ConstantUnion *unionArray = new ConstantUnion[1];
            unionArray->setBConst(false);
            (yyval.interm.intermTypedNode) = context->intermediate.addConstantUnion(unionArray, TType(EbtBool, EbpUndefined, EvqConst), (yylsp[(2) - (3)]));
        }
    }
    break;

  case 52:

    { (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode); }
    break;

  case 53:

    { (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode); }
    break;

  case 54:

    { (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode); }
    break;

  case 55:

    { (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode); }
    break;

  case 56:

    {
        (yyval.interm.intermTypedNode) = context->intermediate.addBinaryMath(EOpLogicalAnd, (yyvsp[(1) - (3)].interm.intermTypedNode), (yyvsp[(3) - (3)].interm.intermTypedNode), (yylsp[(2) - (3)]));
        if ((yyval.interm.intermTypedNode) == 0) {
            context->binaryOpError((yylsp[(2) - (3)]), "&&", (yyvsp[(1) - (3)].interm.intermTypedNode)->getCompleteString(), (yyvsp[(3) - (3)].interm.intermTypedNode)->getCompleteString());
            context->recover();
            ConstantUnion *unionArray = new ConstantUnion[1];
            unionArray->setBConst(false);
            (yyval.interm.intermTypedNode) = context->intermediate.addConstantUnion(unionArray, TType(EbtBool, EbpUndefined, EvqConst), (yylsp[(2) - (3)]));
        }
    }
    break;

  case 57:

    { (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode); }
    break;

  case 58:

    {
        (yyval.interm.intermTypedNode) = context->intermediate.addBinaryMath(EOpLogicalXor, (yyvsp[(1) - (3)].interm.intermTypedNode), (yyvsp[(3) - (3)].interm.intermTypedNode), (yylsp[(2) - (3)]));
        if ((yyval.interm.intermTypedNode) == 0) {
            context->binaryOpError((yylsp[(2) - (3)]), "^^", (yyvsp[(1) - (3)].interm.intermTypedNode)->getCompleteString(), (yyvsp[(3) - (3)].interm.intermTypedNode)->getCompleteString());
            context->recover();
            ConstantUnion *unionArray = new ConstantUnion[1];
            unionArray->setBConst(false);
            (yyval.interm.intermTypedNode) = context->intermediate.addConstantUnion(unionArray, TType(EbtBool, EbpUndefined, EvqConst), (yylsp[(2) - (3)]));
        }
    }
    break;

  case 59:

    { (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode); }
    break;

  case 60:

    {
        (yyval.interm.intermTypedNode) = context->intermediate.addBinaryMath(EOpLogicalOr, (yyvsp[(1) - (3)].interm.intermTypedNode), (yyvsp[(3) - (3)].interm.intermTypedNode), (yylsp[(2) - (3)]));
        if ((yyval.interm.intermTypedNode) == 0) {
            context->binaryOpError((yylsp[(2) - (3)]), "||", (yyvsp[(1) - (3)].interm.intermTypedNode)->getCompleteString(), (yyvsp[(3) - (3)].interm.intermTypedNode)->getCompleteString());
            context->recover();
            ConstantUnion *unionArray = new ConstantUnion[1];
            unionArray->setBConst(false);
            (yyval.interm.intermTypedNode) = context->intermediate.addConstantUnion(unionArray, TType(EbtBool, EbpUndefined, EvqConst), (yylsp[(2) - (3)]));
        }
    }
    break;

  case 61:

    { (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode); }
    break;

  case 62:

    {
       if (context->boolErrorCheck((yylsp[(2) - (5)]), (yyvsp[(1) - (5)].interm.intermTypedNode)))
            context->recover();

        (yyval.interm.intermTypedNode) = context->intermediate.addSelection((yyvsp[(1) - (5)].interm.intermTypedNode), (yyvsp[(3) - (5)].interm.intermTypedNode), (yyvsp[(5) - (5)].interm.intermTypedNode), (yylsp[(2) - (5)]));
        if ((yyvsp[(3) - (5)].interm.intermTypedNode)->getType() != (yyvsp[(5) - (5)].interm.intermTypedNode)->getType())
            (yyval.interm.intermTypedNode) = 0;

        if ((yyval.interm.intermTypedNode) == 0) {
            context->binaryOpError((yylsp[(2) - (5)]), ":", (yyvsp[(3) - (5)].interm.intermTypedNode)->getCompleteString(), (yyvsp[(5) - (5)].interm.intermTypedNode)->getCompleteString());
            context->recover();
            (yyval.interm.intermTypedNode) = (yyvsp[(5) - (5)].interm.intermTypedNode);
        }
    }
    break;

  case 63:

    { (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode); }
    break;

  case 64:

    {
        if (context->lValueErrorCheck((yylsp[(2) - (3)]), "assign", (yyvsp[(1) - (3)].interm.intermTypedNode)))
            context->recover();
        (yyval.interm.intermTypedNode) = context->intermediate.addAssign((yyvsp[(2) - (3)].interm).op, (yyvsp[(1) - (3)].interm.intermTypedNode), (yyvsp[(3) - (3)].interm.intermTypedNode), (yylsp[(2) - (3)]));
        if ((yyval.interm.intermTypedNode) == 0) {
            context->assignError((yylsp[(2) - (3)]), "assign", (yyvsp[(1) - (3)].interm.intermTypedNode)->getCompleteString(), (yyvsp[(3) - (3)].interm.intermTypedNode)->getCompleteString());
            context->recover();
            (yyval.interm.intermTypedNode) = (yyvsp[(1) - (3)].interm.intermTypedNode);
        }
    }
    break;

  case 65:

    { (yyval.interm).op = EOpAssign; }
    break;

  case 66:

    { (yyval.interm).op = EOpMulAssign; }
    break;

  case 67:

    { (yyval.interm).op = EOpDivAssign; }
    break;

  case 68:

    { (yyval.interm).op = EOpAddAssign; }
    break;

  case 69:

    { (yyval.interm).op = EOpSubAssign; }
    break;

  case 70:

    {
        (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode);
    }
    break;

  case 71:

    {
        (yyval.interm.intermTypedNode) = context->intermediate.addComma((yyvsp[(1) - (3)].interm.intermTypedNode), (yyvsp[(3) - (3)].interm.intermTypedNode), (yylsp[(2) - (3)]));
        if ((yyval.interm.intermTypedNode) == 0) {
            context->binaryOpError((yylsp[(2) - (3)]), ",", (yyvsp[(1) - (3)].interm.intermTypedNode)->getCompleteString(), (yyvsp[(3) - (3)].interm.intermTypedNode)->getCompleteString());
            context->recover();
            (yyval.interm.intermTypedNode) = (yyvsp[(3) - (3)].interm.intermTypedNode);
        }
    }
    break;

  case 72:

    {
        if (context->constErrorCheck((yyvsp[(1) - (1)].interm.intermTypedNode)))
            context->recover();
        (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode);
    }
    break;

  case 73:

    {
        if (context->enterStructDeclaration((yylsp[(1) - (2)]), *(yyvsp[(1) - (2)].lex).string))
            context->recover();
        (yyval.lex) = (yyvsp[(1) - (2)].lex);
    }
    break;

  case 74:

    {
        TFunction &function = *((yyvsp[(1) - (2)].interm).function);
        
        TIntermAggregate *prototype = new TIntermAggregate;
        prototype->setType(function.getReturnType());
        prototype->setName(function.getName());
        
        for (size_t i = 0; i < function.getParamCount(); i++)
        {
            const TParameter &param = function.getParam(i);
            if (param.name != 0)
            {
                TVariable variable(param.name, *param.type);
                
                prototype = context->intermediate.growAggregate(prototype, context->intermediate.addSymbol(variable.getUniqueId(), variable.getName(), variable.getType(), (yylsp[(1) - (2)])), (yylsp[(1) - (2)]));
            }
            else
            {
                prototype = context->intermediate.growAggregate(prototype, context->intermediate.addSymbol(0, "", *param.type, (yylsp[(1) - (2)])), (yylsp[(1) - (2)]));
            }
        }
        
        prototype->setOp(EOpPrototype);
        (yyval.interm.intermNode) = prototype;

        context->symbolTable.pop();
    }
    break;

  case 75:

    {
        TIntermAggregate *aggNode = (yyvsp[(1) - (2)].interm).intermAggregate;
        if (aggNode && aggNode->getOp() == EOpNull)
            aggNode->setOp(EOpDeclaration);
        (yyval.interm.intermNode) = aggNode;
    }
    break;

  case 76:

    {
        if (((yyvsp[(2) - (4)].interm.precision) == EbpHigh) && (context->shaderType == GL_FRAGMENT_SHADER) && !context->fragmentPrecisionHigh) {
            context->error((yylsp[(1) - (4)]), "precision is not supported in fragment shader", "highp");
            context->recover();
        }
        if (!context->symbolTable.setDefaultPrecision( (yyvsp[(3) - (4)].interm.type), (yyvsp[(2) - (4)].interm.precision) )) {
            context->error((yylsp[(1) - (4)]), "illegal type argument for default precision qualifier", getBasicString((yyvsp[(3) - (4)].interm.type).type));
            context->recover();
        }
        (yyval.interm.intermNode) = 0;
    }
    break;

  case 77:

    {
        ES3_ONLY(getQualifierString((yyvsp[(1) - (5)].interm.type).qualifier), (yylsp[(1) - (5)]), "interface blocks");
        (yyval.interm.intermNode) = context->addInterfaceBlock((yyvsp[(1) - (5)].interm.type), (yylsp[(2) - (5)]), *(yyvsp[(2) - (5)].lex).string, (yyvsp[(3) - (5)].interm.fieldList), NULL, (yyloc), NULL, (yyloc));
    }
    break;

  case 78:

    {
        ES3_ONLY(getQualifierString((yyvsp[(1) - (6)].interm.type).qualifier), (yylsp[(1) - (6)]), "interface blocks");
        (yyval.interm.intermNode) = context->addInterfaceBlock((yyvsp[(1) - (6)].interm.type), (yylsp[(2) - (6)]), *(yyvsp[(2) - (6)].lex).string, (yyvsp[(3) - (6)].interm.fieldList), (yyvsp[(5) - (6)].lex).string, (yylsp[(5) - (6)]), NULL, (yyloc));
    }
    break;

  case 79:

    {
        ES3_ONLY(getQualifierString((yyvsp[(1) - (9)].interm.type).qualifier), (yylsp[(1) - (9)]), "interface blocks");
        (yyval.interm.intermNode) = context->addInterfaceBlock((yyvsp[(1) - (9)].interm.type), (yylsp[(2) - (9)]), *(yyvsp[(2) - (9)].lex).string, (yyvsp[(3) - (9)].interm.fieldList), (yyvsp[(5) - (9)].lex).string, (yylsp[(5) - (9)]), (yyvsp[(7) - (9)].interm.intermTypedNode), (yylsp[(6) - (9)]));
    }
    break;

  case 80:

    {
        context->parseGlobalLayoutQualifier((yyvsp[(1) - (2)].interm.type));
        (yyval.interm.intermNode) = 0;
    }
    break;

  case 81:

    {
        
        
        
        
        
        
        
        
        TFunction* prevDec = static_cast<TFunction*>(context->symbolTable.find((yyvsp[(1) - (2)].interm.function)->getMangledName(), context->shaderVersion));
        if (prevDec) {
            if (prevDec->getReturnType() != (yyvsp[(1) - (2)].interm.function)->getReturnType()) {
                context->error((yylsp[(2) - (2)]), "overloaded functions must have the same return type", (yyvsp[(1) - (2)].interm.function)->getReturnType().getBasicString());
                context->recover();
            }
            for (size_t i = 0; i < prevDec->getParamCount(); ++i) {
                if (prevDec->getParam(i).type->getQualifier() != (yyvsp[(1) - (2)].interm.function)->getParam(i).type->getQualifier()) {
                    context->error((yylsp[(2) - (2)]), "overloaded functions must have the same parameter qualifiers", (yyvsp[(1) - (2)].interm.function)->getParam(i).type->getQualifierString());
                    context->recover();
                }
            }
        }

        
        
        
        TSymbol *prevSym = context->symbolTable.find((yyvsp[(1) - (2)].interm.function)->getName(), context->shaderVersion);
        if (prevSym)
        {
            if (!prevSym->isFunction())
            {
                context->error((yylsp[(2) - (2)]), "redefinition", (yyvsp[(1) - (2)].interm.function)->getName().c_str(), "function");
                context->recover();
            }
        }
        else
        {
            
            TFunction *function = new TFunction(NewPoolTString((yyvsp[(1) - (2)].interm.function)->getName().c_str()), (yyvsp[(1) - (2)].interm.function)->getReturnType());
            context->symbolTable.getOuterLevel()->insert(function);
        }

        
        
        
        
        
        (yyval.interm).function = (yyvsp[(1) - (2)].interm.function);

        
        
        context->symbolTable.getOuterLevel()->insert((yyval.interm).function);
    }
    break;

  case 82:

    {
        (yyval.interm.function) = (yyvsp[(1) - (1)].interm.function);
    }
    break;

  case 83:

    {
        (yyval.interm.function) = (yyvsp[(1) - (1)].interm.function);
    }
    break;

  case 84:

    {
        
        (yyval.interm.function) = (yyvsp[(1) - (2)].interm.function);
        if ((yyvsp[(2) - (2)].interm).param.type->getBasicType() != EbtVoid)
            (yyvsp[(1) - (2)].interm.function)->addParameter((yyvsp[(2) - (2)].interm).param);
        else
            delete (yyvsp[(2) - (2)].interm).param.type;
    }
    break;

  case 85:

    {
        
        
        
        
        if ((yyvsp[(3) - (3)].interm).param.type->getBasicType() == EbtVoid) {
            
            
            
            context->error((yylsp[(2) - (3)]), "cannot be an argument type except for '(void)'", "void");
            context->recover();
            delete (yyvsp[(3) - (3)].interm).param.type;
        } else {
            
            (yyval.interm.function) = (yyvsp[(1) - (3)].interm.function);
            (yyvsp[(1) - (3)].interm.function)->addParameter((yyvsp[(3) - (3)].interm).param);
        }
    }
    break;

  case 86:

    {
        if ((yyvsp[(1) - (3)].interm.type).qualifier != EvqGlobal && (yyvsp[(1) - (3)].interm.type).qualifier != EvqTemporary) {
            context->error((yylsp[(2) - (3)]), "no qualifiers allowed for function return", getQualifierString((yyvsp[(1) - (3)].interm.type).qualifier));
            context->recover();
        }
        
        if (context->structQualifierErrorCheck((yylsp[(2) - (3)]), (yyvsp[(1) - (3)].interm.type)))
            context->recover();

        
        TFunction *function;
        TType type((yyvsp[(1) - (3)].interm.type));
        function = new TFunction((yyvsp[(2) - (3)].lex).string, type);
        (yyval.interm.function) = function;
        
        context->symbolTable.push();
    }
    break;

  case 87:

    {
        if ((yyvsp[(1) - (2)].interm.type).type == EbtVoid) {
            context->error((yylsp[(2) - (2)]), "illegal use of type 'void'", (yyvsp[(2) - (2)].lex).string->c_str());
            context->recover();
        }
        if (context->reservedErrorCheck((yylsp[(2) - (2)]), *(yyvsp[(2) - (2)].lex).string))
            context->recover();
        TParameter param = {(yyvsp[(2) - (2)].lex).string, new TType((yyvsp[(1) - (2)].interm.type))};
        (yyval.interm).param = param;
    }
    break;

  case 88:

    {
        
        if (context->arrayTypeErrorCheck((yylsp[(3) - (5)]), (yyvsp[(1) - (5)].interm.type)))
            context->recover();

        if (context->reservedErrorCheck((yylsp[(2) - (5)]), *(yyvsp[(2) - (5)].lex).string))
            context->recover();

        int size;
        if (context->arraySizeErrorCheck((yylsp[(3) - (5)]), (yyvsp[(4) - (5)].interm.intermTypedNode), size))
            context->recover();
        (yyvsp[(1) - (5)].interm.type).setArray(true, size);

        TType* type = new TType((yyvsp[(1) - (5)].interm.type));
        TParameter param = { (yyvsp[(2) - (5)].lex).string, type };
        (yyval.interm).param = param;
    }
    break;

  case 89:

    {
        (yyval.interm) = (yyvsp[(3) - (3)].interm);
        if (context->paramErrorCheck((yylsp[(3) - (3)]), (yyvsp[(1) - (3)].interm.qualifier), (yyvsp[(2) - (3)].interm.qualifier), (yyval.interm).param.type))
            context->recover();
    }
    break;

  case 90:

    {
        (yyval.interm) = (yyvsp[(2) - (2)].interm);
        if (context->parameterSamplerErrorCheck((yylsp[(2) - (2)]), (yyvsp[(1) - (2)].interm.qualifier), *(yyvsp[(2) - (2)].interm).param.type))
            context->recover();
        if (context->paramErrorCheck((yylsp[(2) - (2)]), EvqTemporary, (yyvsp[(1) - (2)].interm.qualifier), (yyval.interm).param.type))
            context->recover();
    }
    break;

  case 91:

    {
        (yyval.interm) = (yyvsp[(3) - (3)].interm);
        if (context->paramErrorCheck((yylsp[(3) - (3)]), (yyvsp[(1) - (3)].interm.qualifier), (yyvsp[(2) - (3)].interm.qualifier), (yyval.interm).param.type))
            context->recover();
    }
    break;

  case 92:

    {
        (yyval.interm) = (yyvsp[(2) - (2)].interm);
        if (context->parameterSamplerErrorCheck((yylsp[(2) - (2)]), (yyvsp[(1) - (2)].interm.qualifier), *(yyvsp[(2) - (2)].interm).param.type))
            context->recover();
        if (context->paramErrorCheck((yylsp[(2) - (2)]), EvqTemporary, (yyvsp[(1) - (2)].interm.qualifier), (yyval.interm).param.type))
            context->recover();
    }
    break;

  case 93:

    {
        (yyval.interm.qualifier) = EvqIn;
    }
    break;

  case 94:

    {
        (yyval.interm.qualifier) = EvqIn;
    }
    break;

  case 95:

    {
        (yyval.interm.qualifier) = EvqOut;
    }
    break;

  case 96:

    {
        (yyval.interm.qualifier) = EvqInOut;
    }
    break;

  case 97:

    {
        TParameter param = { 0, new TType((yyvsp[(1) - (1)].interm.type)) };
        (yyval.interm).param = param;
    }
    break;

  case 98:

    {
        (yyval.interm) = (yyvsp[(1) - (1)].interm);
    }
    break;

  case 99:

    {
        (yyval.interm) = (yyvsp[(1) - (3)].interm);
        (yyval.interm).intermAggregate = context->parseDeclarator((yyval.interm).type, (yyvsp[(1) - (3)].interm).intermAggregate, (yyvsp[(3) - (3)].lex).symbol, (yylsp[(3) - (3)]), *(yyvsp[(3) - (3)].lex).string);
    }
    break;

  case 100:

    {
        (yyval.interm) = (yyvsp[(1) - (5)].interm);
        context->parseArrayDeclarator((yyval.interm).type, (yylsp[(3) - (5)]), *(yyvsp[(3) - (5)].lex).string, (yylsp[(4) - (5)]), NULL, NULL);
    }
    break;

  case 101:

    {
        (yyval.interm) = (yyvsp[(1) - (6)].interm);
        (yyval.interm).intermAggregate = context->parseArrayDeclarator((yyval.interm).type, (yylsp[(3) - (6)]), *(yyvsp[(3) - (6)].lex).string, (yylsp[(4) - (6)]), (yyvsp[(1) - (6)].interm).intermNode, (yyvsp[(5) - (6)].interm.intermTypedNode));
    }
    break;

  case 102:

    {
        (yyval.interm) = (yyvsp[(1) - (5)].interm);
        (yyval.interm).intermAggregate = context->parseInitDeclarator((yyval.interm).type, (yyvsp[(1) - (5)].interm).intermAggregate, (yylsp[(3) - (5)]), *(yyvsp[(3) - (5)].lex).string, (yylsp[(4) - (5)]), (yyvsp[(5) - (5)].interm.intermTypedNode));
    }
    break;

  case 103:

    {
        (yyval.interm).type = (yyvsp[(1) - (1)].interm.type);
        (yyval.interm).intermAggregate = context->parseSingleDeclaration((yyval.interm).type, (yylsp[(1) - (1)]), "");
    }
    break;

  case 104:

    {
        (yyval.interm).type = (yyvsp[(1) - (2)].interm.type);
        (yyval.interm).intermAggregate = context->parseSingleDeclaration((yyval.interm).type, (yylsp[(2) - (2)]), *(yyvsp[(2) - (2)].lex).string);
    }
    break;

  case 105:

    {
        context->error((yylsp[(2) - (4)]), "unsized array declarations not supported", (yyvsp[(2) - (4)].lex).string->c_str());
        context->recover();

        (yyval.interm).type = (yyvsp[(1) - (4)].interm.type);
        (yyval.interm).intermAggregate = context->parseSingleDeclaration((yyval.interm).type, (yylsp[(2) - (4)]), *(yyvsp[(2) - (4)].lex).string);
    }
    break;

  case 106:

    {
        (yyval.interm).type = (yyvsp[(1) - (5)].interm.type);
        (yyval.interm).intermAggregate = context->parseSingleArrayDeclaration((yyval.interm).type, (yylsp[(2) - (5)]), *(yyvsp[(2) - (5)].lex).string, (yylsp[(3) - (5)]), (yyvsp[(4) - (5)].interm.intermTypedNode));
    }
    break;

  case 107:

    {
        (yyval.interm).type = (yyvsp[(1) - (4)].interm.type);
        (yyval.interm).intermAggregate = context->parseSingleInitDeclaration((yyval.interm).type, (yylsp[(2) - (4)]), *(yyvsp[(2) - (4)].lex).string, (yylsp[(3) - (4)]), (yyvsp[(4) - (4)].interm.intermTypedNode));
    }
    break;

  case 108:

    {
        
        (yyval.interm).intermAggregate = context->parseInvariantDeclaration((yylsp[(1) - (2)]), (yylsp[(2) - (2)]), (yyvsp[(2) - (2)].lex).string, (yyvsp[(2) - (2)].lex).symbol);
    }
    break;

  case 109:

    {
        (yyval.interm.type) = (yyvsp[(1) - (1)].interm.type);

        if ((yyvsp[(1) - (1)].interm.type).array) {
            context->error((yylsp[(1) - (1)]), "not supported", "first-class array");
            context->recover();
            (yyvsp[(1) - (1)].interm.type).setArray(false);
        }
    }
    break;

  case 110:

    {
        (yyval.interm.type) = context->addFullySpecifiedType((yyvsp[(1) - (2)].interm.type).qualifier, (yyvsp[(1) - (2)].interm.type).layoutQualifier, (yyvsp[(2) - (2)].interm.type));
    }
    break;

  case 111:

    {
        (yyval.interm.type).qualifier = EvqSmooth;
    }
    break;

  case 112:

    {
        (yyval.interm.type).qualifier = EvqFlat;
    }
    break;

  case 113:

    {
        (yyval.interm.qualifier) = EvqConst;
    }
    break;

  case 114:

    {
        VERTEX_ONLY("attribute", (yylsp[(1) - (1)]));
        ES2_ONLY("attribute", (yylsp[(1) - (1)]));
        if (context->globalErrorCheck((yylsp[(1) - (1)]), context->symbolTable.atGlobalLevel(), "attribute"))
            context->recover();
        (yyval.interm.type).setBasic(EbtVoid, EvqAttribute, (yylsp[(1) - (1)]));
    }
    break;

  case 115:

    {
        ES2_ONLY("varying", (yylsp[(1) - (1)]));
        if (context->globalErrorCheck((yylsp[(1) - (1)]), context->symbolTable.atGlobalLevel(), "varying"))
            context->recover();
        if (context->shaderType == GL_VERTEX_SHADER)
            (yyval.interm.type).setBasic(EbtVoid, EvqVaryingOut, (yylsp[(1) - (1)]));
        else
            (yyval.interm.type).setBasic(EbtVoid, EvqVaryingIn, (yylsp[(1) - (1)]));
    }
    break;

  case 116:

    {
        ES2_ONLY("varying", (yylsp[(1) - (2)]));
        if (context->globalErrorCheck((yylsp[(1) - (2)]), context->symbolTable.atGlobalLevel(), "invariant varying"))
            context->recover();
        if (context->shaderType == GL_VERTEX_SHADER)
            (yyval.interm.type).setBasic(EbtVoid, EvqInvariantVaryingOut, (yylsp[(1) - (2)]));
        else
            (yyval.interm.type).setBasic(EbtVoid, EvqInvariantVaryingIn, (yylsp[(1) - (2)]));
    }
    break;

  case 117:

    {
        if ((yyvsp[(1) - (1)].interm.type).qualifier != EvqConst && !context->symbolTable.atGlobalLevel()) {
            context->error((yylsp[(1) - (1)]), "Local variables can only use the const storage qualifier.", getQualifierString((yyvsp[(1) - (1)].interm.type).qualifier));
            context->recover();
        } else {
            (yyval.interm.type).setBasic(EbtVoid, (yyvsp[(1) - (1)].interm.type).qualifier, (yylsp[(1) - (1)]));
        }
    }
    break;

  case 118:

    {
        (yyval.interm.type) = context->joinInterpolationQualifiers((yylsp[(1) - (2)]), (yyvsp[(1) - (2)].interm.type).qualifier, (yylsp[(2) - (2)]), (yyvsp[(2) - (2)].interm.type).qualifier);
    }
    break;

  case 119:

    {
        context->error((yylsp[(1) - (1)]), "interpolation qualifier requires a fragment 'in' or vertex 'out' storage qualifier", getInterpolationString((yyvsp[(1) - (1)].interm.type).qualifier));
        context->recover();
        
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtVoid, qual, (yylsp[(1) - (1)]));
    }
    break;

  case 120:

    {
        (yyval.interm.type).qualifier = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).layoutQualifier = (yyvsp[(1) - (1)].interm.layoutQualifier);
    }
    break;

  case 121:

    {
        (yyval.interm.type).setBasic(EbtVoid, (yyvsp[(2) - (2)].interm.type).qualifier, (yylsp[(2) - (2)]));
        (yyval.interm.type).layoutQualifier = (yyvsp[(1) - (2)].interm.layoutQualifier);
    }
    break;

  case 122:

    {
        (yyval.interm.type).qualifier = EvqConst;
    }
    break;

  case 123:

    {
        ES3_ONLY("in", (yylsp[(1) - (1)]), "storage qualifier");
        (yyval.interm.type).qualifier = (context->shaderType == GL_FRAGMENT_SHADER) ? EvqFragmentIn : EvqVertexIn;
    }
    break;

  case 124:

    {
        ES3_ONLY("out", (yylsp[(1) - (1)]), "storage qualifier");
        (yyval.interm.type).qualifier = (context->shaderType == GL_FRAGMENT_SHADER) ? EvqFragmentOut : EvqVertexOut;
    }
    break;

  case 125:

    {
        ES3_ONLY("centroid in", (yylsp[(1) - (2)]), "storage qualifier");
        if (context->shaderType == GL_VERTEX_SHADER)
        {
            context->error((yylsp[(1) - (2)]), "invalid storage qualifier", "it is an error to use 'centroid in' in the vertex shader");
            context->recover();
        }
        (yyval.interm.type).qualifier = (context->shaderType == GL_FRAGMENT_SHADER) ? EvqCentroidIn : EvqVertexIn;
    }
    break;

  case 126:

    {
        ES3_ONLY("centroid out", (yylsp[(1) - (2)]), "storage qualifier");
        if (context->shaderType == GL_FRAGMENT_SHADER)
        {
            context->error((yylsp[(1) - (2)]), "invalid storage qualifier", "it is an error to use 'centroid out' in the fragment shader");
            context->recover();
        }
        (yyval.interm.type).qualifier = (context->shaderType == GL_FRAGMENT_SHADER) ? EvqFragmentOut : EvqCentroidOut;
    }
    break;

  case 127:

    {
        if (context->globalErrorCheck((yylsp[(1) - (1)]), context->symbolTable.atGlobalLevel(), "uniform"))
            context->recover();
        (yyval.interm.type).qualifier = EvqUniform;
    }
    break;

  case 128:

    {
        (yyval.interm.type) = (yyvsp[(1) - (1)].interm.type);

        if ((yyval.interm.type).precision == EbpUndefined) {
            (yyval.interm.type).precision = context->symbolTable.getDefaultPrecision((yyvsp[(1) - (1)].interm.type).type);
            if (context->precisionErrorCheck((yylsp[(1) - (1)]), (yyval.interm.type).precision, (yyvsp[(1) - (1)].interm.type).type)) {
                context->recover();
            }
        }
    }
    break;

  case 129:

    {
        (yyval.interm.type) = (yyvsp[(2) - (2)].interm.type);
        (yyval.interm.type).precision = (yyvsp[(1) - (2)].interm.precision);

        if (!SupportsPrecision((yyvsp[(2) - (2)].interm.type).type)) {
            context->error((yylsp[(1) - (2)]), "illegal type for precision qualifier", getBasicString((yyvsp[(2) - (2)].interm.type).type));
            context->recover();
        }
    }
    break;

  case 130:

    {
        (yyval.interm.precision) = EbpHigh;
    }
    break;

  case 131:

    {
        (yyval.interm.precision) = EbpMedium;
    }
    break;

  case 132:

    {
        (yyval.interm.precision) = EbpLow;
    }
    break;

  case 133:

    {
        ES3_ONLY("layout", (yylsp[(1) - (4)]), "qualifier");
        (yyval.interm.layoutQualifier) = (yyvsp[(3) - (4)].interm.layoutQualifier);
    }
    break;

  case 134:

    {
        (yyval.interm.layoutQualifier) = (yyvsp[(1) - (1)].interm.layoutQualifier);
    }
    break;

  case 135:

    {
        (yyval.interm.layoutQualifier) = context->joinLayoutQualifiers((yyvsp[(1) - (3)].interm.layoutQualifier), (yyvsp[(3) - (3)].interm.layoutQualifier));
    }
    break;

  case 136:

    {
        (yyval.interm.layoutQualifier) = context->parseLayoutQualifier(*(yyvsp[(1) - (1)].lex).string, (yylsp[(1) - (1)]));
    }
    break;

  case 137:

    {
        (yyval.interm.layoutQualifier) = context->parseLayoutQualifier(*(yyvsp[(1) - (3)].lex).string, (yylsp[(1) - (3)]), *(yyvsp[(3) - (3)].lex).string, (yyvsp[(3) - (3)].lex).i, (yylsp[(3) - (3)]));
    }
    break;

  case 138:

    {
        (yyval.interm.layoutQualifier) = context->parseLayoutQualifier(*(yyvsp[(1) - (3)].lex).string, (yylsp[(1) - (3)]), *(yyvsp[(3) - (3)].lex).string, (yyvsp[(3) - (3)].lex).i, (yylsp[(3) - (3)]));
    }
    break;

  case 139:

    {
        (yyval.interm.type) = (yyvsp[(1) - (1)].interm.type);
    }
    break;

  case 140:

    {
        (yyval.interm.type) = (yyvsp[(1) - (4)].interm.type);

        if (context->arrayTypeErrorCheck((yylsp[(2) - (4)]), (yyvsp[(1) - (4)].interm.type)))
            context->recover();
        else {
            int size;
            if (context->arraySizeErrorCheck((yylsp[(2) - (4)]), (yyvsp[(3) - (4)].interm.intermTypedNode), size))
                context->recover();
            (yyval.interm.type).setArray(true, size);
        }
    }
    break;

  case 141:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtVoid, qual, (yylsp[(1) - (1)]));
    }
    break;

  case 142:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtFloat, qual, (yylsp[(1) - (1)]));
    }
    break;

  case 143:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtInt, qual, (yylsp[(1) - (1)]));
    }
    break;

  case 144:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtUInt, qual, (yylsp[(1) - (1)]));
    }
    break;

  case 145:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtBool, qual, (yylsp[(1) - (1)]));
    }
    break;

  case 146:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtFloat, qual, (yylsp[(1) - (1)]));
        (yyval.interm.type).setAggregate(2);
    }
    break;

  case 147:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtFloat, qual, (yylsp[(1) - (1)]));
        (yyval.interm.type).setAggregate(3);
    }
    break;

  case 148:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtFloat, qual, (yylsp[(1) - (1)]));
        (yyval.interm.type).setAggregate(4);
    }
    break;

  case 149:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtBool, qual, (yylsp[(1) - (1)]));
        (yyval.interm.type).setAggregate(2);
    }
    break;

  case 150:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtBool, qual, (yylsp[(1) - (1)]));
        (yyval.interm.type).setAggregate(3);
    }
    break;

  case 151:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtBool, qual, (yylsp[(1) - (1)]));
        (yyval.interm.type).setAggregate(4);
    }
    break;

  case 152:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtInt, qual, (yylsp[(1) - (1)]));
        (yyval.interm.type).setAggregate(2);
    }
    break;

  case 153:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtInt, qual, (yylsp[(1) - (1)]));
        (yyval.interm.type).setAggregate(3);
    }
    break;

  case 154:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtInt, qual, (yylsp[(1) - (1)]));
        (yyval.interm.type).setAggregate(4);
    }
    break;

  case 155:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtUInt, qual, (yylsp[(1) - (1)]));
        (yyval.interm.type).setAggregate(2);
    }
    break;

  case 156:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtUInt, qual, (yylsp[(1) - (1)]));
        (yyval.interm.type).setAggregate(3);
    }
    break;

  case 157:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtUInt, qual, (yylsp[(1) - (1)]));
        (yyval.interm.type).setAggregate(4);
    }
    break;

  case 158:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtFloat, qual, (yylsp[(1) - (1)]));
        (yyval.interm.type).setMatrix(2, 2);
    }
    break;

  case 159:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtFloat, qual, (yylsp[(1) - (1)]));
        (yyval.interm.type).setMatrix(3, 3);
    }
    break;

  case 160:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtFloat, qual, (yylsp[(1) - (1)]));
        (yyval.interm.type).setMatrix(4, 4);
    }
    break;

  case 161:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtFloat, qual, (yylsp[(1) - (1)]));
        (yyval.interm.type).setMatrix(2, 3);
    }
    break;

  case 162:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtFloat, qual, (yylsp[(1) - (1)]));
        (yyval.interm.type).setMatrix(3, 2);
    }
    break;

  case 163:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtFloat, qual, (yylsp[(1) - (1)]));
        (yyval.interm.type).setMatrix(2, 4);
    }
    break;

  case 164:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtFloat, qual, (yylsp[(1) - (1)]));
        (yyval.interm.type).setMatrix(4, 2);
    }
    break;

  case 165:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtFloat, qual, (yylsp[(1) - (1)]));
        (yyval.interm.type).setMatrix(3, 4);
    }
    break;

  case 166:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtFloat, qual, (yylsp[(1) - (1)]));
        (yyval.interm.type).setMatrix(4, 3);
    }
    break;

  case 167:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtSampler2D, qual, (yylsp[(1) - (1)]));
    }
    break;

  case 168:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtSampler3D, qual, (yylsp[(1) - (1)]));
    }
    break;

  case 169:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtSamplerCube, qual, (yylsp[(1) - (1)]));
    }
    break;

  case 170:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtSampler2DArray, qual, (yylsp[(1) - (1)]));
    }
    break;

  case 171:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtISampler2D, qual, (yylsp[(1) - (1)]));
    }
    break;

  case 172:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtISampler3D, qual, (yylsp[(1) - (1)]));
    }
    break;

  case 173:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtISamplerCube, qual, (yylsp[(1) - (1)]));
    }
    break;

  case 174:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtISampler2DArray, qual, (yylsp[(1) - (1)]));
    }
    break;

  case 175:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtUSampler2D, qual, (yylsp[(1) - (1)]));
    }
    break;

  case 176:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtUSampler3D, qual, (yylsp[(1) - (1)]));
    }
    break;

  case 177:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtUSamplerCube, qual, (yylsp[(1) - (1)]));
    }
    break;

  case 178:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtUSampler2DArray, qual, (yylsp[(1) - (1)]));
    }
    break;

  case 179:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtSampler2DShadow, qual, (yylsp[(1) - (1)]));
    }
    break;

  case 180:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtSamplerCubeShadow, qual, (yylsp[(1) - (1)]));
    }
    break;

  case 181:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtSampler2DArrayShadow, qual, (yylsp[(1) - (1)]));
    }
    break;

  case 182:

    {
        if (!context->supportsExtension("GL_OES_EGL_image_external")) {
            context->error((yylsp[(1) - (1)]), "unsupported type", "samplerExternalOES");
            context->recover();
        }
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtSamplerExternalOES, qual, (yylsp[(1) - (1)]));
    }
    break;

  case 183:

    {
        if (!context->supportsExtension("GL_ARB_texture_rectangle")) {
            context->error((yylsp[(1) - (1)]), "unsupported type", "sampler2DRect");
            context->recover();
        }
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtSampler2DRect, qual, (yylsp[(1) - (1)]));
    }
    break;

  case 184:

    {
        (yyval.interm.type) = (yyvsp[(1) - (1)].interm.type);
        (yyval.interm.type).qualifier = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
    }
    break;

  case 185:

    {
        
        
        
        
        TType& structure = static_cast<TVariable*>((yyvsp[(1) - (1)].lex).symbol)->getType();
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtStruct, qual, (yylsp[(1) - (1)]));
        (yyval.interm.type).userDef = &structure;
    }
    break;

  case 186:

    { if (context->enterStructDeclaration((yylsp[(2) - (3)]), *(yyvsp[(2) - (3)].lex).string)) context->recover(); }
    break;

  case 187:

    {
        (yyval.interm.type) = context->addStructure((yylsp[(1) - (6)]), (yylsp[(2) - (6)]), (yyvsp[(2) - (6)].lex).string, (yyvsp[(5) - (6)].interm.fieldList));
    }
    break;

  case 188:

    { if (context->enterStructDeclaration((yylsp[(2) - (2)]), *(yyvsp[(2) - (2)].lex).string)) context->recover(); }
    break;

  case 189:

    {
        (yyval.interm.type) = context->addStructure((yylsp[(1) - (5)]), (yyloc), NewPoolTString(""), (yyvsp[(4) - (5)].interm.fieldList));
    }
    break;

  case 190:

    {
        (yyval.interm.fieldList) = (yyvsp[(1) - (1)].interm.fieldList);
    }
    break;

  case 191:

    {
        (yyval.interm.fieldList) = (yyvsp[(1) - (2)].interm.fieldList);
        for (size_t i = 0; i < (yyvsp[(2) - (2)].interm.fieldList)->size(); ++i) {
            TField* field = (*(yyvsp[(2) - (2)].interm.fieldList))[i];
            for (size_t j = 0; j < (yyval.interm.fieldList)->size(); ++j) {
                if ((*(yyval.interm.fieldList))[j]->name() == field->name()) {
                    context->error((yylsp[(2) - (2)]), "duplicate field name in structure:", "struct", field->name().c_str());
                    context->recover();
                }
            }
            (yyval.interm.fieldList)->push_back(field);
        }
    }
    break;

  case 192:

    {
        (yyval.interm.fieldList) = context->addStructDeclaratorList((yyvsp[(1) - (3)].interm.type), (yyvsp[(2) - (3)].interm.fieldList));
    }
    break;

  case 193:

    {
        
        (yyvsp[(2) - (4)].interm.type).qualifier = (yyvsp[(1) - (4)].interm.type).qualifier;
        (yyvsp[(2) - (4)].interm.type).layoutQualifier = (yyvsp[(1) - (4)].interm.type).layoutQualifier;
        (yyval.interm.fieldList) = context->addStructDeclaratorList((yyvsp[(2) - (4)].interm.type), (yyvsp[(3) - (4)].interm.fieldList));
    }
    break;

  case 194:

    {
        (yyval.interm.fieldList) = NewPoolTFieldList();
        (yyval.interm.fieldList)->push_back((yyvsp[(1) - (1)].interm.field));
    }
    break;

  case 195:

    {
        (yyval.interm.fieldList)->push_back((yyvsp[(3) - (3)].interm.field));
    }
    break;

  case 196:

    {
        if (context->reservedErrorCheck((yylsp[(1) - (1)]), *(yyvsp[(1) - (1)].lex).string))
            context->recover();

        TType* type = new TType(EbtVoid, EbpUndefined);
        (yyval.interm.field) = new TField(type, (yyvsp[(1) - (1)].lex).string, (yylsp[(1) - (1)]));
    }
    break;

  case 197:

    {
        if (context->reservedErrorCheck((yylsp[(1) - (4)]), *(yyvsp[(1) - (4)].lex).string))
            context->recover();

        TType* type = new TType(EbtVoid, EbpUndefined);
        int size;
        if (context->arraySizeErrorCheck((yylsp[(3) - (4)]), (yyvsp[(3) - (4)].interm.intermTypedNode), size))
            context->recover();
        type->setArraySize(size);

        (yyval.interm.field) = new TField(type, (yyvsp[(1) - (4)].lex).string, (yylsp[(1) - (4)]));
    }
    break;

  case 198:

    { (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode); }
    break;

  case 199:

    { (yyval.interm.intermNode) = (yyvsp[(1) - (1)].interm.intermNode); }
    break;

  case 200:

    { (yyval.interm.intermNode) = (yyvsp[(1) - (1)].interm.intermAggregate); }
    break;

  case 201:

    { (yyval.interm.intermNode) = (yyvsp[(1) - (1)].interm.intermNode); }
    break;

  case 202:

    { (yyval.interm.intermNode) = (yyvsp[(1) - (1)].interm.intermNode); }
    break;

  case 203:

    { (yyval.interm.intermNode) = (yyvsp[(1) - (1)].interm.intermNode); }
    break;

  case 204:

    { (yyval.interm.intermNode) = (yyvsp[(1) - (1)].interm.intermNode); }
    break;

  case 205:

    { (yyval.interm.intermNode) = (yyvsp[(1) - (1)].interm.intermNode); }
    break;

  case 206:

    { (yyval.interm.intermNode) = (yyvsp[(1) - (1)].interm.intermNode); }
    break;

  case 207:

    { (yyval.interm.intermAggregate) = 0; }
    break;

  case 208:

    { context->symbolTable.push(); }
    break;

  case 209:

    { context->symbolTable.pop(); }
    break;

  case 210:

    {
        if ((yyvsp[(3) - (5)].interm.intermAggregate) != 0) {
            (yyvsp[(3) - (5)].interm.intermAggregate)->setOp(EOpSequence);
            (yyvsp[(3) - (5)].interm.intermAggregate)->setLine((yyloc));
        }
        (yyval.interm.intermAggregate) = (yyvsp[(3) - (5)].interm.intermAggregate);
    }
    break;

  case 211:

    { (yyval.interm.intermNode) = (yyvsp[(1) - (1)].interm.intermNode); }
    break;

  case 212:

    { (yyval.interm.intermNode) = (yyvsp[(1) - (1)].interm.intermNode); }
    break;

  case 213:

    { context->symbolTable.push(); }
    break;

  case 214:

    { context->symbolTable.pop(); (yyval.interm.intermNode) = (yyvsp[(2) - (2)].interm.intermNode); }
    break;

  case 215:

    { context->symbolTable.push(); }
    break;

  case 216:

    { context->symbolTable.pop(); (yyval.interm.intermNode) = (yyvsp[(2) - (2)].interm.intermNode); }
    break;

  case 217:

    {
        (yyval.interm.intermNode) = 0;
    }
    break;

  case 218:

    {
        if ((yyvsp[(2) - (3)].interm.intermAggregate)) {
            (yyvsp[(2) - (3)].interm.intermAggregate)->setOp(EOpSequence);
            (yyvsp[(2) - (3)].interm.intermAggregate)->setLine((yyloc));
        }
        (yyval.interm.intermNode) = (yyvsp[(2) - (3)].interm.intermAggregate);
    }
    break;

  case 219:

    {
        (yyval.interm.intermAggregate) = context->intermediate.makeAggregate((yyvsp[(1) - (1)].interm.intermNode), (yyloc));
    }
    break;

  case 220:

    {
        (yyval.interm.intermAggregate) = context->intermediate.growAggregate((yyvsp[(1) - (2)].interm.intermAggregate), (yyvsp[(2) - (2)].interm.intermNode), (yyloc));
    }
    break;

  case 221:

    { (yyval.interm.intermNode) = 0; }
    break;

  case 222:

    { (yyval.interm.intermNode) = static_cast<TIntermNode*>((yyvsp[(1) - (2)].interm.intermTypedNode)); }
    break;

  case 223:

    {
        if (context->boolErrorCheck((yylsp[(1) - (5)]), (yyvsp[(3) - (5)].interm.intermTypedNode)))
            context->recover();
        (yyval.interm.intermNode) = context->intermediate.addSelection((yyvsp[(3) - (5)].interm.intermTypedNode), (yyvsp[(5) - (5)].interm.nodePair), (yylsp[(1) - (5)]));
    }
    break;

  case 224:

    {
        (yyval.interm.nodePair).node1 = (yyvsp[(1) - (3)].interm.intermNode);
        (yyval.interm.nodePair).node2 = (yyvsp[(3) - (3)].interm.intermNode);
    }
    break;

  case 225:

    {
        (yyval.interm.nodePair).node1 = (yyvsp[(1) - (1)].interm.intermNode);
        (yyval.interm.nodePair).node2 = 0;
    }
    break;

  case 226:

    {
        (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode);
        if (context->boolErrorCheck((yyvsp[(1) - (1)].interm.intermTypedNode)->getLine(), (yyvsp[(1) - (1)].interm.intermTypedNode)))
            context->recover();
    }
    break;

  case 227:

    {
        TIntermNode* intermNode;
        if (context->structQualifierErrorCheck((yylsp[(2) - (4)]), (yyvsp[(1) - (4)].interm.type)))
            context->recover();
        if (context->boolErrorCheck((yylsp[(2) - (4)]), (yyvsp[(1) - (4)].interm.type)))
            context->recover();

        if (!context->executeInitializer((yylsp[(2) - (4)]), *(yyvsp[(2) - (4)].lex).string, (yyvsp[(1) - (4)].interm.type), (yyvsp[(4) - (4)].interm.intermTypedNode), intermNode))
            (yyval.interm.intermTypedNode) = (yyvsp[(4) - (4)].interm.intermTypedNode);
        else {
            context->recover();
            (yyval.interm.intermTypedNode) = 0;
        }
    }
    break;

  case 228:

    { context->symbolTable.push(); ++context->loopNestingLevel; }
    break;

  case 229:

    {
        context->symbolTable.pop();
        (yyval.interm.intermNode) = context->intermediate.addLoop(ELoopWhile, 0, (yyvsp[(4) - (6)].interm.intermTypedNode), 0, (yyvsp[(6) - (6)].interm.intermNode), (yylsp[(1) - (6)]));
        --context->loopNestingLevel;
    }
    break;

  case 230:

    { ++context->loopNestingLevel; }
    break;

  case 231:

    {
        if (context->boolErrorCheck((yylsp[(8) - (8)]), (yyvsp[(6) - (8)].interm.intermTypedNode)))
            context->recover();

        (yyval.interm.intermNode) = context->intermediate.addLoop(ELoopDoWhile, 0, (yyvsp[(6) - (8)].interm.intermTypedNode), 0, (yyvsp[(3) - (8)].interm.intermNode), (yylsp[(4) - (8)]));
        --context->loopNestingLevel;
    }
    break;

  case 232:

    { context->symbolTable.push(); ++context->loopNestingLevel; }
    break;

  case 233:

    {
        context->symbolTable.pop();
        (yyval.interm.intermNode) = context->intermediate.addLoop(ELoopFor, (yyvsp[(4) - (7)].interm.intermNode), reinterpret_cast<TIntermTyped*>((yyvsp[(5) - (7)].interm.nodePair).node1), reinterpret_cast<TIntermTyped*>((yyvsp[(5) - (7)].interm.nodePair).node2), (yyvsp[(7) - (7)].interm.intermNode), (yylsp[(1) - (7)]));
        --context->loopNestingLevel;
    }
    break;

  case 234:

    {
        (yyval.interm.intermNode) = (yyvsp[(1) - (1)].interm.intermNode);
    }
    break;

  case 235:

    {
        (yyval.interm.intermNode) = (yyvsp[(1) - (1)].interm.intermNode);
    }
    break;

  case 236:

    {
        (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode);
    }
    break;

  case 237:

    {
        (yyval.interm.intermTypedNode) = 0;
    }
    break;

  case 238:

    {
        (yyval.interm.nodePair).node1 = (yyvsp[(1) - (2)].interm.intermTypedNode);
        (yyval.interm.nodePair).node2 = 0;
    }
    break;

  case 239:

    {
        (yyval.interm.nodePair).node1 = (yyvsp[(1) - (3)].interm.intermTypedNode);
        (yyval.interm.nodePair).node2 = (yyvsp[(3) - (3)].interm.intermTypedNode);
    }
    break;

  case 240:

    {
        if (context->loopNestingLevel <= 0) {
            context->error((yylsp[(1) - (2)]), "continue statement only allowed in loops", "");
            context->recover();
        }
        (yyval.interm.intermNode) = context->intermediate.addBranch(EOpContinue, (yylsp[(1) - (2)]));
    }
    break;

  case 241:

    {
        if (context->loopNestingLevel <= 0) {
            context->error((yylsp[(1) - (2)]), "break statement only allowed in loops", "");
            context->recover();
        }
        (yyval.interm.intermNode) = context->intermediate.addBranch(EOpBreak, (yylsp[(1) - (2)]));
    }
    break;

  case 242:

    {
        (yyval.interm.intermNode) = context->intermediate.addBranch(EOpReturn, (yylsp[(1) - (2)]));
        if (context->currentFunctionType->getBasicType() != EbtVoid) {
            context->error((yylsp[(1) - (2)]), "non-void function must return a value", "return");
            context->recover();
        }
    }
    break;

  case 243:

    {
        (yyval.interm.intermNode) = context->intermediate.addBranch(EOpReturn, (yyvsp[(2) - (3)].interm.intermTypedNode), (yylsp[(1) - (3)]));
        context->functionReturnsValue = true;
        if (context->currentFunctionType->getBasicType() == EbtVoid) {
            context->error((yylsp[(1) - (3)]), "void function cannot return a value", "return");
            context->recover();
        } else if (*(context->currentFunctionType) != (yyvsp[(2) - (3)].interm.intermTypedNode)->getType()) {
            context->error((yylsp[(1) - (3)]), "function return is not matching type:", "return");
            context->recover();
        }
    }
    break;

  case 244:

    {
        FRAG_ONLY("discard", (yylsp[(1) - (2)]));
        (yyval.interm.intermNode) = context->intermediate.addBranch(EOpKill, (yylsp[(1) - (2)]));
    }
    break;

  case 245:

    {
        (yyval.interm.intermNode) = (yyvsp[(1) - (1)].interm.intermNode);
        context->treeRoot = (yyval.interm.intermNode);
    }
    break;

  case 246:

    {
        (yyval.interm.intermNode) = context->intermediate.growAggregate((yyvsp[(1) - (2)].interm.intermNode), (yyvsp[(2) - (2)].interm.intermNode), (yyloc));
        context->treeRoot = (yyval.interm.intermNode);
    }
    break;

  case 247:

    {
        (yyval.interm.intermNode) = (yyvsp[(1) - (1)].interm.intermNode);
    }
    break;

  case 248:

    {
        (yyval.interm.intermNode) = (yyvsp[(1) - (1)].interm.intermNode);
    }
    break;

  case 249:

    {
        TFunction* function = (yyvsp[(1) - (1)].interm).function;
        
        const TSymbol *builtIn = context->symbolTable.findBuiltIn(function->getMangledName(), context->shaderVersion);
        
        if (builtIn)
        {
            context->error((yylsp[(1) - (1)]), "built-in functions cannot be redefined", function->getName().c_str());
            context->recover();
        }
        
        TFunction* prevDec = static_cast<TFunction*>(context->symbolTable.find(function->getMangledName(), context->shaderVersion));
        
        
        
        
        
        if (prevDec->isDefined()) {
            
            
            
            context->error((yylsp[(1) - (1)]), "function already has a body", function->getName().c_str());
            context->recover();
        }
        prevDec->setDefined();

        
        
        
        if (function->getName() == "main") {
            if (function->getParamCount() > 0) {
                context->error((yylsp[(1) - (1)]), "function cannot take any parameter(s)", function->getName().c_str());
                context->recover();
            }
            if (function->getReturnType().getBasicType() != EbtVoid) {
                context->error((yylsp[(1) - (1)]), "", function->getReturnType().getBasicString(), "main function cannot return a value");
                context->recover();
            }
        }

        
        
        
        context->currentFunctionType = &(prevDec->getReturnType());
        context->functionReturnsValue = false;

        
        
        
        
        
        
        
        
        TIntermAggregate* paramNodes = new TIntermAggregate;
        for (size_t i = 0; i < function->getParamCount(); i++) {
            const TParameter& param = function->getParam(i);
            if (param.name != 0) {
                TVariable *variable = new TVariable(param.name, *param.type);
                
                
                
                if (! context->symbolTable.declare(variable)) {
                    context->error((yylsp[(1) - (1)]), "redefinition", variable->getName().c_str());
                    context->recover();
                    delete variable;
                }

                
                
                
                paramNodes = context->intermediate.growAggregate(
                                               paramNodes,
                                               context->intermediate.addSymbol(variable->getUniqueId(),
                                                                       variable->getName(),
                                                                       variable->getType(), (yylsp[(1) - (1)])),
                                               (yylsp[(1) - (1)]));
            } else {
                paramNodes = context->intermediate.growAggregate(paramNodes, context->intermediate.addSymbol(0, "", *param.type, (yylsp[(1) - (1)])), (yylsp[(1) - (1)]));
            }
        }
        context->intermediate.setAggregateOperator(paramNodes, EOpParameters, (yylsp[(1) - (1)]));
        (yyvsp[(1) - (1)].interm).intermAggregate = paramNodes;
        context->loopNestingLevel = 0;
    }
    break;

  case 250:

    {
        
        
        if (context->currentFunctionType->getBasicType() != EbtVoid && ! context->functionReturnsValue) {
            context->error((yylsp[(1) - (3)]), "function does not return a value:", "", (yyvsp[(1) - (3)].interm).function->getName().c_str());
            context->recover();
        }
        
        (yyval.interm.intermNode) = context->intermediate.growAggregate((yyvsp[(1) - (3)].interm).intermAggregate, (yyvsp[(3) - (3)].interm.intermNode), (yyloc));
        context->intermediate.setAggregateOperator((yyval.interm.intermNode), EOpFunction, (yylsp[(1) - (3)]));
        (yyval.interm.intermNode)->getAsAggregate()->setName((yyvsp[(1) - (3)].interm).function->getMangledName().c_str());
        (yyval.interm.intermNode)->getAsAggregate()->setType((yyvsp[(1) - (3)].interm).function->getReturnType());

        
        
        (yyval.interm.intermNode)->getAsAggregate()->setOptimize(context->pragma().optimize);
        (yyval.interm.intermNode)->getAsAggregate()->setDebug(context->pragma().debug);

        context->symbolTable.pop();
    }
    break;



      default: break;
    }
  










  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;
  *++yylsp = yyloc;

  



  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;





yyerrlab:
  

  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (&yylloc, context, YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (&yylloc, context, yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }

  yyerror_range[1] = yylloc;

  if (yyerrstatus == 3)
    {
      


      if (yychar <= YYEOF)
	{
	  
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval, &yylloc, context);
	  yychar = YYEMPTY;
	}
    }

  

  goto yyerrlab1;





yyerrorlab:

  


  if ( 0)
     goto yyerrorlab;

  yyerror_range[1] = yylsp[1-yylen];
  

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;





yyerrlab1:
  yyerrstatus = 3;	

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      
      if (yyssp == yyss)
	YYABORT;

      yyerror_range[1] = *yylsp;
      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp, yylsp, context);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  yyerror_range[2] = yylloc;
  

  YYLLOC_DEFAULT (yyloc, yyerror_range, 2);
  *++yylsp = yyloc;

  
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;





yyacceptlab:
  yyresult = 0;
  goto yyreturn;




yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined yyoverflow || YYERROR_VERBOSE



yyexhaustedlab:
  yyerror (&yylloc, context, YY_("memory exhausted"));
  yyresult = 2;
  
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      

      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, &yylloc, context);
    }
  

  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp, yylsp, context);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  
  return YYID (yyresult);
}





int glslang_parse(TParseContext* context) {
    return yyparse(context);
}
