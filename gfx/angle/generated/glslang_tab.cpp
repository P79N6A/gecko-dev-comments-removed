












































#define YYBISON 1


#define YYBISON_VERSION "2.4.1"


#define YYSKELETON_NAME "yacc.c"


#define YYPURE 1


#define YYPUSH 0


#define YYPULL 1


#define YYLSP_NEEDED 0






















#include "compiler/SymbolTable.h"
#include "compiler/ParseHelper.h"
#include "GLSLANG/ShaderLang.h"

#define YYPARSE_PARAM parseContextLocal




#define YYPARSE_PARAM_DECL void*
#define parseContext ((TParseContext*)(parseContextLocal))
#define YYLEX_PARAM parseContextLocal
#define YY_DECL int yylex(YYSTYPE* pyylval, void* parseContextLocal)
extern void yyerror(char*);

#define FRAG_VERT_ONLY(S, L) {                                                  \
    if (parseContext->language != EShLangFragment &&                             \
        parseContext->language != EShLangVertex) {                               \
        parseContext->error(L, " supported in vertex/fragment shaders only ", S, "", "");   \
        parseContext->recover();                                                            \
    }                                                                           \
}

#define VERTEX_ONLY(S, L) {                                                     \
    if (parseContext->language != EShLangVertex) {                               \
        parseContext->error(L, " supported in vertex shaders only ", S, "", "");            \
        parseContext->recover();                                                            \
    }                                                                           \
}

#define FRAG_ONLY(S, L) {                                                       \
    if (parseContext->language != EShLangFragment) {                             \
        parseContext->error(L, " supported in fragment shaders only ", S, "", "");          \
        parseContext->recover();                                                            \
    }                                                                           \
}

#define PACK_ONLY(S, L) {                                                       \
    if (parseContext->language != EShLangPack) {                                 \
        parseContext->error(L, " supported in pack shaders only ", S, "", "");              \
        parseContext->recover();                                                            \
    }                                                                           \
}

#define UNPACK_ONLY(S, L) {                                                     \
    if (parseContext->language != EShLangUnpack) {                               \
        parseContext->error(L, " supported in unpack shaders only ", S, "", "");            \
        parseContext->recover();                                                            \
    }                                                                           \
}

#define PACK_UNPACK_ONLY(S, L) {                                                \
    if (parseContext->language != EShLangUnpack &&                               \
        parseContext->language != EShLangPack) {                                 \
        parseContext->error(L, " supported in pack/unpack shaders only ", S, "", "");       \
        parseContext->recover();                                                            \
    }                                                                           \
}




#ifndef YYDEBUG
# define YYDEBUG 0
#endif


#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif


#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif



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
     IDENTIFIER = 298,
     TYPE_NAME = 299,
     FLOATCONSTANT = 300,
     INTCONSTANT = 301,
     BOOLCONSTANT = 302,
     FIELD_SELECTION = 303,
     LEFT_OP = 304,
     RIGHT_OP = 305,
     INC_OP = 306,
     DEC_OP = 307,
     LE_OP = 308,
     GE_OP = 309,
     EQ_OP = 310,
     NE_OP = 311,
     AND_OP = 312,
     OR_OP = 313,
     XOR_OP = 314,
     MUL_ASSIGN = 315,
     DIV_ASSIGN = 316,
     ADD_ASSIGN = 317,
     MOD_ASSIGN = 318,
     LEFT_ASSIGN = 319,
     RIGHT_ASSIGN = 320,
     AND_ASSIGN = 321,
     XOR_ASSIGN = 322,
     OR_ASSIGN = 323,
     SUB_ASSIGN = 324,
     LEFT_PAREN = 325,
     RIGHT_PAREN = 326,
     LEFT_BRACKET = 327,
     RIGHT_BRACKET = 328,
     LEFT_BRACE = 329,
     RIGHT_BRACE = 330,
     DOT = 331,
     COMMA = 332,
     COLON = 333,
     EQUAL = 334,
     SEMICOLON = 335,
     BANG = 336,
     DASH = 337,
     TILDE = 338,
     PLUS = 339,
     STAR = 340,
     SLASH = 341,
     PERCENT = 342,
     LEFT_ANGLE = 343,
     RIGHT_ANGLE = 344,
     VERTICAL_BAR = 345,
     CARET = 346,
     AMPERSAND = 347,
     QUESTION = 348
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{


    struct {
        TSourceLoc line;
        union {
            TString *string;
            float f;
            int i;
            bool b;
        };
        TSymbol* symbol;
    } lex;
    struct {
        TSourceLoc line;
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
            TTypeLine typeLine;
            TTypeList* typeList;
        };
    } interm;



} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif





    extern int yylex(YYSTYPE*, void*);



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
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> 
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif


#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e)
#endif


#ifndef lint
# define YYID(n) (n)
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
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> 
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
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
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> 
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); 
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); 
#   endif
#  endif
# endif
#endif 


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))


union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};


# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)



# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)



# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif






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


#define YYFINAL  69

#define YYLAST   1378


#define YYNTOKENS  94

#define YYNNTS  78

#define YYNRULES  195

#define YYNSTATES  300


#define YYUNDEFTOK  2
#define YYMAXUTOK   348

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
      85,    86,    87,    88,    89,    90,    91,    92,    93
};

#if YYDEBUG


static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     5,     7,     9,    11,    13,    17,    19,
      24,    26,    30,    33,    36,    38,    40,    42,    46,    49,
      52,    55,    57,    60,    64,    67,    69,    71,    73,    75,
      78,    81,    84,    86,    88,    90,    92,    96,   100,   102,
     106,   110,   112,   114,   118,   122,   126,   130,   132,   136,
     140,   142,   144,   146,   148,   152,   154,   158,   160,   164,
     166,   172,   174,   178,   180,   182,   184,   186,   188,   190,
     194,   196,   199,   202,   207,   210,   212,   214,   217,   221,
     225,   228,   234,   238,   241,   245,   248,   249,   251,   253,
     255,   257,   259,   263,   269,   276,   284,   293,   299,   301,
     304,   309,   315,   320,   323,   325,   328,   330,   332,   334,
     337,   339,   341,   344,   346,   348,   350,   352,   357,   359,
     361,   363,   365,   367,   369,   371,   373,   375,   377,   379,
     381,   383,   385,   387,   389,   391,   393,   395,   397,   403,
     408,   410,   413,   417,   419,   423,   425,   430,   432,   434,
     436,   438,   440,   442,   444,   446,   448,   451,   452,   453,
     459,   461,   463,   466,   470,   472,   475,   477,   480,   486,
     490,   492,   494,   499,   500,   507,   508,   517,   518,   526,
     528,   530,   532,   533,   536,   540,   543,   546,   549,   553,
     556,   558,   561,   563,   565,   566
};


static const yytype_int16 yyrhs[] =
{
     168,     0,    -1,    43,    -1,    95,    -1,    46,    -1,    45,
      -1,    47,    -1,    70,   122,    71,    -1,    96,    -1,    97,
      72,    98,    73,    -1,    99,    -1,    97,    76,    48,    -1,
      97,    51,    -1,    97,    52,    -1,   122,    -1,   100,    -1,
     101,    -1,    97,    76,   101,    -1,   103,    71,    -1,   102,
      71,    -1,   104,    39,    -1,   104,    -1,   104,   120,    -1,
     103,    77,   120,    -1,   105,    70,    -1,   137,    -1,    43,
      -1,    48,    -1,    97,    -1,    51,   106,    -1,    52,   106,
      -1,   107,   106,    -1,    84,    -1,    82,    -1,    81,    -1,
     106,    -1,   108,    85,   106,    -1,   108,    86,   106,    -1,
     108,    -1,   109,    84,   108,    -1,   109,    82,   108,    -1,
     109,    -1,   110,    -1,   111,    88,   110,    -1,   111,    89,
     110,    -1,   111,    53,   110,    -1,   111,    54,   110,    -1,
     111,    -1,   112,    55,   111,    -1,   112,    56,   111,    -1,
     112,    -1,   113,    -1,   114,    -1,   115,    -1,   116,    57,
     115,    -1,   116,    -1,   117,    59,   116,    -1,   117,    -1,
     118,    58,   117,    -1,   118,    -1,   118,    93,   122,    78,
     120,    -1,   119,    -1,   106,   121,   120,    -1,    79,    -1,
      60,    -1,    61,    -1,    62,    -1,    69,    -1,   120,    -1,
     122,    77,   120,    -1,   119,    -1,   125,    80,    -1,   133,
      80,    -1,     7,   138,   139,    80,    -1,   126,    71,    -1,
     128,    -1,   127,    -1,   128,   130,    -1,   127,    77,   130,
      -1,   135,    43,    70,    -1,   137,    43,    -1,   137,    43,
      72,   123,    73,    -1,   136,   131,   129,    -1,   131,   129,
      -1,   136,   131,   132,    -1,   131,   132,    -1,    -1,    33,
      -1,    34,    -1,    35,    -1,   137,    -1,   134,    -1,   133,
      77,    43,    -1,   133,    77,    43,    72,    73,    -1,   133,
      77,    43,    72,   123,    73,    -1,   133,    77,    43,    72,
      73,    79,   146,    -1,   133,    77,    43,    72,   123,    73,
      79,   146,    -1,   133,    77,    43,    79,   146,    -1,   135,
      -1,   135,    43,    -1,   135,    43,    72,    73,    -1,   135,
      43,    72,   123,    73,    -1,   135,    43,    79,   146,    -1,
       3,    43,    -1,   137,    -1,   136,   137,    -1,     9,    -1,
       8,    -1,    37,    -1,     3,    37,    -1,    36,    -1,   139,
      -1,   138,   139,    -1,     4,    -1,     5,    -1,     6,    -1,
     140,    -1,   140,    72,   123,    73,    -1,    39,    -1,    11,
      -1,    12,    -1,    10,    -1,    27,    -1,    28,    -1,    29,
      -1,    21,    -1,    22,    -1,    23,    -1,    24,    -1,    25,
      -1,    26,    -1,    30,    -1,    31,    -1,    32,    -1,    41,
      -1,    42,    -1,   141,    -1,    44,    -1,    38,    43,    74,
     142,    75,    -1,    38,    74,   142,    75,    -1,   143,    -1,
     142,   143,    -1,   137,   144,    80,    -1,   145,    -1,   144,
      77,   145,    -1,    43,    -1,    43,    72,   123,    73,    -1,
     120,    -1,   124,    -1,   150,    -1,   149,    -1,   147,    -1,
     156,    -1,   157,    -1,   160,    -1,   167,    -1,    74,    75,
      -1,    -1,    -1,    74,   151,   155,   152,    75,    -1,   154,
      -1,   149,    -1,    74,    75,    -1,    74,   155,    75,    -1,
     148,    -1,   155,   148,    -1,    80,    -1,   122,    80,    -1,
      18,    70,   122,    71,   158,    -1,   148,    16,   148,    -1,
     148,    -1,   122,    -1,   135,    43,    79,   146,    -1,    -1,
      40,    70,   161,   159,    71,   153,    -1,    -1,    15,   162,
     148,    40,    70,   122,    71,    80,    -1,    -1,    17,    70,
     163,   164,   166,    71,   153,    -1,   156,    -1,   147,    -1,
     159,    -1,    -1,   165,    80,    -1,   165,    80,   122,    -1,
      14,    80,    -1,    13,    80,    -1,    20,    80,    -1,    20,
     122,    80,    -1,    19,    80,    -1,   169,    -1,   168,   169,
      -1,   170,    -1,   124,    -1,    -1,   125,   171,   154,    -1
};


static const yytype_uint16 yyrline[] =
{
       0,   183,   183,   218,   221,   234,   239,   244,   250,   253,
     326,   329,   438,   448,   461,   469,   567,   570,   578,   582,
     589,   593,   600,   606,   615,   623,   687,   694,   704,   707,
     717,   727,   748,   749,   750,   755,   756,   765,   777,   778,
     786,   797,   801,   802,   812,   822,   832,   845,   846,   857,
     871,   875,   879,   883,   884,   897,   898,   911,   912,   925,
     926,   943,   944,   958,   959,   960,   961,   962,   966,   969,
     980,   988,  1013,  1018,  1025,  1061,  1064,  1071,  1079,  1100,
    1119,  1130,  1159,  1164,  1174,  1179,  1189,  1192,  1195,  1198,
    1204,  1211,  1221,  1233,  1251,  1275,  1308,  1344,  1367,  1371,
    1385,  1405,  1434,  1454,  1530,  1540,  1566,  1569,  1575,  1583,
    1591,  1599,  1602,  1609,  1612,  1615,  1621,  1624,  1639,  1643,
    1647,  1651,  1660,  1665,  1670,  1675,  1680,  1685,  1690,  1695,
    1700,  1705,  1711,  1717,  1723,  1728,  1733,  1738,  1751,  1764,
    1772,  1775,  1790,  1816,  1820,  1826,  1834,  1850,  1854,  1858,
    1859,  1865,  1866,  1867,  1868,  1869,  1873,  1874,  1874,  1874,
    1882,  1883,  1888,  1891,  1899,  1902,  1908,  1909,  1913,  1921,
    1925,  1935,  1940,  1957,  1957,  1962,  1962,  1969,  1969,  1977,
    1980,  1986,  1989,  1995,  1999,  2006,  2013,  2020,  2027,  2038,
    2047,  2051,  2058,  2061,  2067,  2067
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE


static const char *const yytname[] =
{
  "$end", "error", "$undefined", "INVARIANT", "HIGH_PRECISION",
  "MEDIUM_PRECISION", "LOW_PRECISION", "PRECISION", "ATTRIBUTE",
  "CONST_QUAL", "BOOL_TYPE", "FLOAT_TYPE", "INT_TYPE", "BREAK", "CONTINUE",
  "DO", "ELSE", "FOR", "IF", "DISCARD", "RETURN", "BVEC2", "BVEC3",
  "BVEC4", "IVEC2", "IVEC3", "IVEC4", "VEC2", "VEC3", "VEC4", "MATRIX2",
  "MATRIX3", "MATRIX4", "IN_QUAL", "OUT_QUAL", "INOUT_QUAL", "UNIFORM",
  "VARYING", "STRUCT", "VOID_TYPE", "WHILE", "SAMPLER2D", "SAMPLERCUBE",
  "IDENTIFIER", "TYPE_NAME", "FLOATCONSTANT", "INTCONSTANT",
  "BOOLCONSTANT", "FIELD_SELECTION", "LEFT_OP", "RIGHT_OP", "INC_OP",
  "DEC_OP", "LE_OP", "GE_OP", "EQ_OP", "NE_OP", "AND_OP", "OR_OP",
  "XOR_OP", "MUL_ASSIGN", "DIV_ASSIGN", "ADD_ASSIGN", "MOD_ASSIGN",
  "LEFT_ASSIGN", "RIGHT_ASSIGN", "AND_ASSIGN", "XOR_ASSIGN", "OR_ASSIGN",
  "SUB_ASSIGN", "LEFT_PAREN", "RIGHT_PAREN", "LEFT_BRACKET",
  "RIGHT_BRACKET", "LEFT_BRACE", "RIGHT_BRACE", "DOT", "COMMA", "COLON",
  "EQUAL", "SEMICOLON", "BANG", "DASH", "TILDE", "PLUS", "STAR", "SLASH",
  "PERCENT", "LEFT_ANGLE", "RIGHT_ANGLE", "VERTICAL_BAR", "CARET",
  "AMPERSAND", "QUESTION", "$accept", "variable_identifier",
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
  "constant_expression", "declaration", "function_prototype",
  "function_declarator", "function_header_with_parameters",
  "function_header", "parameter_declarator", "parameter_declaration",
  "parameter_qualifier", "parameter_type_specifier",
  "init_declarator_list", "single_declaration", "fully_specified_type",
  "type_qualifier", "type_specifier", "precision_qualifier",
  "type_specifier_no_prec", "type_specifier_nonarray", "struct_specifier",
  "struct_declaration_list", "struct_declaration",
  "struct_declarator_list", "struct_declarator", "initializer",
  "declaration_statement", "statement", "simple_statement",
  "compound_statement", "$@1", "$@2", "statement_no_new_scope",
  "compound_statement_no_new_scope", "statement_list",
  "expression_statement", "selection_statement",
  "selection_rest_statement", "condition", "iteration_statement", "$@3",
  "$@4", "$@5", "for_init_statement", "conditionopt", "for_rest_statement",
  "jump_statement", "translation_unit", "external_declaration",
  "function_definition", "$@6", 0
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
     345,   346,   347,   348
};
# endif


static const yytype_uint8 yyr1[] =
{
       0,    94,    95,    96,    96,    96,    96,    96,    97,    97,
      97,    97,    97,    97,    98,    99,   100,   100,   101,   101,
     102,   102,   103,   103,   104,   105,   105,   105,   106,   106,
     106,   106,   107,   107,   107,   108,   108,   108,   109,   109,
     109,   110,   111,   111,   111,   111,   111,   112,   112,   112,
     113,   114,   115,   116,   116,   117,   117,   118,   118,   119,
     119,   120,   120,   121,   121,   121,   121,   121,   122,   122,
     123,   124,   124,   124,   125,   126,   126,   127,   127,   128,
     129,   129,   130,   130,   130,   130,   131,   131,   131,   131,
     132,   133,   133,   133,   133,   133,   133,   133,   134,   134,
     134,   134,   134,   134,   135,   135,   136,   136,   136,   136,
     136,   137,   137,   138,   138,   138,   139,   139,   140,   140,
     140,   140,   140,   140,   140,   140,   140,   140,   140,   140,
     140,   140,   140,   140,   140,   140,   140,   140,   141,   141,
     142,   142,   143,   144,   144,   145,   145,   146,   147,   148,
     148,   149,   149,   149,   149,   149,   150,   151,   152,   150,
     153,   153,   154,   154,   155,   155,   156,   156,   157,   158,
     158,   159,   159,   161,   160,   162,   160,   163,   160,   164,
     164,   165,   165,   166,   166,   167,   167,   167,   167,   167,
     168,   168,   169,   169,   171,   170
};


static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     1,     1,     1,     3,     1,     4,
       1,     3,     2,     2,     1,     1,     1,     3,     2,     2,
       2,     1,     2,     3,     2,     1,     1,     1,     1,     2,
       2,     2,     1,     1,     1,     1,     3,     3,     1,     3,
       3,     1,     1,     3,     3,     3,     3,     1,     3,     3,
       1,     1,     1,     1,     3,     1,     3,     1,     3,     1,
       5,     1,     3,     1,     1,     1,     1,     1,     1,     3,
       1,     2,     2,     4,     2,     1,     1,     2,     3,     3,
       2,     5,     3,     2,     3,     2,     0,     1,     1,     1,
       1,     1,     3,     5,     6,     7,     8,     5,     1,     2,
       4,     5,     4,     2,     1,     2,     1,     1,     1,     2,
       1,     1,     2,     1,     1,     1,     1,     4,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     5,     4,
       1,     2,     3,     1,     3,     1,     4,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     2,     0,     0,     5,
       1,     1,     2,     3,     1,     2,     1,     2,     5,     3,
       1,     1,     4,     0,     6,     0,     8,     0,     7,     1,
       1,     1,     0,     2,     3,     2,     2,     2,     3,     2,
       1,     2,     1,     1,     0,     3
};




static const yytype_uint8 yydefact[] =
{
       0,     0,   113,   114,   115,     0,   107,   106,   121,   119,
     120,   125,   126,   127,   128,   129,   130,   122,   123,   124,
     131,   132,   133,   110,   108,     0,   118,   134,   135,   137,
     193,   194,     0,    76,    86,     0,    91,    98,     0,   104,
       0,   111,   116,   136,     0,   190,   192,   109,   103,     0,
       0,     0,    71,     0,    74,    86,     0,    87,    88,    89,
      77,     0,    86,     0,    72,    99,   105,   112,     0,     1,
     191,     0,     0,     0,     0,   140,     0,   195,    78,    83,
      85,    90,     0,    92,    79,     0,     0,     2,     5,     4,
       6,    27,     0,     0,     0,    34,    33,    32,     3,     8,
      28,    10,    15,    16,     0,     0,    21,     0,    35,     0,
      38,    41,    42,    47,    50,    51,    52,    53,    55,    57,
      59,    70,     0,    25,    73,     0,   145,     0,   143,   139,
     141,     0,     0,   175,     0,     0,     0,     0,     0,   157,
     162,   166,    35,    61,    68,     0,   148,     0,   104,   151,
     164,   150,   149,     0,   152,   153,   154,   155,    80,    82,
      84,     0,     0,   100,     0,   147,   102,    29,    30,     0,
      12,    13,     0,     0,    19,    18,     0,   118,    22,    24,
      31,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   117,   138,     0,     0,   142,
     186,   185,     0,   177,     0,   189,   187,     0,   173,   156,
       0,    64,    65,    66,    67,    63,     0,     0,   167,   163,
     165,     0,    93,     0,    97,   101,     7,     0,    14,    26,
      11,    17,    23,    36,    37,    40,    39,    45,    46,    43,
      44,    48,    49,    54,    56,    58,     0,     0,   144,     0,
       0,     0,   188,     0,   158,    62,    69,     0,     0,    94,
       9,     0,   146,     0,   180,   179,   182,     0,   171,     0,
       0,     0,    81,    95,     0,    60,     0,   181,     0,     0,
     170,   168,     0,     0,   159,    96,     0,   183,     0,     0,
       0,   161,   174,   160,     0,   184,   178,   169,   172,   176
};


static const yytype_int16 yydefgoto[] =
{
      -1,    98,    99,   100,   227,   101,   102,   103,   104,   105,
     106,   107,   142,   109,   110,   111,   112,   113,   114,   115,
     116,   117,   118,   119,   120,   143,   144,   216,   145,   122,
     146,   147,    32,    33,    34,    79,    60,    61,    80,    35,
      36,    37,    38,   123,    40,    41,    42,    43,    74,    75,
     127,   128,   166,   149,   150,   151,   152,   210,   271,   292,
     293,   153,   154,   155,   281,   270,   156,   253,   202,   250,
     266,   278,   279,   157,    44,    45,    46,    53
};



#define YYPACT_NINF -253
static const yytype_int16 yypact[] =
{
    1269,   -11,  -253,  -253,  -253,   124,  -253,  -253,  -253,  -253,
    -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,
    -253,  -253,  -253,  -253,  -253,   -40,  -253,  -253,  -253,  -253,
    -253,   -65,   -52,   -50,    21,    16,  -253,   -10,  1310,  -253,
    1334,  -253,   -12,  -253,  1182,  -253,  -253,  -253,  -253,  1334,
      -3,  1310,  -253,    10,  -253,    32,    68,  -253,  -253,  -253,
    -253,  1310,   122,    78,  -253,    11,  -253,  -253,   993,  -253,
    -253,    31,  1310,    83,   202,  -253,   287,  -253,  -253,  -253,
    -253,    91,  1310,   -63,  -253,   764,   993,    80,  -253,  -253,
    -253,  -253,   993,   993,   993,  -253,  -253,  -253,  -253,  -253,
      27,  -253,  -253,  -253,    81,   -34,  1060,    90,  -253,   993,
      12,    -9,  -253,   -36,   103,  -253,  -253,  -253,   111,   110,
     -44,  -253,    97,  -253,  -253,  1127,    99,    36,  -253,  -253,
    -253,    92,   100,  -253,   107,   109,   101,   845,   112,   108,
    -253,  -253,    25,  -253,  -253,    42,  -253,   -65,   115,  -253,
    -253,  -253,  -253,   369,  -253,  -253,  -253,  -253,   114,  -253,
    -253,   912,   993,  -253,   117,  -253,  -253,  -253,  -253,    -7,
    -253,  -253,   993,  1223,  -253,  -253,   993,   116,  -253,  -253,
    -253,   993,   993,   993,   993,   993,   993,   993,   993,   993,
     993,   993,   993,   993,   993,  -253,  -253,   993,    83,  -253,
    -253,  -253,   451,  -253,   993,  -253,  -253,    43,  -253,  -253,
     451,  -253,  -253,  -253,  -253,  -253,   993,   993,  -253,  -253,
    -253,   993,   113,   118,  -253,  -253,  -253,   120,   119,  -253,
     127,  -253,  -253,  -253,  -253,    12,    12,  -253,  -253,  -253,
    -253,   -36,   -36,  -253,   111,   110,    85,   121,  -253,   148,
     615,    18,  -253,   697,   451,  -253,  -253,   125,   993,   130,
    -253,   993,  -253,   129,  -253,  -253,   697,   451,   119,   157,
     132,   126,  -253,  -253,   993,  -253,   993,  -253,   131,   133,
     199,  -253,   137,   533,  -253,  -253,    29,   993,   533,   451,
     993,  -253,  -253,  -253,   138,   119,  -253,  -253,  -253,  -253
};


static const yytype_int16 yypgoto[] =
{
    -253,  -253,  -253,  -253,  -253,  -253,  -253,    44,  -253,  -253,
    -253,  -253,   -46,  -253,   -19,  -253,   -78,   -23,  -253,  -253,
    -253,    28,    30,    45,  -253,   -43,   -85,  -253,   -92,   -73,
       4,     6,  -253,  -253,  -253,   139,   165,   173,   154,  -253,
    -253,  -243,   -27,     0,   232,   -29,  -253,  -253,   167,   -66,
    -253,    47,  -157,    -8,  -140,  -252,  -253,  -253,  -253,   -41,
     195,    39,     1,  -253,  -253,   -14,  -253,  -253,  -253,  -253,
    -253,  -253,  -253,  -253,  -253,   211,  -253,  -253
};





#define YYTABLE_NINF -76
static const yytype_int16 yytable[] =
{
      39,   165,   169,    50,    30,   224,    31,    62,   130,   161,
     269,    67,   164,   220,   193,    52,   162,   185,   186,    54,
      71,   178,   108,   269,    56,   121,    47,    55,    62,     6,
       7,   291,    48,    65,    51,    56,   291,   175,    66,   108,
       6,     7,   121,   176,    39,   207,   167,   168,    30,   194,
      31,    73,   187,   188,    57,    58,    59,    23,    24,   130,
      68,    81,   249,   180,   226,    57,    58,    59,    23,    24,
     217,    72,    73,   183,    73,   184,   148,   165,   170,   171,
     228,    84,    81,    85,    76,   211,   212,   213,   223,   267,
      86,   232,   -75,    63,   214,   217,    64,   181,   182,   172,
     294,   273,   246,   173,   215,    47,   217,   237,   238,   239,
     240,   124,   251,   198,   220,   108,   199,   285,   121,   217,
     217,    83,   218,   252,   247,    73,   126,   280,     2,     3,
       4,   255,   256,   298,   158,   233,   234,   108,   108,   108,
     108,   108,   108,   108,   108,   108,   108,   108,   257,   297,
     -26,   108,   174,   148,   121,    57,    58,    59,   189,   190,
     179,   268,   217,   261,   235,   236,   241,   242,   191,   192,
     195,   197,   200,   165,   268,   108,   275,   203,   121,   204,
     201,   205,   208,   209,   286,   -25,   221,   -20,   263,   165,
     225,   259,   258,   260,   262,   295,   217,   -27,   272,   276,
     282,   284,   148,   283,   288,   165,     2,     3,     4,   274,
     148,   287,     8,     9,    10,   289,   290,   231,   299,   243,
      78,   159,   244,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    82,   160,    49,   245,   125,
      25,    26,   264,    27,    28,   248,    29,   296,    77,   254,
     148,   265,   277,   148,   148,    70,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   148,   148,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   129,     0,     0,
       0,     0,     0,   148,     0,     0,     0,     0,   148,   148,
       1,     2,     3,     4,     5,     6,     7,     8,     9,    10,
     131,   132,   133,     0,   134,   135,   136,   137,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
       0,     0,     0,    23,    24,    25,    26,   138,    27,    28,
      87,    29,    88,    89,    90,    91,     0,     0,    92,    93,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    94,     0,     0,
       0,   139,   140,     0,     0,     0,     0,   141,    95,    96,
       0,    97,     1,     2,     3,     4,     5,     6,     7,     8,
       9,    10,   131,   132,   133,     0,   134,   135,   136,   137,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,     0,     0,     0,    23,    24,    25,    26,   138,
      27,    28,    87,    29,    88,    89,    90,    91,     0,     0,
      92,    93,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    94,
       0,     0,     0,   139,   219,     0,     0,     0,     0,   141,
      95,    96,     0,    97,     1,     2,     3,     4,     5,     6,
       7,     8,     9,    10,   131,   132,   133,     0,   134,   135,
     136,   137,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,     0,     0,     0,    23,    24,    25,
      26,   138,    27,    28,    87,    29,    88,    89,    90,    91,
       0,     0,    92,    93,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    94,     0,     0,     0,   139,     0,     0,     0,     0,
       0,   141,    95,    96,     0,    97,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,   131,   132,   133,     0,
     134,   135,   136,   137,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,     0,     0,     0,    23,
      24,    25,    26,   138,    27,    28,    87,    29,    88,    89,
      90,    91,     0,     0,    92,    93,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    94,     0,     0,     0,    76,     0,     0,
       0,     0,     0,   141,    95,    96,     0,    97,     1,     2,
       3,     4,     5,     6,     7,     8,     9,    10,     0,     0,
       0,     0,     0,     0,     0,     0,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,     0,     0,
       0,    23,    24,    25,    26,     0,    27,    28,    87,    29,
      88,    89,    90,    91,     0,     0,    92,    93,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    94,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   141,    95,    96,     0,    97,
      56,     2,     3,     4,     0,     6,     7,     8,     9,    10,
       0,     0,     0,     0,     0,     0,     0,     0,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
       0,     0,     0,    23,    24,    25,    26,     0,    27,    28,
      87,    29,    88,    89,    90,    91,     0,     0,    92,    93,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    94,     2,     3,
       4,     0,     0,     0,     8,     9,    10,     0,    95,    96,
       0,    97,     0,     0,     0,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,     0,     0,     0,
       0,     0,    25,    26,     0,    27,    28,    87,    29,    88,
      89,    90,    91,     0,     0,    92,    93,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    94,     0,     0,   163,     0,     0,
       0,     0,     0,     0,     0,    95,    96,     0,    97,     2,
       3,     4,     0,     0,     0,     8,     9,    10,     0,     0,
       0,     0,     0,     0,     0,     0,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,     0,     0,
       0,     0,     0,    25,    26,     0,    27,    28,    87,    29,
      88,    89,    90,    91,     0,     0,    92,    93,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    94,     2,     3,     4,     0,
       0,     0,     8,     9,    10,   206,    95,    96,     0,    97,
       0,     0,     0,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,     0,     0,     0,     0,     0,
      25,    26,     0,    27,    28,    87,    29,    88,    89,    90,
      91,     0,     0,    92,    93,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    94,     0,     0,   222,     0,     0,     0,     0,
       0,     0,     0,    95,    96,     0,    97,     2,     3,     4,
       0,     0,     0,     8,     9,    10,     0,     0,     0,     0,
       0,     0,     0,     0,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,     0,     0,     0,     0,
       0,    25,    26,     0,    27,    28,    87,    29,    88,    89,
      90,    91,     0,     0,    92,    93,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    94,     2,     3,     4,     0,     0,     0,
       8,     9,    10,     0,    95,    96,     0,    97,     0,     0,
       0,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,     0,     0,     0,     0,     0,    25,   177,
       0,    27,    28,    87,    29,    88,    89,    90,    91,     0,
       0,    92,    93,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      94,     2,     3,     4,     0,     0,     0,     8,     9,    10,
       0,    95,    96,     0,    97,     0,     0,     0,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
       0,     0,     0,     0,     0,    25,    26,     0,    27,    28,
       0,    29,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    69,     0,     0,     1,     2,     3,     4,     5,
       6,     7,     8,     9,    10,     0,     0,     0,     0,     0,
       0,     0,   196,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,     0,     0,     0,    23,    24,
      25,    26,     0,    27,    28,     0,    29,     2,     3,     4,
       0,     0,     0,     8,     9,    10,     0,     0,     0,     0,
       0,     0,     0,     0,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,     0,     0,     0,     0,
       0,    25,    26,     0,    27,    28,   229,    29,     0,     0,
       0,   230,     1,     2,     3,     4,     5,     6,     7,     8,
       9,    10,     0,     0,     0,     0,     0,     0,     0,     0,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,     0,     0,     0,    23,    24,    25,    26,     0,
      27,    28,     0,    29,     2,     3,     4,     0,     0,     0,
       8,     9,    10,     0,     0,     0,     0,     0,     0,     0,
       0,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,     0,     8,     9,    10,     0,    25,    26,
       0,    27,    28,     0,    29,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,     0,     0,     0,
       0,     0,    25,    26,     0,    27,    28,     0,    29
};

static const yytype_int16 yycheck[] =
{
       0,    86,    94,    43,     0,   162,     0,    34,    74,    72,
     253,    40,    85,   153,    58,    80,    79,    53,    54,    71,
      49,   106,    68,   266,     3,    68,    37,    77,    55,     8,
       9,   283,    43,    43,    74,     3,   288,    71,    38,    85,
       8,     9,    85,    77,    44,   137,    92,    93,    44,    93,
      44,    51,    88,    89,    33,    34,    35,    36,    37,   125,
      72,    61,   202,   109,    71,    33,    34,    35,    36,    37,
      77,    74,    72,    82,    74,    84,    76,   162,    51,    52,
     172,    70,    82,    72,    74,    60,    61,    62,   161,    71,
      79,   176,    71,    77,    69,    77,    80,    85,    86,    72,
      71,   258,   194,    76,    79,    37,    77,   185,   186,   187,
     188,    80,   204,    77,   254,   161,    80,   274,   161,    77,
      77,    43,    80,    80,   197,   125,    43,   267,     4,     5,
       6,   216,   217,   290,    43,   181,   182,   183,   184,   185,
     186,   187,   188,   189,   190,   191,   192,   193,   221,   289,
      70,   197,    71,   153,   197,    33,    34,    35,    55,    56,
      70,   253,    77,    78,   183,   184,   189,   190,    57,    59,
      73,    72,    80,   258,   266,   221,   261,    70,   221,    70,
      80,    80,    70,    75,   276,    70,    72,    71,    40,   274,
      73,    73,    79,    73,    73,   287,    77,    70,    73,    70,
      43,    75,   202,    71,    71,   290,     4,     5,     6,    79,
     210,    80,    10,    11,    12,    16,    79,   173,    80,   191,
      55,    82,   192,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    62,    82,     5,   193,    72,
      38,    39,   250,    41,    42,   198,    44,   288,    53,   210,
     250,   250,   266,   253,   254,    44,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   266,   267,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    75,    -1,    -1,
      -1,    -1,    -1,   283,    -1,    -1,    -1,    -1,   288,   289,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    -1,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      -1,    -1,    -1,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    -1,    -1,    51,    52,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    70,    -1,    -1,
      -1,    74,    75,    -1,    -1,    -1,    -1,    80,    81,    82,
      -1,    84,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    -1,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    -1,    -1,    -1,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    -1,    -1,
      51,    52,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    70,
      -1,    -1,    -1,    74,    75,    -1,    -1,    -1,    -1,    80,
      81,    82,    -1,    84,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    -1,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    -1,    -1,    -1,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      -1,    -1,    51,    52,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    70,    -1,    -1,    -1,    74,    -1,    -1,    -1,    -1,
      -1,    80,    81,    82,    -1,    84,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    -1,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    -1,    -1,    -1,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    -1,    -1,    51,    52,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    70,    -1,    -1,    -1,    74,    -1,    -1,
      -1,    -1,    -1,    80,    81,    82,    -1,    84,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    -1,    -1,
      -1,    36,    37,    38,    39,    -1,    41,    42,    43,    44,
      45,    46,    47,    48,    -1,    -1,    51,    52,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    70,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    80,    81,    82,    -1,    84,
       3,     4,     5,     6,    -1,     8,     9,    10,    11,    12,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      -1,    -1,    -1,    36,    37,    38,    39,    -1,    41,    42,
      43,    44,    45,    46,    47,    48,    -1,    -1,    51,    52,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    70,     4,     5,
       6,    -1,    -1,    -1,    10,    11,    12,    -1,    81,    82,
      -1,    84,    -1,    -1,    -1,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    -1,    -1,    -1,
      -1,    -1,    38,    39,    -1,    41,    42,    43,    44,    45,
      46,    47,    48,    -1,    -1,    51,    52,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    70,    -1,    -1,    73,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    81,    82,    -1,    84,     4,
       5,     6,    -1,    -1,    -1,    10,    11,    12,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    -1,    -1,
      -1,    -1,    -1,    38,    39,    -1,    41,    42,    43,    44,
      45,    46,    47,    48,    -1,    -1,    51,    52,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    70,     4,     5,     6,    -1,
      -1,    -1,    10,    11,    12,    80,    81,    82,    -1,    84,
      -1,    -1,    -1,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    -1,    -1,    -1,    -1,    -1,
      38,    39,    -1,    41,    42,    43,    44,    45,    46,    47,
      48,    -1,    -1,    51,    52,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    70,    -1,    -1,    73,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    81,    82,    -1,    84,     4,     5,     6,
      -1,    -1,    -1,    10,    11,    12,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    -1,    -1,    -1,    -1,
      -1,    38,    39,    -1,    41,    42,    43,    44,    45,    46,
      47,    48,    -1,    -1,    51,    52,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    70,     4,     5,     6,    -1,    -1,    -1,
      10,    11,    12,    -1,    81,    82,    -1,    84,    -1,    -1,
      -1,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    -1,    -1,    -1,    -1,    -1,    38,    39,
      -1,    41,    42,    43,    44,    45,    46,    47,    48,    -1,
      -1,    51,    52,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      70,     4,     5,     6,    -1,    -1,    -1,    10,    11,    12,
      -1,    81,    82,    -1,    84,    -1,    -1,    -1,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      -1,    -1,    -1,    -1,    -1,    38,    39,    -1,    41,    42,
      -1,    44,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,     0,    -1,    -1,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    75,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    -1,    -1,    -1,    36,    37,
      38,    39,    -1,    41,    42,    -1,    44,     4,     5,     6,
      -1,    -1,    -1,    10,    11,    12,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    -1,    -1,    -1,    -1,
      -1,    38,    39,    -1,    41,    42,    43,    44,    -1,    -1,
      -1,    48,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    -1,    -1,    -1,    36,    37,    38,    39,    -1,
      41,    42,    -1,    44,     4,     5,     6,    -1,    -1,    -1,
      10,    11,    12,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    -1,    10,    11,    12,    -1,    38,    39,
      -1,    41,    42,    -1,    44,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    -1,    -1,    -1,
      -1,    -1,    38,    39,    -1,    41,    42,    -1,    44
};



static const yytype_uint8 yystos[] =
{
       0,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    36,    37,    38,    39,    41,    42,    44,
     124,   125,   126,   127,   128,   133,   134,   135,   136,   137,
     138,   139,   140,   141,   168,   169,   170,    37,    43,   138,
      43,    74,    80,   171,    71,    77,     3,    33,    34,    35,
     130,   131,   136,    77,    80,    43,   137,   139,    72,     0,
     169,   139,    74,   137,   142,   143,    74,   154,   130,   129,
     132,   137,   131,    43,    70,    72,    79,    43,    45,    46,
      47,    48,    51,    52,    70,    81,    82,    84,    95,    96,
      97,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,   123,   137,    80,   142,    43,   144,   145,    75,
     143,    13,    14,    15,    17,    18,    19,    20,    40,    74,
      75,    80,   106,   119,   120,   122,   124,   125,   137,   147,
     148,   149,   150,   155,   156,   157,   160,   167,    43,   129,
     132,    72,    79,    73,   123,   120,   146,   106,   106,   122,
      51,    52,    72,    76,    71,    71,    77,    39,   120,    70,
     106,    85,    86,    82,    84,    53,    54,    88,    89,    55,
      56,    57,    59,    58,    93,    73,    75,    72,    77,    80,
      80,    80,   162,    70,    70,    80,    80,   122,    70,    75,
     151,    60,    61,    62,    69,    79,   121,    77,    80,    75,
     148,    72,    73,   123,   146,    73,    71,    98,   122,    43,
      48,   101,   120,   106,   106,   108,   108,   110,   110,   110,
     110,   111,   111,   115,   116,   117,   122,   123,   145,   148,
     163,   122,    80,   161,   155,   120,   120,   123,    79,    73,
      73,    78,    73,    40,   147,   156,   164,    71,   122,   135,
     159,   152,    73,   146,    79,   120,    70,   159,   165,   166,
     148,   158,    43,    71,    75,   146,   122,    80,    71,    16,
      79,   149,   153,   154,    71,   122,   153,   148,   146,    80
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab






#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256






#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif






#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif




#ifdef YYLEX_PARAM
# define YYLEX yylex (&yylval, YYLEX_PARAM)
#else
# define YYLEX yylex (&yylval)
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
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))







#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}






#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
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
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
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
		       		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
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








static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      

      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      

      int yyxbegin = yyn < 0 ? -yyn : 0;

      
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  


	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif 







#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}


#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else 
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif 









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
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{

int yychar;


YYSTYPE yylval;

    
    int yynerrs;

    int yystate;
    
    int yyerrstatus;

    






    
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  
  int yytoken;
  

  YYSTYPE yyval;

#if YYERROR_VERBOSE
  
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  

  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; 

  



  yyssp = yyss;
  yyvsp = yyvs;

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

	



	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

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
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif 

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

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
  if (yyn == YYPACT_NINF)
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
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  

  if (yyerrstatus)
    yyerrstatus--;

  
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

  goto yynewstate;





yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;





yyreduce:
  
  yylen = yyr2[yyn];

  







  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:

    {
        
        const TSymbol* symbol = (yyvsp[(1) - (1)].lex).symbol;
        const TVariable* variable;
        if (symbol == 0) {
            parseContext->error((yyvsp[(1) - (1)].lex).line, "undeclared identifier", (yyvsp[(1) - (1)].lex).string->c_str(), "");
            parseContext->recover();
            TType type(EbtFloat, EbpUndefined);
            TVariable* fakeVariable = new TVariable((yyvsp[(1) - (1)].lex).string, type);
            parseContext->symbolTable.insert(*fakeVariable);
            variable = fakeVariable;
        } else {
            
            if (! symbol->isVariable()) {
                parseContext->error((yyvsp[(1) - (1)].lex).line, "variable expected", (yyvsp[(1) - (1)].lex).string->c_str(), "");
                parseContext->recover();
            }
            variable = static_cast<const TVariable*>(symbol);
        }

        
        

        if (variable->getType().getQualifier() == EvqConst ) {
            ConstantUnion* constArray = variable->getConstPointer();
            TType t(variable->getType());
            (yyval.interm.intermTypedNode) = parseContext->intermediate.addConstantUnion(constArray, t, (yyvsp[(1) - (1)].lex).line);
        } else
            (yyval.interm.intermTypedNode) = parseContext->intermediate.addSymbol(variable->getUniqueId(),
                                                     variable->getName(),
                                                     variable->getType(), (yyvsp[(1) - (1)].lex).line);
    }
    break;

  case 3:

    {
        (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode);
    }
    break;

  case 4:

    {
        
        
        
        
        if (abs((yyvsp[(1) - (1)].lex).i) >= (1 << 16)) {
            parseContext->error((yyvsp[(1) - (1)].lex).line, " integer constant overflow", "", "");
            parseContext->recover();
        }
        ConstantUnion *unionArray = new ConstantUnion[1];
        unionArray->setIConst((yyvsp[(1) - (1)].lex).i);
        (yyval.interm.intermTypedNode) = parseContext->intermediate.addConstantUnion(unionArray, TType(EbtInt, EbpUndefined, EvqConst), (yyvsp[(1) - (1)].lex).line);
    }
    break;

  case 5:

    {
        ConstantUnion *unionArray = new ConstantUnion[1];
        unionArray->setFConst((yyvsp[(1) - (1)].lex).f);
        (yyval.interm.intermTypedNode) = parseContext->intermediate.addConstantUnion(unionArray, TType(EbtFloat, EbpUndefined, EvqConst), (yyvsp[(1) - (1)].lex).line);
    }
    break;

  case 6:

    {
        ConstantUnion *unionArray = new ConstantUnion[1];
        unionArray->setBConst((yyvsp[(1) - (1)].lex).b);
        (yyval.interm.intermTypedNode) = parseContext->intermediate.addConstantUnion(unionArray, TType(EbtBool, EbpUndefined, EvqConst), (yyvsp[(1) - (1)].lex).line);
    }
    break;

  case 7:

    {
        (yyval.interm.intermTypedNode) = (yyvsp[(2) - (3)].interm.intermTypedNode);
    }
    break;

  case 8:

    {
        (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode);
    }
    break;

  case 9:

    {
        if (!(yyvsp[(1) - (4)].interm.intermTypedNode)->isArray() && !(yyvsp[(1) - (4)].interm.intermTypedNode)->isMatrix() && !(yyvsp[(1) - (4)].interm.intermTypedNode)->isVector()) {
            if ((yyvsp[(1) - (4)].interm.intermTypedNode)->getAsSymbolNode())
                parseContext->error((yyvsp[(2) - (4)].lex).line, " left of '[' is not of type array, matrix, or vector ", (yyvsp[(1) - (4)].interm.intermTypedNode)->getAsSymbolNode()->getSymbol().c_str(), "");
            else
                parseContext->error((yyvsp[(2) - (4)].lex).line, " left of '[' is not of type array, matrix, or vector ", "expression", "");
            parseContext->recover();
        }
        if ((yyvsp[(1) - (4)].interm.intermTypedNode)->getType().getQualifier() == EvqConst && (yyvsp[(3) - (4)].interm.intermTypedNode)->getQualifier() == EvqConst) {
            if ((yyvsp[(1) - (4)].interm.intermTypedNode)->isArray()) { 
                (yyval.interm.intermTypedNode) = parseContext->addConstArrayNode((yyvsp[(3) - (4)].interm.intermTypedNode)->getAsConstantUnion()->getUnionArrayPointer()->getIConst(), (yyvsp[(1) - (4)].interm.intermTypedNode), (yyvsp[(2) - (4)].lex).line);
            } else if ((yyvsp[(1) - (4)].interm.intermTypedNode)->isVector()) {  
                TVectorFields fields;
                fields.num = 1;
                fields.offsets[0] = (yyvsp[(3) - (4)].interm.intermTypedNode)->getAsConstantUnion()->getUnionArrayPointer()->getIConst(); 
                (yyval.interm.intermTypedNode) = parseContext->addConstVectorNode(fields, (yyvsp[(1) - (4)].interm.intermTypedNode), (yyvsp[(2) - (4)].lex).line);
            } else if ((yyvsp[(1) - (4)].interm.intermTypedNode)->isMatrix()) { 
                (yyval.interm.intermTypedNode) = parseContext->addConstMatrixNode((yyvsp[(3) - (4)].interm.intermTypedNode)->getAsConstantUnion()->getUnionArrayPointer()->getIConst(), (yyvsp[(1) - (4)].interm.intermTypedNode), (yyvsp[(2) - (4)].lex).line);
            }
        } else {
            if ((yyvsp[(3) - (4)].interm.intermTypedNode)->getQualifier() == EvqConst) {
                if (((yyvsp[(1) - (4)].interm.intermTypedNode)->isVector() || (yyvsp[(1) - (4)].interm.intermTypedNode)->isMatrix()) && (yyvsp[(1) - (4)].interm.intermTypedNode)->getType().getNominalSize() <= (yyvsp[(3) - (4)].interm.intermTypedNode)->getAsConstantUnion()->getUnionArrayPointer()->getIConst() && !(yyvsp[(1) - (4)].interm.intermTypedNode)->isArray() ) {
                    parseContext->error((yyvsp[(2) - (4)].lex).line, "", "[", "field selection out of range '%d'", (yyvsp[(3) - (4)].interm.intermTypedNode)->getAsConstantUnion()->getUnionArrayPointer()->getIConst());
                    parseContext->recover();
                } else {
                    if ((yyvsp[(1) - (4)].interm.intermTypedNode)->isArray()) {
                        if ((yyvsp[(1) - (4)].interm.intermTypedNode)->getType().getArraySize() == 0) {
                            if ((yyvsp[(1) - (4)].interm.intermTypedNode)->getType().getMaxArraySize() <= (yyvsp[(3) - (4)].interm.intermTypedNode)->getAsConstantUnion()->getUnionArrayPointer()->getIConst()) {
                                if (parseContext->arraySetMaxSize((yyvsp[(1) - (4)].interm.intermTypedNode)->getAsSymbolNode(), (yyvsp[(1) - (4)].interm.intermTypedNode)->getTypePointer(), (yyvsp[(3) - (4)].interm.intermTypedNode)->getAsConstantUnion()->getUnionArrayPointer()->getIConst(), true, (yyvsp[(2) - (4)].lex).line))
                                    parseContext->recover();
                            } else {
                                if (parseContext->arraySetMaxSize((yyvsp[(1) - (4)].interm.intermTypedNode)->getAsSymbolNode(), (yyvsp[(1) - (4)].interm.intermTypedNode)->getTypePointer(), 0, false, (yyvsp[(2) - (4)].lex).line))
                                    parseContext->recover();
                            }
                        } else if ( (yyvsp[(3) - (4)].interm.intermTypedNode)->getAsConstantUnion()->getUnionArrayPointer()->getIConst() >= (yyvsp[(1) - (4)].interm.intermTypedNode)->getType().getArraySize()) {
                            parseContext->error((yyvsp[(2) - (4)].lex).line, "", "[", "array index out of range '%d'", (yyvsp[(3) - (4)].interm.intermTypedNode)->getAsConstantUnion()->getUnionArrayPointer()->getIConst());
                            parseContext->recover();
                        }
                    }
                    (yyval.interm.intermTypedNode) = parseContext->intermediate.addIndex(EOpIndexDirect, (yyvsp[(1) - (4)].interm.intermTypedNode), (yyvsp[(3) - (4)].interm.intermTypedNode), (yyvsp[(2) - (4)].lex).line);
                }
            } else {
                if ((yyvsp[(1) - (4)].interm.intermTypedNode)->isArray() && (yyvsp[(1) - (4)].interm.intermTypedNode)->getType().getArraySize() == 0) {
                    parseContext->error((yyvsp[(2) - (4)].lex).line, "", "[", "array must be redeclared with a size before being indexed with a variable");
                    parseContext->recover();
                }

                (yyval.interm.intermTypedNode) = parseContext->intermediate.addIndex(EOpIndexIndirect, (yyvsp[(1) - (4)].interm.intermTypedNode), (yyvsp[(3) - (4)].interm.intermTypedNode), (yyvsp[(2) - (4)].lex).line);
            }
        }
        if ((yyval.interm.intermTypedNode) == 0) {
            ConstantUnion *unionArray = new ConstantUnion[1];
            unionArray->setFConst(0.0f);
            (yyval.interm.intermTypedNode) = parseContext->intermediate.addConstantUnion(unionArray, TType(EbtFloat, EbpHigh, EvqConst), (yyvsp[(2) - (4)].lex).line);
        } else if ((yyvsp[(1) - (4)].interm.intermTypedNode)->isArray()) {
            if ((yyvsp[(1) - (4)].interm.intermTypedNode)->getType().getStruct())
                (yyval.interm.intermTypedNode)->setType(TType((yyvsp[(1) - (4)].interm.intermTypedNode)->getType().getStruct(), (yyvsp[(1) - (4)].interm.intermTypedNode)->getType().getTypeName()));
            else
                (yyval.interm.intermTypedNode)->setType(TType((yyvsp[(1) - (4)].interm.intermTypedNode)->getBasicType(), (yyvsp[(1) - (4)].interm.intermTypedNode)->getPrecision(), EvqTemporary, (yyvsp[(1) - (4)].interm.intermTypedNode)->getNominalSize(), (yyvsp[(1) - (4)].interm.intermTypedNode)->isMatrix()));

            if ((yyvsp[(1) - (4)].interm.intermTypedNode)->getType().getQualifier() == EvqConst)
                (yyval.interm.intermTypedNode)->getTypePointer()->changeQualifier(EvqConst);
        } else if ((yyvsp[(1) - (4)].interm.intermTypedNode)->isMatrix() && (yyvsp[(1) - (4)].interm.intermTypedNode)->getType().getQualifier() == EvqConst)
            (yyval.interm.intermTypedNode)->setType(TType((yyvsp[(1) - (4)].interm.intermTypedNode)->getBasicType(), (yyvsp[(1) - (4)].interm.intermTypedNode)->getPrecision(), EvqConst, (yyvsp[(1) - (4)].interm.intermTypedNode)->getNominalSize()));
        else if ((yyvsp[(1) - (4)].interm.intermTypedNode)->isMatrix())
            (yyval.interm.intermTypedNode)->setType(TType((yyvsp[(1) - (4)].interm.intermTypedNode)->getBasicType(), (yyvsp[(1) - (4)].interm.intermTypedNode)->getPrecision(), EvqTemporary, (yyvsp[(1) - (4)].interm.intermTypedNode)->getNominalSize()));
        else if ((yyvsp[(1) - (4)].interm.intermTypedNode)->isVector() && (yyvsp[(1) - (4)].interm.intermTypedNode)->getType().getQualifier() == EvqConst)
            (yyval.interm.intermTypedNode)->setType(TType((yyvsp[(1) - (4)].interm.intermTypedNode)->getBasicType(), (yyvsp[(1) - (4)].interm.intermTypedNode)->getPrecision(), EvqConst));
        else if ((yyvsp[(1) - (4)].interm.intermTypedNode)->isVector())
            (yyval.interm.intermTypedNode)->setType(TType((yyvsp[(1) - (4)].interm.intermTypedNode)->getBasicType(), (yyvsp[(1) - (4)].interm.intermTypedNode)->getPrecision(), EvqTemporary));
        else
            (yyval.interm.intermTypedNode)->setType((yyvsp[(1) - (4)].interm.intermTypedNode)->getType());
    }
    break;

  case 10:

    {
        (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode);
    }
    break;

  case 11:

    {
        if ((yyvsp[(1) - (3)].interm.intermTypedNode)->isArray()) {
            parseContext->error((yyvsp[(3) - (3)].lex).line, "cannot apply dot operator to an array", ".", "");
            parseContext->recover();
        }

        if ((yyvsp[(1) - (3)].interm.intermTypedNode)->isVector()) {
            TVectorFields fields;
            if (! parseContext->parseVectorFields(*(yyvsp[(3) - (3)].lex).string, (yyvsp[(1) - (3)].interm.intermTypedNode)->getNominalSize(), fields, (yyvsp[(3) - (3)].lex).line)) {
                fields.num = 1;
                fields.offsets[0] = 0;
                parseContext->recover();
            }

            if ((yyvsp[(1) - (3)].interm.intermTypedNode)->getType().getQualifier() == EvqConst) { 
                (yyval.interm.intermTypedNode) = parseContext->addConstVectorNode(fields, (yyvsp[(1) - (3)].interm.intermTypedNode), (yyvsp[(3) - (3)].lex).line);
                if ((yyval.interm.intermTypedNode) == 0) {
                    parseContext->recover();
                    (yyval.interm.intermTypedNode) = (yyvsp[(1) - (3)].interm.intermTypedNode);
                }
                else
                    (yyval.interm.intermTypedNode)->setType(TType((yyvsp[(1) - (3)].interm.intermTypedNode)->getBasicType(), (yyvsp[(1) - (3)].interm.intermTypedNode)->getPrecision(), EvqConst, (int) (*(yyvsp[(3) - (3)].lex).string).size()));
            } else {
                if (fields.num == 1) {
                    ConstantUnion *unionArray = new ConstantUnion[1];
                    unionArray->setIConst(fields.offsets[0]);
                    TIntermTyped* index = parseContext->intermediate.addConstantUnion(unionArray, TType(EbtInt, EbpUndefined, EvqConst), (yyvsp[(3) - (3)].lex).line);
                    (yyval.interm.intermTypedNode) = parseContext->intermediate.addIndex(EOpIndexDirect, (yyvsp[(1) - (3)].interm.intermTypedNode), index, (yyvsp[(2) - (3)].lex).line);
                    (yyval.interm.intermTypedNode)->setType(TType((yyvsp[(1) - (3)].interm.intermTypedNode)->getBasicType(), (yyvsp[(1) - (3)].interm.intermTypedNode)->getPrecision()));
                } else {
                    TString vectorString = *(yyvsp[(3) - (3)].lex).string;
                    TIntermTyped* index = parseContext->intermediate.addSwizzle(fields, (yyvsp[(3) - (3)].lex).line);
                    (yyval.interm.intermTypedNode) = parseContext->intermediate.addIndex(EOpVectorSwizzle, (yyvsp[(1) - (3)].interm.intermTypedNode), index, (yyvsp[(2) - (3)].lex).line);
                    (yyval.interm.intermTypedNode)->setType(TType((yyvsp[(1) - (3)].interm.intermTypedNode)->getBasicType(), (yyvsp[(1) - (3)].interm.intermTypedNode)->getPrecision(), EvqTemporary, (int) vectorString.size()));
                }
            }
        } else if ((yyvsp[(1) - (3)].interm.intermTypedNode)->isMatrix()) {
            TMatrixFields fields;
            if (! parseContext->parseMatrixFields(*(yyvsp[(3) - (3)].lex).string, (yyvsp[(1) - (3)].interm.intermTypedNode)->getNominalSize(), fields, (yyvsp[(3) - (3)].lex).line)) {
                fields.wholeRow = false;
                fields.wholeCol = false;
                fields.row = 0;
                fields.col = 0;
                parseContext->recover();
            }

            if (fields.wholeRow || fields.wholeCol) {
                parseContext->error((yyvsp[(2) - (3)].lex).line, " non-scalar fields not implemented yet", ".", "");
                parseContext->recover();
                ConstantUnion *unionArray = new ConstantUnion[1];
                unionArray->setIConst(0);
                TIntermTyped* index = parseContext->intermediate.addConstantUnion(unionArray, TType(EbtInt, EbpUndefined, EvqConst), (yyvsp[(3) - (3)].lex).line);
                (yyval.interm.intermTypedNode) = parseContext->intermediate.addIndex(EOpIndexDirect, (yyvsp[(1) - (3)].interm.intermTypedNode), index, (yyvsp[(2) - (3)].lex).line);
                (yyval.interm.intermTypedNode)->setType(TType((yyvsp[(1) - (3)].interm.intermTypedNode)->getBasicType(), (yyvsp[(1) - (3)].interm.intermTypedNode)->getPrecision(),EvqTemporary, (yyvsp[(1) - (3)].interm.intermTypedNode)->getNominalSize()));
            } else {
                ConstantUnion *unionArray = new ConstantUnion[1];
                unionArray->setIConst(fields.col * (yyvsp[(1) - (3)].interm.intermTypedNode)->getNominalSize() + fields.row);
                TIntermTyped* index = parseContext->intermediate.addConstantUnion(unionArray, TType(EbtInt, EbpUndefined, EvqConst), (yyvsp[(3) - (3)].lex).line);
                (yyval.interm.intermTypedNode) = parseContext->intermediate.addIndex(EOpIndexDirect, (yyvsp[(1) - (3)].interm.intermTypedNode), index, (yyvsp[(2) - (3)].lex).line);
                (yyval.interm.intermTypedNode)->setType(TType((yyvsp[(1) - (3)].interm.intermTypedNode)->getBasicType(), (yyvsp[(1) - (3)].interm.intermTypedNode)->getPrecision()));
            }
        } else if ((yyvsp[(1) - (3)].interm.intermTypedNode)->getBasicType() == EbtStruct) {
            bool fieldFound = false;
            TTypeList* fields = (yyvsp[(1) - (3)].interm.intermTypedNode)->getType().getStruct();
            if (fields == 0) {
                parseContext->error((yyvsp[(2) - (3)].lex).line, "structure has no fields", "Internal Error", "");
                parseContext->recover();
                (yyval.interm.intermTypedNode) = (yyvsp[(1) - (3)].interm.intermTypedNode);
            } else {
                unsigned int i;
                for (i = 0; i < fields->size(); ++i) {
                    if ((*fields)[i].type->getFieldName() == *(yyvsp[(3) - (3)].lex).string) {
                        fieldFound = true;
                        break;
                    }
                }
                if (fieldFound) {
                    if ((yyvsp[(1) - (3)].interm.intermTypedNode)->getType().getQualifier() == EvqConst) {
                        (yyval.interm.intermTypedNode) = parseContext->addConstStruct(*(yyvsp[(3) - (3)].lex).string, (yyvsp[(1) - (3)].interm.intermTypedNode), (yyvsp[(2) - (3)].lex).line);
                        if ((yyval.interm.intermTypedNode) == 0) {
                            parseContext->recover();
                            (yyval.interm.intermTypedNode) = (yyvsp[(1) - (3)].interm.intermTypedNode);
                        }
                        else {
                            (yyval.interm.intermTypedNode)->setType(*(*fields)[i].type);
                            
                            
                            (yyval.interm.intermTypedNode)->getTypePointer()->changeQualifier(EvqConst);
                        }
                    } else {
                        ConstantUnion *unionArray = new ConstantUnion[1];
                        unionArray->setIConst(i);
                        TIntermTyped* index = parseContext->intermediate.addConstantUnion(unionArray, *(*fields)[i].type, (yyvsp[(3) - (3)].lex).line);
                        (yyval.interm.intermTypedNode) = parseContext->intermediate.addIndex(EOpIndexDirectStruct, (yyvsp[(1) - (3)].interm.intermTypedNode), index, (yyvsp[(2) - (3)].lex).line);
                        (yyval.interm.intermTypedNode)->setType(*(*fields)[i].type);
                    }
                } else {
                    parseContext->error((yyvsp[(2) - (3)].lex).line, " no such field in structure", (yyvsp[(3) - (3)].lex).string->c_str(), "");
                    parseContext->recover();
                    (yyval.interm.intermTypedNode) = (yyvsp[(1) - (3)].interm.intermTypedNode);
                }
            }
        } else {
            parseContext->error((yyvsp[(2) - (3)].lex).line, " field selection requires structure, vector, or matrix on left hand side", (yyvsp[(3) - (3)].lex).string->c_str(), "");
            parseContext->recover();
            (yyval.interm.intermTypedNode) = (yyvsp[(1) - (3)].interm.intermTypedNode);
        }
        
    }
    break;

  case 12:

    {
        if (parseContext->lValueErrorCheck((yyvsp[(2) - (2)].lex).line, "++", (yyvsp[(1) - (2)].interm.intermTypedNode)))
            parseContext->recover();
        (yyval.interm.intermTypedNode) = parseContext->intermediate.addUnaryMath(EOpPostIncrement, (yyvsp[(1) - (2)].interm.intermTypedNode), (yyvsp[(2) - (2)].lex).line, parseContext->symbolTable);
        if ((yyval.interm.intermTypedNode) == 0) {
            parseContext->unaryOpError((yyvsp[(2) - (2)].lex).line, "++", (yyvsp[(1) - (2)].interm.intermTypedNode)->getCompleteString());
            parseContext->recover();
            (yyval.interm.intermTypedNode) = (yyvsp[(1) - (2)].interm.intermTypedNode);
        }
    }
    break;

  case 13:

    {
        if (parseContext->lValueErrorCheck((yyvsp[(2) - (2)].lex).line, "--", (yyvsp[(1) - (2)].interm.intermTypedNode)))
            parseContext->recover();
        (yyval.interm.intermTypedNode) = parseContext->intermediate.addUnaryMath(EOpPostDecrement, (yyvsp[(1) - (2)].interm.intermTypedNode), (yyvsp[(2) - (2)].lex).line, parseContext->symbolTable);
        if ((yyval.interm.intermTypedNode) == 0) {
            parseContext->unaryOpError((yyvsp[(2) - (2)].lex).line, "--", (yyvsp[(1) - (2)].interm.intermTypedNode)->getCompleteString());
            parseContext->recover();
            (yyval.interm.intermTypedNode) = (yyvsp[(1) - (2)].interm.intermTypedNode);
        }
    }
    break;

  case 14:

    {
        if (parseContext->integerErrorCheck((yyvsp[(1) - (1)].interm.intermTypedNode), "[]"))
            parseContext->recover();
        (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode);
    }
    break;

  case 15:

    {
        TFunction* fnCall = (yyvsp[(1) - (1)].interm).function;
        TOperator op = fnCall->getBuiltInOp();

        if (op != EOpNull)
        {
            
            
            
            
            
            TType type(EbtVoid, EbpUndefined);  
            if (parseContext->constructorErrorCheck((yyvsp[(1) - (1)].interm).line, (yyvsp[(1) - (1)].interm).intermNode, *fnCall, op, &type)) {
                (yyval.interm.intermTypedNode) = 0;
            } else {
                
                
                
                (yyval.interm.intermTypedNode) = parseContext->addConstructor((yyvsp[(1) - (1)].interm).intermNode, &type, op, fnCall, (yyvsp[(1) - (1)].interm).line);
            }

            if ((yyval.interm.intermTypedNode) == 0) {
                parseContext->recover();
                (yyval.interm.intermTypedNode) = parseContext->intermediate.setAggregateOperator(0, op, (yyvsp[(1) - (1)].interm).line);
            }
            (yyval.interm.intermTypedNode)->setType(type);
        } else {
            
            
            
            const TFunction* fnCandidate;
            bool builtIn;
            fnCandidate = parseContext->findFunction((yyvsp[(1) - (1)].interm).line, fnCall, &builtIn);
            if (fnCandidate) {
                
                
                
                
                op = fnCandidate->getBuiltInOp();
                if (builtIn && op != EOpNull) {
                    
                    
                    
                    if (fnCandidate->getParamCount() == 1) {
                        
                        
                        
                        (yyval.interm.intermTypedNode) = parseContext->intermediate.addUnaryMath(op, (yyvsp[(1) - (1)].interm).intermNode, 0, parseContext->symbolTable);
                        if ((yyval.interm.intermTypedNode) == 0)  {
                            parseContext->error((yyvsp[(1) - (1)].interm).intermNode->getLine(), " wrong operand type", "Internal Error",
                                "built in unary operator function.  Type: %s",
                                static_cast<TIntermTyped*>((yyvsp[(1) - (1)].interm).intermNode)->getCompleteString().c_str());
                            YYERROR;
                        }
                    } else {
                        (yyval.interm.intermTypedNode) = parseContext->intermediate.setAggregateOperator((yyvsp[(1) - (1)].interm).intermAggregate, op, (yyvsp[(1) - (1)].interm).line);
                    }
                } else {
                    

                    (yyval.interm.intermTypedNode) = parseContext->intermediate.setAggregateOperator((yyvsp[(1) - (1)].interm).intermAggregate, EOpFunctionCall, (yyvsp[(1) - (1)].interm).line);
                    (yyval.interm.intermTypedNode)->setType(fnCandidate->getReturnType());

                    
                    
                    
                    if (!builtIn)
                        (yyval.interm.intermTypedNode)->getAsAggregate()->setUserDefined();
                    (yyval.interm.intermTypedNode)->getAsAggregate()->setName(fnCandidate->getMangledName());

                    TQualifier qual;
                    TQualifierList& qualifierList = (yyval.interm.intermTypedNode)->getAsAggregate()->getQualifier();
                    for (int i = 0; i < fnCandidate->getParamCount(); ++i) {
                        qual = (*fnCandidate)[i].type->getQualifier();
                        if (qual == EvqOut || qual == EvqInOut) {
                            if (parseContext->lValueErrorCheck((yyval.interm.intermTypedNode)->getLine(), "assign", (yyval.interm.intermTypedNode)->getAsAggregate()->getSequence()[i]->getAsTyped())) {
                                parseContext->error((yyvsp[(1) - (1)].interm).intermNode->getLine(), "Constant value cannot be passed for 'out' or 'inout' parameters.", "Error", "");
                                parseContext->recover();
                            }
                        }
                        qualifierList.push_back(qual);
                    }
                }
                (yyval.interm.intermTypedNode)->setType(fnCandidate->getReturnType());
            } else {
                
                
                ConstantUnion *unionArray = new ConstantUnion[1];
                unionArray->setFConst(0.0f);
                (yyval.interm.intermTypedNode) = parseContext->intermediate.addConstantUnion(unionArray, TType(EbtFloat, EbpUndefined, EvqConst), (yyvsp[(1) - (1)].interm).line);
                parseContext->recover();
            }
        }
        delete fnCall;
    }
    break;

  case 16:

    {
        (yyval.interm) = (yyvsp[(1) - (1)].interm);
    }
    break;

  case 17:

    {
        parseContext->error((yyvsp[(3) - (3)].interm).line, "methods are not supported", "", "");
        parseContext->recover();
        (yyval.interm) = (yyvsp[(3) - (3)].interm);
    }
    break;

  case 18:

    {
        (yyval.interm) = (yyvsp[(1) - (2)].interm);
        (yyval.interm).line = (yyvsp[(2) - (2)].lex).line;
    }
    break;

  case 19:

    {
        (yyval.interm) = (yyvsp[(1) - (2)].interm);
        (yyval.interm).line = (yyvsp[(2) - (2)].lex).line;
    }
    break;

  case 20:

    {
        (yyval.interm).function = (yyvsp[(1) - (2)].interm.function);
        (yyval.interm).intermNode = 0;
    }
    break;

  case 21:

    {
        (yyval.interm).function = (yyvsp[(1) - (1)].interm.function);
        (yyval.interm).intermNode = 0;
    }
    break;

  case 22:

    {
        TParameter param = { 0, new TType((yyvsp[(2) - (2)].interm.intermTypedNode)->getType()) };
        (yyvsp[(1) - (2)].interm.function)->addParameter(param);
        (yyval.interm).function = (yyvsp[(1) - (2)].interm.function);
        (yyval.interm).intermNode = (yyvsp[(2) - (2)].interm.intermTypedNode);
    }
    break;

  case 23:

    {
        TParameter param = { 0, new TType((yyvsp[(3) - (3)].interm.intermTypedNode)->getType()) };
        (yyvsp[(1) - (3)].interm).function->addParameter(param);
        (yyval.interm).function = (yyvsp[(1) - (3)].interm).function;
        (yyval.interm).intermNode = parseContext->intermediate.growAggregate((yyvsp[(1) - (3)].interm).intermNode, (yyvsp[(3) - (3)].interm.intermTypedNode), (yyvsp[(2) - (3)].lex).line);
    }
    break;

  case 24:

    {
        (yyval.interm.function) = (yyvsp[(1) - (2)].interm.function);
    }
    break;

  case 25:

    {
        
        
        
        if ((yyvsp[(1) - (1)].interm.type).array) {
            if (parseContext->extensionErrorCheck((yyvsp[(1) - (1)].interm.type).line, "GL_3DL_array_objects")) {
                parseContext->recover();
                (yyvsp[(1) - (1)].interm.type).setArray(false);
            }
        }

        if ((yyvsp[(1) - (1)].interm.type).userDef) {
            TString tempString = "";
            TType type((yyvsp[(1) - (1)].interm.type));
            TFunction *function = new TFunction(&tempString, type, EOpConstructStruct);
            (yyval.interm.function) = function;
        } else {
            TOperator op = EOpNull;
            switch ((yyvsp[(1) - (1)].interm.type).type) {
            case EbtFloat:
                if ((yyvsp[(1) - (1)].interm.type).matrix) {
                    switch((yyvsp[(1) - (1)].interm.type).size) {
                    case 2:                                     op = EOpConstructMat2;  break;
                    case 3:                                     op = EOpConstructMat3;  break;
                    case 4:                                     op = EOpConstructMat4;  break;
                    }
                } else {
                    switch((yyvsp[(1) - (1)].interm.type).size) {
                    case 1:                                     op = EOpConstructFloat; break;
                    case 2:                                     op = EOpConstructVec2;  break;
                    case 3:                                     op = EOpConstructVec3;  break;
                    case 4:                                     op = EOpConstructVec4;  break;
                    }
                }
                break;
            case EbtInt:
                switch((yyvsp[(1) - (1)].interm.type).size) {
                case 1:                                         op = EOpConstructInt;   break;
                case 2:       FRAG_VERT_ONLY("ivec2", (yyvsp[(1) - (1)].interm.type).line); op = EOpConstructIVec2; break;
                case 3:       FRAG_VERT_ONLY("ivec3", (yyvsp[(1) - (1)].interm.type).line); op = EOpConstructIVec3; break;
                case 4:       FRAG_VERT_ONLY("ivec4", (yyvsp[(1) - (1)].interm.type).line); op = EOpConstructIVec4; break;
                }
                break;
            case EbtBool:
                switch((yyvsp[(1) - (1)].interm.type).size) {
                case 1:                                         op = EOpConstructBool;  break;
                case 2:       FRAG_VERT_ONLY("bvec2", (yyvsp[(1) - (1)].interm.type).line); op = EOpConstructBVec2; break;
                case 3:       FRAG_VERT_ONLY("bvec3", (yyvsp[(1) - (1)].interm.type).line); op = EOpConstructBVec3; break;
                case 4:       FRAG_VERT_ONLY("bvec4", (yyvsp[(1) - (1)].interm.type).line); op = EOpConstructBVec4; break;
                }
                break;
            }
            if (op == EOpNull) {
                parseContext->error((yyvsp[(1) - (1)].interm.type).line, "cannot construct this type", TType::getBasicString((yyvsp[(1) - (1)].interm.type).type), "");
                parseContext->recover();
                (yyvsp[(1) - (1)].interm.type).type = EbtFloat;
                op = EOpConstructFloat;
            }
            TString tempString = "";
            TType type((yyvsp[(1) - (1)].interm.type));
            TFunction *function = new TFunction(&tempString, type, op);
            (yyval.interm.function) = function;
        }
    }
    break;

  case 26:

    {
        if (parseContext->reservedErrorCheck((yyvsp[(1) - (1)].lex).line, *(yyvsp[(1) - (1)].lex).string))
            parseContext->recover();
        TType type(EbtVoid, EbpUndefined);
        TFunction *function = new TFunction((yyvsp[(1) - (1)].lex).string, type);
        (yyval.interm.function) = function;
    }
    break;

  case 27:

    {
        if (parseContext->reservedErrorCheck((yyvsp[(1) - (1)].lex).line, *(yyvsp[(1) - (1)].lex).string))
            parseContext->recover();
        TType type(EbtVoid, EbpUndefined);
        TFunction *function = new TFunction((yyvsp[(1) - (1)].lex).string, type);
        (yyval.interm.function) = function;
    }
    break;

  case 28:

    {
        (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode);
    }
    break;

  case 29:

    {
        if (parseContext->lValueErrorCheck((yyvsp[(1) - (2)].lex).line, "++", (yyvsp[(2) - (2)].interm.intermTypedNode)))
            parseContext->recover();
        (yyval.interm.intermTypedNode) = parseContext->intermediate.addUnaryMath(EOpPreIncrement, (yyvsp[(2) - (2)].interm.intermTypedNode), (yyvsp[(1) - (2)].lex).line, parseContext->symbolTable);
        if ((yyval.interm.intermTypedNode) == 0) {
            parseContext->unaryOpError((yyvsp[(1) - (2)].lex).line, "++", (yyvsp[(2) - (2)].interm.intermTypedNode)->getCompleteString());
            parseContext->recover();
            (yyval.interm.intermTypedNode) = (yyvsp[(2) - (2)].interm.intermTypedNode);
        }
    }
    break;

  case 30:

    {
        if (parseContext->lValueErrorCheck((yyvsp[(1) - (2)].lex).line, "--", (yyvsp[(2) - (2)].interm.intermTypedNode)))
            parseContext->recover();
        (yyval.interm.intermTypedNode) = parseContext->intermediate.addUnaryMath(EOpPreDecrement, (yyvsp[(2) - (2)].interm.intermTypedNode), (yyvsp[(1) - (2)].lex).line, parseContext->symbolTable);
        if ((yyval.interm.intermTypedNode) == 0) {
            parseContext->unaryOpError((yyvsp[(1) - (2)].lex).line, "--", (yyvsp[(2) - (2)].interm.intermTypedNode)->getCompleteString());
            parseContext->recover();
            (yyval.interm.intermTypedNode) = (yyvsp[(2) - (2)].interm.intermTypedNode);
        }
    }
    break;

  case 31:

    {
        if ((yyvsp[(1) - (2)].interm).op != EOpNull) {
            (yyval.interm.intermTypedNode) = parseContext->intermediate.addUnaryMath((yyvsp[(1) - (2)].interm).op, (yyvsp[(2) - (2)].interm.intermTypedNode), (yyvsp[(1) - (2)].interm).line, parseContext->symbolTable);
            if ((yyval.interm.intermTypedNode) == 0) {
                const char* errorOp = "";
                switch((yyvsp[(1) - (2)].interm).op) {
                case EOpNegative:   errorOp = "-"; break;
                case EOpLogicalNot: errorOp = "!"; break;
                default: break;
                }
                parseContext->unaryOpError((yyvsp[(1) - (2)].interm).line, errorOp, (yyvsp[(2) - (2)].interm.intermTypedNode)->getCompleteString());
                parseContext->recover();
                (yyval.interm.intermTypedNode) = (yyvsp[(2) - (2)].interm.intermTypedNode);
            }
        } else
            (yyval.interm.intermTypedNode) = (yyvsp[(2) - (2)].interm.intermTypedNode);
    }
    break;

  case 32:

    { (yyval.interm).line = (yyvsp[(1) - (1)].lex).line; (yyval.interm).op = EOpNull; }
    break;

  case 33:

    { (yyval.interm).line = (yyvsp[(1) - (1)].lex).line; (yyval.interm).op = EOpNegative; }
    break;

  case 34:

    { (yyval.interm).line = (yyvsp[(1) - (1)].lex).line; (yyval.interm).op = EOpLogicalNot; }
    break;

  case 35:

    { (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode); }
    break;

  case 36:

    {
        FRAG_VERT_ONLY("*", (yyvsp[(2) - (3)].lex).line);
        (yyval.interm.intermTypedNode) = parseContext->intermediate.addBinaryMath(EOpMul, (yyvsp[(1) - (3)].interm.intermTypedNode), (yyvsp[(3) - (3)].interm.intermTypedNode), (yyvsp[(2) - (3)].lex).line, parseContext->symbolTable);
        if ((yyval.interm.intermTypedNode) == 0) {
            parseContext->binaryOpError((yyvsp[(2) - (3)].lex).line, "*", (yyvsp[(1) - (3)].interm.intermTypedNode)->getCompleteString(), (yyvsp[(3) - (3)].interm.intermTypedNode)->getCompleteString());
            parseContext->recover();
            (yyval.interm.intermTypedNode) = (yyvsp[(1) - (3)].interm.intermTypedNode);
        }
    }
    break;

  case 37:

    {
        FRAG_VERT_ONLY("/", (yyvsp[(2) - (3)].lex).line);
        (yyval.interm.intermTypedNode) = parseContext->intermediate.addBinaryMath(EOpDiv, (yyvsp[(1) - (3)].interm.intermTypedNode), (yyvsp[(3) - (3)].interm.intermTypedNode), (yyvsp[(2) - (3)].lex).line, parseContext->symbolTable);
        if ((yyval.interm.intermTypedNode) == 0) {
            parseContext->binaryOpError((yyvsp[(2) - (3)].lex).line, "/", (yyvsp[(1) - (3)].interm.intermTypedNode)->getCompleteString(), (yyvsp[(3) - (3)].interm.intermTypedNode)->getCompleteString());
            parseContext->recover();
            (yyval.interm.intermTypedNode) = (yyvsp[(1) - (3)].interm.intermTypedNode);
        }
    }
    break;

  case 38:

    { (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode); }
    break;

  case 39:

    {
        (yyval.interm.intermTypedNode) = parseContext->intermediate.addBinaryMath(EOpAdd, (yyvsp[(1) - (3)].interm.intermTypedNode), (yyvsp[(3) - (3)].interm.intermTypedNode), (yyvsp[(2) - (3)].lex).line, parseContext->symbolTable);
        if ((yyval.interm.intermTypedNode) == 0) {
            parseContext->binaryOpError((yyvsp[(2) - (3)].lex).line, "+", (yyvsp[(1) - (3)].interm.intermTypedNode)->getCompleteString(), (yyvsp[(3) - (3)].interm.intermTypedNode)->getCompleteString());
            parseContext->recover();
            (yyval.interm.intermTypedNode) = (yyvsp[(1) - (3)].interm.intermTypedNode);
        }
    }
    break;

  case 40:

    {
        (yyval.interm.intermTypedNode) = parseContext->intermediate.addBinaryMath(EOpSub, (yyvsp[(1) - (3)].interm.intermTypedNode), (yyvsp[(3) - (3)].interm.intermTypedNode), (yyvsp[(2) - (3)].lex).line, parseContext->symbolTable);
        if ((yyval.interm.intermTypedNode) == 0) {
            parseContext->binaryOpError((yyvsp[(2) - (3)].lex).line, "-", (yyvsp[(1) - (3)].interm.intermTypedNode)->getCompleteString(), (yyvsp[(3) - (3)].interm.intermTypedNode)->getCompleteString());
            parseContext->recover();
            (yyval.interm.intermTypedNode) = (yyvsp[(1) - (3)].interm.intermTypedNode);
        }
    }
    break;

  case 41:

    { (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode); }
    break;

  case 42:

    { (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode); }
    break;

  case 43:

    {
        (yyval.interm.intermTypedNode) = parseContext->intermediate.addBinaryMath(EOpLessThan, (yyvsp[(1) - (3)].interm.intermTypedNode), (yyvsp[(3) - (3)].interm.intermTypedNode), (yyvsp[(2) - (3)].lex).line, parseContext->symbolTable);
        if ((yyval.interm.intermTypedNode) == 0) {
            parseContext->binaryOpError((yyvsp[(2) - (3)].lex).line, "<", (yyvsp[(1) - (3)].interm.intermTypedNode)->getCompleteString(), (yyvsp[(3) - (3)].interm.intermTypedNode)->getCompleteString());
            parseContext->recover();
            ConstantUnion *unionArray = new ConstantUnion[1];
            unionArray->setBConst(false);
            (yyval.interm.intermTypedNode) = parseContext->intermediate.addConstantUnion(unionArray, TType(EbtBool, EbpUndefined, EvqConst), (yyvsp[(2) - (3)].lex).line);
        }
    }
    break;

  case 44:

    {
        (yyval.interm.intermTypedNode) = parseContext->intermediate.addBinaryMath(EOpGreaterThan, (yyvsp[(1) - (3)].interm.intermTypedNode), (yyvsp[(3) - (3)].interm.intermTypedNode), (yyvsp[(2) - (3)].lex).line, parseContext->symbolTable);
        if ((yyval.interm.intermTypedNode) == 0) {
            parseContext->binaryOpError((yyvsp[(2) - (3)].lex).line, ">", (yyvsp[(1) - (3)].interm.intermTypedNode)->getCompleteString(), (yyvsp[(3) - (3)].interm.intermTypedNode)->getCompleteString());
            parseContext->recover();
            ConstantUnion *unionArray = new ConstantUnion[1];
            unionArray->setBConst(false);
            (yyval.interm.intermTypedNode) = parseContext->intermediate.addConstantUnion(unionArray, TType(EbtBool, EbpUndefined, EvqConst), (yyvsp[(2) - (3)].lex).line);
        }
    }
    break;

  case 45:

    {
        (yyval.interm.intermTypedNode) = parseContext->intermediate.addBinaryMath(EOpLessThanEqual, (yyvsp[(1) - (3)].interm.intermTypedNode), (yyvsp[(3) - (3)].interm.intermTypedNode), (yyvsp[(2) - (3)].lex).line, parseContext->symbolTable);
        if ((yyval.interm.intermTypedNode) == 0) {
            parseContext->binaryOpError((yyvsp[(2) - (3)].lex).line, "<=", (yyvsp[(1) - (3)].interm.intermTypedNode)->getCompleteString(), (yyvsp[(3) - (3)].interm.intermTypedNode)->getCompleteString());
            parseContext->recover();
            ConstantUnion *unionArray = new ConstantUnion[1];
            unionArray->setBConst(false);
            (yyval.interm.intermTypedNode) = parseContext->intermediate.addConstantUnion(unionArray, TType(EbtBool, EbpUndefined, EvqConst), (yyvsp[(2) - (3)].lex).line);
        }
    }
    break;

  case 46:

    {
        (yyval.interm.intermTypedNode) = parseContext->intermediate.addBinaryMath(EOpGreaterThanEqual, (yyvsp[(1) - (3)].interm.intermTypedNode), (yyvsp[(3) - (3)].interm.intermTypedNode), (yyvsp[(2) - (3)].lex).line, parseContext->symbolTable);
        if ((yyval.interm.intermTypedNode) == 0) {
            parseContext->binaryOpError((yyvsp[(2) - (3)].lex).line, ">=", (yyvsp[(1) - (3)].interm.intermTypedNode)->getCompleteString(), (yyvsp[(3) - (3)].interm.intermTypedNode)->getCompleteString());
            parseContext->recover();
            ConstantUnion *unionArray = new ConstantUnion[1];
            unionArray->setBConst(false);
            (yyval.interm.intermTypedNode) = parseContext->intermediate.addConstantUnion(unionArray, TType(EbtBool, EbpUndefined, EvqConst), (yyvsp[(2) - (3)].lex).line);
        }
    }
    break;

  case 47:

    { (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode); }
    break;

  case 48:

    {
        (yyval.interm.intermTypedNode) = parseContext->intermediate.addBinaryMath(EOpEqual, (yyvsp[(1) - (3)].interm.intermTypedNode), (yyvsp[(3) - (3)].interm.intermTypedNode), (yyvsp[(2) - (3)].lex).line, parseContext->symbolTable);
        if ((yyval.interm.intermTypedNode) == 0) {
            parseContext->binaryOpError((yyvsp[(2) - (3)].lex).line, "==", (yyvsp[(1) - (3)].interm.intermTypedNode)->getCompleteString(), (yyvsp[(3) - (3)].interm.intermTypedNode)->getCompleteString());
            parseContext->recover();
            ConstantUnion *unionArray = new ConstantUnion[1];
            unionArray->setBConst(false);
            (yyval.interm.intermTypedNode) = parseContext->intermediate.addConstantUnion(unionArray, TType(EbtBool, EbpUndefined, EvqConst), (yyvsp[(2) - (3)].lex).line);
        } else if (((yyvsp[(1) - (3)].interm.intermTypedNode)->isArray() || (yyvsp[(3) - (3)].interm.intermTypedNode)->isArray()) && parseContext->extensionErrorCheck((yyvsp[(2) - (3)].lex).line, "GL_3DL_array_objects"))
            parseContext->recover();
    }
    break;

  case 49:

    {
        (yyval.interm.intermTypedNode) = parseContext->intermediate.addBinaryMath(EOpNotEqual, (yyvsp[(1) - (3)].interm.intermTypedNode), (yyvsp[(3) - (3)].interm.intermTypedNode), (yyvsp[(2) - (3)].lex).line, parseContext->symbolTable);
        if ((yyval.interm.intermTypedNode) == 0) {
            parseContext->binaryOpError((yyvsp[(2) - (3)].lex).line, "!=", (yyvsp[(1) - (3)].interm.intermTypedNode)->getCompleteString(), (yyvsp[(3) - (3)].interm.intermTypedNode)->getCompleteString());
            parseContext->recover();
            ConstantUnion *unionArray = new ConstantUnion[1];
            unionArray->setBConst(false);
            (yyval.interm.intermTypedNode) = parseContext->intermediate.addConstantUnion(unionArray, TType(EbtBool, EbpUndefined, EvqConst), (yyvsp[(2) - (3)].lex).line);
        } else if (((yyvsp[(1) - (3)].interm.intermTypedNode)->isArray() || (yyvsp[(3) - (3)].interm.intermTypedNode)->isArray()) && parseContext->extensionErrorCheck((yyvsp[(2) - (3)].lex).line, "GL_3DL_array_objects"))
            parseContext->recover();
    }
    break;

  case 50:

    { (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode); }
    break;

  case 51:

    { (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode); }
    break;

  case 52:

    { (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode); }
    break;

  case 53:

    { (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode); }
    break;

  case 54:

    {
        (yyval.interm.intermTypedNode) = parseContext->intermediate.addBinaryMath(EOpLogicalAnd, (yyvsp[(1) - (3)].interm.intermTypedNode), (yyvsp[(3) - (3)].interm.intermTypedNode), (yyvsp[(2) - (3)].lex).line, parseContext->symbolTable);
        if ((yyval.interm.intermTypedNode) == 0) {
            parseContext->binaryOpError((yyvsp[(2) - (3)].lex).line, "&&", (yyvsp[(1) - (3)].interm.intermTypedNode)->getCompleteString(), (yyvsp[(3) - (3)].interm.intermTypedNode)->getCompleteString());
            parseContext->recover();
            ConstantUnion *unionArray = new ConstantUnion[1];
            unionArray->setBConst(false);
            (yyval.interm.intermTypedNode) = parseContext->intermediate.addConstantUnion(unionArray, TType(EbtBool, EbpUndefined, EvqConst), (yyvsp[(2) - (3)].lex).line);
        }
    }
    break;

  case 55:

    { (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode); }
    break;

  case 56:

    {
        (yyval.interm.intermTypedNode) = parseContext->intermediate.addBinaryMath(EOpLogicalXor, (yyvsp[(1) - (3)].interm.intermTypedNode), (yyvsp[(3) - (3)].interm.intermTypedNode), (yyvsp[(2) - (3)].lex).line, parseContext->symbolTable);
        if ((yyval.interm.intermTypedNode) == 0) {
            parseContext->binaryOpError((yyvsp[(2) - (3)].lex).line, "^^", (yyvsp[(1) - (3)].interm.intermTypedNode)->getCompleteString(), (yyvsp[(3) - (3)].interm.intermTypedNode)->getCompleteString());
            parseContext->recover();
            ConstantUnion *unionArray = new ConstantUnion[1];
            unionArray->setBConst(false);
            (yyval.interm.intermTypedNode) = parseContext->intermediate.addConstantUnion(unionArray, TType(EbtBool, EbpUndefined, EvqConst), (yyvsp[(2) - (3)].lex).line);
        }
    }
    break;

  case 57:

    { (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode); }
    break;

  case 58:

    {
        (yyval.interm.intermTypedNode) = parseContext->intermediate.addBinaryMath(EOpLogicalOr, (yyvsp[(1) - (3)].interm.intermTypedNode), (yyvsp[(3) - (3)].interm.intermTypedNode), (yyvsp[(2) - (3)].lex).line, parseContext->symbolTable);
        if ((yyval.interm.intermTypedNode) == 0) {
            parseContext->binaryOpError((yyvsp[(2) - (3)].lex).line, "||", (yyvsp[(1) - (3)].interm.intermTypedNode)->getCompleteString(), (yyvsp[(3) - (3)].interm.intermTypedNode)->getCompleteString());
            parseContext->recover();
            ConstantUnion *unionArray = new ConstantUnion[1];
            unionArray->setBConst(false);
            (yyval.interm.intermTypedNode) = parseContext->intermediate.addConstantUnion(unionArray, TType(EbtBool, EbpUndefined, EvqConst), (yyvsp[(2) - (3)].lex).line);
        }
    }
    break;

  case 59:

    { (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode); }
    break;

  case 60:

    {
       if (parseContext->boolErrorCheck((yyvsp[(2) - (5)].lex).line, (yyvsp[(1) - (5)].interm.intermTypedNode)))
            parseContext->recover();

        (yyval.interm.intermTypedNode) = parseContext->intermediate.addSelection((yyvsp[(1) - (5)].interm.intermTypedNode), (yyvsp[(3) - (5)].interm.intermTypedNode), (yyvsp[(5) - (5)].interm.intermTypedNode), (yyvsp[(2) - (5)].lex).line);
        if ((yyvsp[(3) - (5)].interm.intermTypedNode)->getType() != (yyvsp[(5) - (5)].interm.intermTypedNode)->getType())
            (yyval.interm.intermTypedNode) = 0;

        if ((yyval.interm.intermTypedNode) == 0) {
            parseContext->binaryOpError((yyvsp[(2) - (5)].lex).line, ":", (yyvsp[(3) - (5)].interm.intermTypedNode)->getCompleteString(), (yyvsp[(5) - (5)].interm.intermTypedNode)->getCompleteString());
            parseContext->recover();
            (yyval.interm.intermTypedNode) = (yyvsp[(5) - (5)].interm.intermTypedNode);
        }
    }
    break;

  case 61:

    { (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode); }
    break;

  case 62:

    {
        if (parseContext->lValueErrorCheck((yyvsp[(2) - (3)].interm).line, "assign", (yyvsp[(1) - (3)].interm.intermTypedNode)))
            parseContext->recover();
        (yyval.interm.intermTypedNode) = parseContext->intermediate.addAssign((yyvsp[(2) - (3)].interm).op, (yyvsp[(1) - (3)].interm.intermTypedNode), (yyvsp[(3) - (3)].interm.intermTypedNode), (yyvsp[(2) - (3)].interm).line);
        if ((yyval.interm.intermTypedNode) == 0) {
            parseContext->assignError((yyvsp[(2) - (3)].interm).line, "assign", (yyvsp[(1) - (3)].interm.intermTypedNode)->getCompleteString(), (yyvsp[(3) - (3)].interm.intermTypedNode)->getCompleteString());
            parseContext->recover();
            (yyval.interm.intermTypedNode) = (yyvsp[(1) - (3)].interm.intermTypedNode);
        } else if (((yyvsp[(1) - (3)].interm.intermTypedNode)->isArray() || (yyvsp[(3) - (3)].interm.intermTypedNode)->isArray()) && parseContext->extensionErrorCheck((yyvsp[(2) - (3)].interm).line, "GL_3DL_array_objects"))
            parseContext->recover();
    }
    break;

  case 63:

    {                                    (yyval.interm).line = (yyvsp[(1) - (1)].lex).line; (yyval.interm).op = EOpAssign; }
    break;

  case 64:

    { FRAG_VERT_ONLY("*=", (yyvsp[(1) - (1)].lex).line);     (yyval.interm).line = (yyvsp[(1) - (1)].lex).line; (yyval.interm).op = EOpMulAssign; }
    break;

  case 65:

    { FRAG_VERT_ONLY("/=", (yyvsp[(1) - (1)].lex).line);     (yyval.interm).line = (yyvsp[(1) - (1)].lex).line; (yyval.interm).op = EOpDivAssign; }
    break;

  case 66:

    {                                    (yyval.interm).line = (yyvsp[(1) - (1)].lex).line; (yyval.interm).op = EOpAddAssign; }
    break;

  case 67:

    {                                    (yyval.interm).line = (yyvsp[(1) - (1)].lex).line; (yyval.interm).op = EOpSubAssign; }
    break;

  case 68:

    {
        (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode);
    }
    break;

  case 69:

    {
        (yyval.interm.intermTypedNode) = parseContext->intermediate.addComma((yyvsp[(1) - (3)].interm.intermTypedNode), (yyvsp[(3) - (3)].interm.intermTypedNode), (yyvsp[(2) - (3)].lex).line);
        if ((yyval.interm.intermTypedNode) == 0) {
            parseContext->binaryOpError((yyvsp[(2) - (3)].lex).line, ",", (yyvsp[(1) - (3)].interm.intermTypedNode)->getCompleteString(), (yyvsp[(3) - (3)].interm.intermTypedNode)->getCompleteString());
            parseContext->recover();
            (yyval.interm.intermTypedNode) = (yyvsp[(3) - (3)].interm.intermTypedNode);
        }
    }
    break;

  case 70:

    {
        if (parseContext->constErrorCheck((yyvsp[(1) - (1)].interm.intermTypedNode)))
            parseContext->recover();
        (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode);
    }
    break;

  case 71:

    {
        TFunction &function = *((yyvsp[(1) - (2)].interm).function);
        
        TIntermAggregate *prototype = new TIntermAggregate;
        prototype->setType(function.getReturnType());
        prototype->setName(function.getName());
        
        for (int i = 0; i < function.getParamCount(); i++)
        {
            TParameter &param = function[i];
            if (param.name != 0)
            {
                TVariable *variable = new TVariable(param.name, *param.type);
                
                prototype = parseContext->intermediate.growAggregate(prototype, parseContext->intermediate.addSymbol(variable->getUniqueId(), variable->getName(), variable->getType(), (yyvsp[(1) - (2)].interm).line), (yyvsp[(1) - (2)].interm).line);
            }
            else
            {
                prototype = parseContext->intermediate.growAggregate(prototype, parseContext->intermediate.addSymbol(0, "", *param.type, (yyvsp[(1) - (2)].interm).line), (yyvsp[(1) - (2)].interm).line);
            }
        }
        
        prototype->setOperator(EOpPrototype);
        (yyval.interm.intermNode) = prototype;
    }
    break;

  case 72:

    {
        if ((yyvsp[(1) - (2)].interm).intermAggregate)
            (yyvsp[(1) - (2)].interm).intermAggregate->setOperator(EOpDeclaration);
        (yyval.interm.intermNode) = (yyvsp[(1) - (2)].interm).intermAggregate;
    }
    break;

  case 73:

    {
        parseContext->symbolTable.setDefaultPrecision( (yyvsp[(3) - (4)].interm.type).type, (yyvsp[(2) - (4)].interm.precision) );
        (yyval.interm.intermNode) = 0;
    }
    break;

  case 74:

    {
        
        
        
        
        
        
        
        
        TFunction* prevDec = static_cast<TFunction*>(parseContext->symbolTable.find((yyvsp[(1) - (2)].interm.function)->getMangledName()));
        if (prevDec) {
            if (prevDec->getReturnType() != (yyvsp[(1) - (2)].interm.function)->getReturnType()) {
                parseContext->error((yyvsp[(2) - (2)].lex).line, "overloaded functions must have the same return type", (yyvsp[(1) - (2)].interm.function)->getReturnType().getBasicString(), "");
                parseContext->recover();
            }
            for (int i = 0; i < prevDec->getParamCount(); ++i) {
                if ((*prevDec)[i].type->getQualifier() != (*(yyvsp[(1) - (2)].interm.function))[i].type->getQualifier()) {
                    parseContext->error((yyvsp[(2) - (2)].lex).line, "overloaded functions must have the same parameter qualifiers", (*(yyvsp[(1) - (2)].interm.function))[i].type->getQualifierString(), "");
                    parseContext->recover();
                }
            }
        }

        
        
        
        
        
        (yyval.interm).function = (yyvsp[(1) - (2)].interm.function);
        (yyval.interm).line = (yyvsp[(2) - (2)].lex).line;

        parseContext->symbolTable.insert(*(yyval.interm).function);
    }
    break;

  case 75:

    {
        (yyval.interm.function) = (yyvsp[(1) - (1)].interm.function);
    }
    break;

  case 76:

    {
        (yyval.interm.function) = (yyvsp[(1) - (1)].interm.function);
    }
    break;

  case 77:

    {
        
        (yyval.interm.function) = (yyvsp[(1) - (2)].interm.function);
        if ((yyvsp[(2) - (2)].interm).param.type->getBasicType() != EbtVoid)
            (yyvsp[(1) - (2)].interm.function)->addParameter((yyvsp[(2) - (2)].interm).param);
        else
            delete (yyvsp[(2) - (2)].interm).param.type;
    }
    break;

  case 78:

    {
        
        
        
        
        if ((yyvsp[(3) - (3)].interm).param.type->getBasicType() == EbtVoid) {
            
            
            
            parseContext->error((yyvsp[(2) - (3)].lex).line, "cannot be an argument type except for '(void)'", "void", "");
            parseContext->recover();
            delete (yyvsp[(3) - (3)].interm).param.type;
        } else {
            
            (yyval.interm.function) = (yyvsp[(1) - (3)].interm.function);
            (yyvsp[(1) - (3)].interm.function)->addParameter((yyvsp[(3) - (3)].interm).param);
        }
    }
    break;

  case 79:

    {
        if ((yyvsp[(1) - (3)].interm.type).qualifier != EvqGlobal && (yyvsp[(1) - (3)].interm.type).qualifier != EvqTemporary) {
            parseContext->error((yyvsp[(2) - (3)].lex).line, "no qualifiers allowed for function return", getQualifierString((yyvsp[(1) - (3)].interm.type).qualifier), "");
            parseContext->recover();
        }
        
        if (parseContext->structQualifierErrorCheck((yyvsp[(2) - (3)].lex).line, (yyvsp[(1) - (3)].interm.type)))
            parseContext->recover();

        
        TFunction *function;
        TType type((yyvsp[(1) - (3)].interm.type));
        function = new TFunction((yyvsp[(2) - (3)].lex).string, type);
        (yyval.interm.function) = function;
    }
    break;

  case 80:

    {
        if ((yyvsp[(1) - (2)].interm.type).type == EbtVoid) {
            parseContext->error((yyvsp[(2) - (2)].lex).line, "illegal use of type 'void'", (yyvsp[(2) - (2)].lex).string->c_str(), "");
            parseContext->recover();
        }
        if (parseContext->reservedErrorCheck((yyvsp[(2) - (2)].lex).line, *(yyvsp[(2) - (2)].lex).string))
            parseContext->recover();
        TParameter param = {(yyvsp[(2) - (2)].lex).string, new TType((yyvsp[(1) - (2)].interm.type))};
        (yyval.interm).line = (yyvsp[(2) - (2)].lex).line;
        (yyval.interm).param = param;
    }
    break;

  case 81:

    {
        
        if (parseContext->arrayTypeErrorCheck((yyvsp[(3) - (5)].lex).line, (yyvsp[(1) - (5)].interm.type)))
            parseContext->recover();

        if (parseContext->reservedErrorCheck((yyvsp[(2) - (5)].lex).line, *(yyvsp[(2) - (5)].lex).string))
            parseContext->recover();

        int size;
        if (parseContext->arraySizeErrorCheck((yyvsp[(3) - (5)].lex).line, (yyvsp[(4) - (5)].interm.intermTypedNode), size))
            parseContext->recover();
        (yyvsp[(1) - (5)].interm.type).setArray(true, size);

        TType* type = new TType((yyvsp[(1) - (5)].interm.type));
        TParameter param = { (yyvsp[(2) - (5)].lex).string, type };
        (yyval.interm).line = (yyvsp[(2) - (5)].lex).line;
        (yyval.interm).param = param;
    }
    break;

  case 82:

    {
        (yyval.interm) = (yyvsp[(3) - (3)].interm);
        if (parseContext->paramErrorCheck((yyvsp[(3) - (3)].interm).line, (yyvsp[(1) - (3)].interm.type).qualifier, (yyvsp[(2) - (3)].interm.qualifier), (yyval.interm).param.type))
            parseContext->recover();
    }
    break;

  case 83:

    {
        (yyval.interm) = (yyvsp[(2) - (2)].interm);
        if (parseContext->parameterSamplerErrorCheck((yyvsp[(2) - (2)].interm).line, (yyvsp[(1) - (2)].interm.qualifier), *(yyvsp[(2) - (2)].interm).param.type))
            parseContext->recover();
        if (parseContext->paramErrorCheck((yyvsp[(2) - (2)].interm).line, EvqTemporary, (yyvsp[(1) - (2)].interm.qualifier), (yyval.interm).param.type))
            parseContext->recover();
    }
    break;

  case 84:

    {
        (yyval.interm) = (yyvsp[(3) - (3)].interm);
        if (parseContext->paramErrorCheck((yyvsp[(3) - (3)].interm).line, (yyvsp[(1) - (3)].interm.type).qualifier, (yyvsp[(2) - (3)].interm.qualifier), (yyval.interm).param.type))
            parseContext->recover();
    }
    break;

  case 85:

    {
        (yyval.interm) = (yyvsp[(2) - (2)].interm);
        if (parseContext->parameterSamplerErrorCheck((yyvsp[(2) - (2)].interm).line, (yyvsp[(1) - (2)].interm.qualifier), *(yyvsp[(2) - (2)].interm).param.type))
            parseContext->recover();
        if (parseContext->paramErrorCheck((yyvsp[(2) - (2)].interm).line, EvqTemporary, (yyvsp[(1) - (2)].interm.qualifier), (yyval.interm).param.type))
            parseContext->recover();
    }
    break;

  case 86:

    {
        (yyval.interm.qualifier) = EvqIn;
    }
    break;

  case 87:

    {
        (yyval.interm.qualifier) = EvqIn;
    }
    break;

  case 88:

    {
        (yyval.interm.qualifier) = EvqOut;
    }
    break;

  case 89:

    {
        (yyval.interm.qualifier) = EvqInOut;
    }
    break;

  case 90:

    {
        TParameter param = { 0, new TType((yyvsp[(1) - (1)].interm.type)) };
        (yyval.interm).param = param;
    }
    break;

  case 91:

    {
        (yyval.interm) = (yyvsp[(1) - (1)].interm);
        
        if ((yyval.interm).type.precision == EbpUndefined) {
            (yyval.interm).type.precision = parseContext->symbolTable.getDefaultPrecision((yyvsp[(1) - (1)].interm).type.type);
            if (parseContext->precisionErrorCheck((yyvsp[(1) - (1)].interm).line, (yyval.interm).type.precision, (yyvsp[(1) - (1)].interm).type.type)) {
                parseContext->recover();
            }
        }
    }
    break;

  case 92:

    {
        (yyval.interm).intermAggregate = parseContext->intermediate.growAggregate((yyvsp[(1) - (3)].interm).intermNode, parseContext->intermediate.addSymbol(0, *(yyvsp[(3) - (3)].lex).string, TType((yyvsp[(1) - (3)].interm).type), (yyvsp[(3) - (3)].lex).line), (yyvsp[(3) - (3)].lex).line);
        
        if (parseContext->structQualifierErrorCheck((yyvsp[(3) - (3)].lex).line, (yyval.interm).type))
            parseContext->recover();

        if (parseContext->nonInitConstErrorCheck((yyvsp[(3) - (3)].lex).line, *(yyvsp[(3) - (3)].lex).string, (yyval.interm).type))
            parseContext->recover();

        if (parseContext->nonInitErrorCheck((yyvsp[(3) - (3)].lex).line, *(yyvsp[(3) - (3)].lex).string, (yyval.interm).type))
            parseContext->recover();
    }
    break;

  case 93:

    {
        if (parseContext->structQualifierErrorCheck((yyvsp[(3) - (5)].lex).line, (yyvsp[(1) - (5)].interm).type))
            parseContext->recover();

        if (parseContext->nonInitConstErrorCheck((yyvsp[(3) - (5)].lex).line, *(yyvsp[(3) - (5)].lex).string, (yyvsp[(1) - (5)].interm).type))
            parseContext->recover();

        (yyval.interm) = (yyvsp[(1) - (5)].interm);

        if (parseContext->arrayTypeErrorCheck((yyvsp[(4) - (5)].lex).line, (yyvsp[(1) - (5)].interm).type) || parseContext->arrayQualifierErrorCheck((yyvsp[(4) - (5)].lex).line, (yyvsp[(1) - (5)].interm).type))
            parseContext->recover();
        else {
            (yyvsp[(1) - (5)].interm).type.setArray(true);
            TVariable* variable;
            if (parseContext->arrayErrorCheck((yyvsp[(4) - (5)].lex).line, *(yyvsp[(3) - (5)].lex).string, (yyvsp[(1) - (5)].interm).type, variable))
                parseContext->recover();
        }
    }
    break;

  case 94:

    {
        if (parseContext->structQualifierErrorCheck((yyvsp[(3) - (6)].lex).line, (yyvsp[(1) - (6)].interm).type))
            parseContext->recover();

        if (parseContext->nonInitConstErrorCheck((yyvsp[(3) - (6)].lex).line, *(yyvsp[(3) - (6)].lex).string, (yyvsp[(1) - (6)].interm).type))
            parseContext->recover();

        (yyval.interm) = (yyvsp[(1) - (6)].interm);

        if (parseContext->arrayTypeErrorCheck((yyvsp[(4) - (6)].lex).line, (yyvsp[(1) - (6)].interm).type) || parseContext->arrayQualifierErrorCheck((yyvsp[(4) - (6)].lex).line, (yyvsp[(1) - (6)].interm).type))
            parseContext->recover();
        else {
            int size;
            if (parseContext->arraySizeErrorCheck((yyvsp[(4) - (6)].lex).line, (yyvsp[(5) - (6)].interm.intermTypedNode), size))
                parseContext->recover();
            (yyvsp[(1) - (6)].interm).type.setArray(true, size);
            TVariable* variable;
            if (parseContext->arrayErrorCheck((yyvsp[(4) - (6)].lex).line, *(yyvsp[(3) - (6)].lex).string, (yyvsp[(1) - (6)].interm).type, variable))
                parseContext->recover();
            TType type = TType((yyvsp[(1) - (6)].interm).type);
            type.setArraySize(size);
            (yyval.interm).intermAggregate = parseContext->intermediate.growAggregate((yyvsp[(1) - (6)].interm).intermNode, parseContext->intermediate.addSymbol(0, *(yyvsp[(3) - (6)].lex).string, type, (yyvsp[(3) - (6)].lex).line), (yyvsp[(3) - (6)].lex).line);
        }
    }
    break;

  case 95:

    {
        if (parseContext->structQualifierErrorCheck((yyvsp[(3) - (7)].lex).line, (yyvsp[(1) - (7)].interm).type))
            parseContext->recover();

        (yyval.interm) = (yyvsp[(1) - (7)].interm);

        TVariable* variable = 0;
        if (parseContext->arrayTypeErrorCheck((yyvsp[(4) - (7)].lex).line, (yyvsp[(1) - (7)].interm).type) || parseContext->arrayQualifierErrorCheck((yyvsp[(4) - (7)].lex).line, (yyvsp[(1) - (7)].interm).type))
            parseContext->recover();
        else {
            (yyvsp[(1) - (7)].interm).type.setArray(true, (yyvsp[(7) - (7)].interm.intermTypedNode)->getType().getArraySize());
            if (parseContext->arrayErrorCheck((yyvsp[(4) - (7)].lex).line, *(yyvsp[(3) - (7)].lex).string, (yyvsp[(1) - (7)].interm).type, variable))
                parseContext->recover();
        }

        if (parseContext->extensionErrorCheck((yyval.interm).line, "GL_3DL_array_objects"))
            parseContext->recover();
        else {
            TIntermNode* intermNode;
            if (!parseContext->executeInitializer((yyvsp[(3) - (7)].lex).line, *(yyvsp[(3) - (7)].lex).string, (yyvsp[(1) - (7)].interm).type, (yyvsp[(7) - (7)].interm.intermTypedNode), intermNode, variable)) {
                
                
                
                if (intermNode)
                    (yyval.interm).intermAggregate = parseContext->intermediate.growAggregate((yyvsp[(1) - (7)].interm).intermNode, intermNode, (yyvsp[(6) - (7)].lex).line);
                else
                    (yyval.interm).intermAggregate = (yyvsp[(1) - (7)].interm).intermAggregate;
            } else {
                parseContext->recover();
                (yyval.interm).intermAggregate = 0;
            }
        }
    }
    break;

  case 96:

    {
        if (parseContext->structQualifierErrorCheck((yyvsp[(3) - (8)].lex).line, (yyvsp[(1) - (8)].interm).type))
            parseContext->recover();

        (yyval.interm) = (yyvsp[(1) - (8)].interm);

        TVariable* variable = 0;
        if (parseContext->arrayTypeErrorCheck((yyvsp[(4) - (8)].lex).line, (yyvsp[(1) - (8)].interm).type) || parseContext->arrayQualifierErrorCheck((yyvsp[(4) - (8)].lex).line, (yyvsp[(1) - (8)].interm).type))
            parseContext->recover();
        else {
            int size;
            if (parseContext->arraySizeErrorCheck((yyvsp[(4) - (8)].lex).line, (yyvsp[(5) - (8)].interm.intermTypedNode), size))
                parseContext->recover();
            (yyvsp[(1) - (8)].interm).type.setArray(true, size);
            if (parseContext->arrayErrorCheck((yyvsp[(4) - (8)].lex).line, *(yyvsp[(3) - (8)].lex).string, (yyvsp[(1) - (8)].interm).type, variable))
                parseContext->recover();
        }

        if (parseContext->extensionErrorCheck((yyval.interm).line, "GL_3DL_array_objects"))
            parseContext->recover();
        else {
            TIntermNode* intermNode;
            if (!parseContext->executeInitializer((yyvsp[(3) - (8)].lex).line, *(yyvsp[(3) - (8)].lex).string, (yyvsp[(1) - (8)].interm).type, (yyvsp[(8) - (8)].interm.intermTypedNode), intermNode, variable)) {
                
                
                
                if (intermNode)
                    (yyval.interm).intermAggregate = parseContext->intermediate.growAggregate((yyvsp[(1) - (8)].interm).intermNode, intermNode, (yyvsp[(7) - (8)].lex).line);
                else
                    (yyval.interm).intermAggregate = (yyvsp[(1) - (8)].interm).intermAggregate;
            } else {
                parseContext->recover();
                (yyval.interm).intermAggregate = 0;
            }
        }
    }
    break;

  case 97:

    {
        if (parseContext->structQualifierErrorCheck((yyvsp[(3) - (5)].lex).line, (yyvsp[(1) - (5)].interm).type))
            parseContext->recover();

        (yyval.interm) = (yyvsp[(1) - (5)].interm);

        TIntermNode* intermNode;
        if (!parseContext->executeInitializer((yyvsp[(3) - (5)].lex).line, *(yyvsp[(3) - (5)].lex).string, (yyvsp[(1) - (5)].interm).type, (yyvsp[(5) - (5)].interm.intermTypedNode), intermNode)) {
            
            
            
            if (intermNode)
        (yyval.interm).intermAggregate = parseContext->intermediate.growAggregate((yyvsp[(1) - (5)].interm).intermNode, intermNode, (yyvsp[(4) - (5)].lex).line);
            else
                (yyval.interm).intermAggregate = (yyvsp[(1) - (5)].interm).intermAggregate;
        } else {
            parseContext->recover();
            (yyval.interm).intermAggregate = 0;
        }
    }
    break;

  case 98:

    {
        (yyval.interm).type = (yyvsp[(1) - (1)].interm.type);
        (yyval.interm).intermAggregate = parseContext->intermediate.makeAggregate(parseContext->intermediate.addSymbol(0, "", TType((yyvsp[(1) - (1)].interm.type)), (yyvsp[(1) - (1)].interm.type).line), (yyvsp[(1) - (1)].interm.type).line);
    }
    break;

  case 99:

    {
        (yyval.interm).intermAggregate = parseContext->intermediate.makeAggregate(parseContext->intermediate.addSymbol(0, *(yyvsp[(2) - (2)].lex).string, TType((yyvsp[(1) - (2)].interm.type)), (yyvsp[(2) - (2)].lex).line), (yyvsp[(2) - (2)].lex).line);
        
        if (parseContext->structQualifierErrorCheck((yyvsp[(2) - (2)].lex).line, (yyval.interm).type))
            parseContext->recover();

        if (parseContext->nonInitConstErrorCheck((yyvsp[(2) - (2)].lex).line, *(yyvsp[(2) - (2)].lex).string, (yyval.interm).type))
            parseContext->recover();
            
            (yyval.interm).type = (yyvsp[(1) - (2)].interm.type);

        if (parseContext->nonInitErrorCheck((yyvsp[(2) - (2)].lex).line, *(yyvsp[(2) - (2)].lex).string, (yyval.interm).type))
            parseContext->recover();
    }
    break;

  case 100:

    {
        (yyval.interm).intermAggregate = parseContext->intermediate.makeAggregate(parseContext->intermediate.addSymbol(0, *(yyvsp[(2) - (4)].lex).string, TType((yyvsp[(1) - (4)].interm.type)), (yyvsp[(2) - (4)].lex).line), (yyvsp[(2) - (4)].lex).line);
        
        if (parseContext->structQualifierErrorCheck((yyvsp[(2) - (4)].lex).line, (yyvsp[(1) - (4)].interm.type)))
            parseContext->recover();

        if (parseContext->nonInitConstErrorCheck((yyvsp[(2) - (4)].lex).line, *(yyvsp[(2) - (4)].lex).string, (yyvsp[(1) - (4)].interm.type)))
            parseContext->recover();

        (yyval.interm).type = (yyvsp[(1) - (4)].interm.type);

        if (parseContext->arrayTypeErrorCheck((yyvsp[(3) - (4)].lex).line, (yyvsp[(1) - (4)].interm.type)) || parseContext->arrayQualifierErrorCheck((yyvsp[(3) - (4)].lex).line, (yyvsp[(1) - (4)].interm.type)))
            parseContext->recover();
        else {
            (yyvsp[(1) - (4)].interm.type).setArray(true);
            TVariable* variable;
            if (parseContext->arrayErrorCheck((yyvsp[(3) - (4)].lex).line, *(yyvsp[(2) - (4)].lex).string, (yyvsp[(1) - (4)].interm.type), variable))
                parseContext->recover();
        }
    }
    break;

  case 101:

    {
        TType type = TType((yyvsp[(1) - (5)].interm.type));
        int size;
        if (parseContext->arraySizeErrorCheck((yyvsp[(2) - (5)].lex).line, (yyvsp[(4) - (5)].interm.intermTypedNode), size))
            parseContext->recover();
        type.setArraySize(size);
        (yyval.interm).intermAggregate = parseContext->intermediate.makeAggregate(parseContext->intermediate.addSymbol(0, *(yyvsp[(2) - (5)].lex).string, type, (yyvsp[(2) - (5)].lex).line), (yyvsp[(2) - (5)].lex).line);
        
        if (parseContext->structQualifierErrorCheck((yyvsp[(2) - (5)].lex).line, (yyvsp[(1) - (5)].interm.type)))
            parseContext->recover();

        if (parseContext->nonInitConstErrorCheck((yyvsp[(2) - (5)].lex).line, *(yyvsp[(2) - (5)].lex).string, (yyvsp[(1) - (5)].interm.type)))
            parseContext->recover();

        (yyval.interm).type = (yyvsp[(1) - (5)].interm.type);

        if (parseContext->arrayTypeErrorCheck((yyvsp[(3) - (5)].lex).line, (yyvsp[(1) - (5)].interm.type)) || parseContext->arrayQualifierErrorCheck((yyvsp[(3) - (5)].lex).line, (yyvsp[(1) - (5)].interm.type)))
            parseContext->recover();
        else {
            int size;
            if (parseContext->arraySizeErrorCheck((yyvsp[(3) - (5)].lex).line, (yyvsp[(4) - (5)].interm.intermTypedNode), size))
                parseContext->recover();

            (yyvsp[(1) - (5)].interm.type).setArray(true, size);
            TVariable* variable;
            if (parseContext->arrayErrorCheck((yyvsp[(3) - (5)].lex).line, *(yyvsp[(2) - (5)].lex).string, (yyvsp[(1) - (5)].interm.type), variable))
                parseContext->recover();
        }
    }
    break;

  case 102:

    {
        if (parseContext->structQualifierErrorCheck((yyvsp[(2) - (4)].lex).line, (yyvsp[(1) - (4)].interm.type)))
            parseContext->recover();

        (yyval.interm).type = (yyvsp[(1) - (4)].interm.type);

        TIntermNode* intermNode;
        if (!parseContext->executeInitializer((yyvsp[(2) - (4)].lex).line, *(yyvsp[(2) - (4)].lex).string, (yyvsp[(1) - (4)].interm.type), (yyvsp[(4) - (4)].interm.intermTypedNode), intermNode)) {
        
        
        
            if(intermNode)
                (yyval.interm).intermAggregate = parseContext->intermediate.makeAggregate(intermNode, (yyvsp[(3) - (4)].lex).line);
            else
                (yyval.interm).intermAggregate = 0;
        } else {
            parseContext->recover();
            (yyval.interm).intermAggregate = 0;
        }
    }
    break;

  case 103:

    {
        VERTEX_ONLY("invariant declaration", (yyvsp[(1) - (2)].lex).line);
        (yyval.interm).qualifier = EvqInvariantVaryingOut;
        (yyval.interm).intermAggregate = 0;
    }
    break;

  case 104:

    {
        (yyval.interm.type) = (yyvsp[(1) - (1)].interm.type);

        if ((yyvsp[(1) - (1)].interm.type).array) {
            if (parseContext->extensionErrorCheck((yyvsp[(1) - (1)].interm.type).line, "GL_3DL_array_objects")) {
                parseContext->recover();
                (yyvsp[(1) - (1)].interm.type).setArray(false);
            }
        }
    }
    break;

  case 105:

    {
        if ((yyvsp[(2) - (2)].interm.type).array && parseContext->extensionErrorCheck((yyvsp[(2) - (2)].interm.type).line, "GL_3DL_array_objects")) {
            parseContext->recover();
            (yyvsp[(2) - (2)].interm.type).setArray(false);
        }
        if ((yyvsp[(2) - (2)].interm.type).array && parseContext->arrayQualifierErrorCheck((yyvsp[(2) - (2)].interm.type).line, (yyvsp[(1) - (2)].interm.type))) {
            parseContext->recover();
            (yyvsp[(2) - (2)].interm.type).setArray(false);
        }

        if ((yyvsp[(1) - (2)].interm.type).qualifier == EvqAttribute &&
            ((yyvsp[(2) - (2)].interm.type).type == EbtBool || (yyvsp[(2) - (2)].interm.type).type == EbtInt)) {
            parseContext->error((yyvsp[(2) - (2)].interm.type).line, "cannot be bool or int", getQualifierString((yyvsp[(1) - (2)].interm.type).qualifier), "");
            parseContext->recover();
        }
        if (((yyvsp[(1) - (2)].interm.type).qualifier == EvqVaryingIn || (yyvsp[(1) - (2)].interm.type).qualifier == EvqVaryingOut) &&
            ((yyvsp[(2) - (2)].interm.type).type == EbtBool || (yyvsp[(2) - (2)].interm.type).type == EbtInt)) {
            parseContext->error((yyvsp[(2) - (2)].interm.type).line, "cannot be bool or int", getQualifierString((yyvsp[(1) - (2)].interm.type).qualifier), "");
            parseContext->recover();
        }
        (yyval.interm.type) = (yyvsp[(2) - (2)].interm.type);
        (yyval.interm.type).qualifier = (yyvsp[(1) - (2)].interm.type).qualifier;
    }
    break;

  case 106:

    {
        (yyval.interm.type).setBasic(EbtVoid, EvqConst, (yyvsp[(1) - (1)].lex).line);
    }
    break;

  case 107:

    {
        VERTEX_ONLY("attribute", (yyvsp[(1) - (1)].lex).line);
        if (parseContext->globalErrorCheck((yyvsp[(1) - (1)].lex).line, parseContext->symbolTable.atGlobalLevel(), "attribute"))
            parseContext->recover();
        (yyval.interm.type).setBasic(EbtVoid, EvqAttribute, (yyvsp[(1) - (1)].lex).line);
    }
    break;

  case 108:

    {
        if (parseContext->globalErrorCheck((yyvsp[(1) - (1)].lex).line, parseContext->symbolTable.atGlobalLevel(), "varying"))
            parseContext->recover();
        if (parseContext->language == EShLangVertex)
            (yyval.interm.type).setBasic(EbtVoid, EvqVaryingOut, (yyvsp[(1) - (1)].lex).line);
        else
            (yyval.interm.type).setBasic(EbtVoid, EvqVaryingIn, (yyvsp[(1) - (1)].lex).line);
    }
    break;

  case 109:

    {
        if (parseContext->globalErrorCheck((yyvsp[(1) - (2)].lex).line, parseContext->symbolTable.atGlobalLevel(), "invariant varying"))
            parseContext->recover();
        if (parseContext->language == EShLangVertex)
            (yyval.interm.type).setBasic(EbtVoid, EvqInvariantVaryingOut, (yyvsp[(1) - (2)].lex).line);
        else
            (yyval.interm.type).setBasic(EbtVoid, EvqInvariantVaryingIn, (yyvsp[(1) - (2)].lex).line);
    }
    break;

  case 110:

    {
        if (parseContext->globalErrorCheck((yyvsp[(1) - (1)].lex).line, parseContext->symbolTable.atGlobalLevel(), "uniform"))
            parseContext->recover();
        (yyval.interm.type).setBasic(EbtVoid, EvqUniform, (yyvsp[(1) - (1)].lex).line);
    }
    break;

  case 111:

    {
        (yyval.interm.type) = (yyvsp[(1) - (1)].interm.type);
    }
    break;

  case 112:

    {
        (yyval.interm.type) = (yyvsp[(2) - (2)].interm.type);
        (yyval.interm.type).precision = (yyvsp[(1) - (2)].interm.precision);
    }
    break;

  case 113:

    {
        (yyval.interm.precision) = EbpHigh;
    }
    break;

  case 114:

    {
        (yyval.interm.precision) = EbpMedium;
    }
    break;

  case 115:

    {
        (yyval.interm.precision) = EbpLow;
    }
    break;

  case 116:

    {
        (yyval.interm.type) = (yyvsp[(1) - (1)].interm.type);
    }
    break;

  case 117:

    {
        (yyval.interm.type) = (yyvsp[(1) - (4)].interm.type);

        if (parseContext->arrayTypeErrorCheck((yyvsp[(2) - (4)].lex).line, (yyvsp[(1) - (4)].interm.type)))
            parseContext->recover();
        else {
            int size;
            if (parseContext->arraySizeErrorCheck((yyvsp[(2) - (4)].lex).line, (yyvsp[(3) - (4)].interm.intermTypedNode), size))
                parseContext->recover();
            (yyval.interm.type).setArray(true, size);
        }
    }
    break;

  case 118:

    {
        TQualifier qual = parseContext->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtVoid, qual, (yyvsp[(1) - (1)].lex).line);
    }
    break;

  case 119:

    {
        TQualifier qual = parseContext->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtFloat, qual, (yyvsp[(1) - (1)].lex).line);
    }
    break;

  case 120:

    {
        TQualifier qual = parseContext->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtInt, qual, (yyvsp[(1) - (1)].lex).line);
    }
    break;

  case 121:

    {
        TQualifier qual = parseContext->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtBool, qual, (yyvsp[(1) - (1)].lex).line);
    }
    break;

  case 122:

    {
        TQualifier qual = parseContext->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtFloat, qual, (yyvsp[(1) - (1)].lex).line);
        (yyval.interm.type).setAggregate(2);
    }
    break;

  case 123:

    {
        TQualifier qual = parseContext->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtFloat, qual, (yyvsp[(1) - (1)].lex).line);
        (yyval.interm.type).setAggregate(3);
    }
    break;

  case 124:

    {
        TQualifier qual = parseContext->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtFloat, qual, (yyvsp[(1) - (1)].lex).line);
        (yyval.interm.type).setAggregate(4);
    }
    break;

  case 125:

    {
        TQualifier qual = parseContext->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtBool, qual, (yyvsp[(1) - (1)].lex).line);
        (yyval.interm.type).setAggregate(2);
    }
    break;

  case 126:

    {
        TQualifier qual = parseContext->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtBool, qual, (yyvsp[(1) - (1)].lex).line);
        (yyval.interm.type).setAggregate(3);
    }
    break;

  case 127:

    {
        TQualifier qual = parseContext->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtBool, qual, (yyvsp[(1) - (1)].lex).line);
        (yyval.interm.type).setAggregate(4);
    }
    break;

  case 128:

    {
        TQualifier qual = parseContext->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtInt, qual, (yyvsp[(1) - (1)].lex).line);
        (yyval.interm.type).setAggregate(2);
    }
    break;

  case 129:

    {
        TQualifier qual = parseContext->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtInt, qual, (yyvsp[(1) - (1)].lex).line);
        (yyval.interm.type).setAggregate(3);
    }
    break;

  case 130:

    {
        TQualifier qual = parseContext->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtInt, qual, (yyvsp[(1) - (1)].lex).line);
        (yyval.interm.type).setAggregate(4);
    }
    break;

  case 131:

    {
        FRAG_VERT_ONLY("mat2", (yyvsp[(1) - (1)].lex).line);
        TQualifier qual = parseContext->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtFloat, qual, (yyvsp[(1) - (1)].lex).line);
        (yyval.interm.type).setAggregate(2, true);
    }
    break;

  case 132:

    {
        FRAG_VERT_ONLY("mat3", (yyvsp[(1) - (1)].lex).line);
        TQualifier qual = parseContext->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtFloat, qual, (yyvsp[(1) - (1)].lex).line);
        (yyval.interm.type).setAggregate(3, true);
    }
    break;

  case 133:

    {
        FRAG_VERT_ONLY("mat4", (yyvsp[(1) - (1)].lex).line);
        TQualifier qual = parseContext->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtFloat, qual, (yyvsp[(1) - (1)].lex).line);
        (yyval.interm.type).setAggregate(4, true);
    }
    break;

  case 134:

    {
        FRAG_VERT_ONLY("sampler2D", (yyvsp[(1) - (1)].lex).line);
        TQualifier qual = parseContext->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtSampler2D, qual, (yyvsp[(1) - (1)].lex).line);
    }
    break;

  case 135:

    {
        FRAG_VERT_ONLY("samplerCube", (yyvsp[(1) - (1)].lex).line);
        TQualifier qual = parseContext->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtSamplerCube, qual, (yyvsp[(1) - (1)].lex).line);
    }
    break;

  case 136:

    {
        FRAG_VERT_ONLY("struct", (yyvsp[(1) - (1)].interm.type).line);
        (yyval.interm.type) = (yyvsp[(1) - (1)].interm.type);
        (yyval.interm.type).qualifier = parseContext->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
    }
    break;

  case 137:

    {
        
        
        
        
        TType& structure = static_cast<TVariable*>((yyvsp[(1) - (1)].lex).symbol)->getType();
        TQualifier qual = parseContext->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtStruct, qual, (yyvsp[(1) - (1)].lex).line);
        (yyval.interm.type).userDef = &structure;
    }
    break;

  case 138:

    {
        if (parseContext->reservedErrorCheck((yyvsp[(2) - (5)].lex).line, *(yyvsp[(2) - (5)].lex).string))
            parseContext->recover();

        TType* structure = new TType((yyvsp[(4) - (5)].interm.typeList), *(yyvsp[(2) - (5)].lex).string);
        TVariable* userTypeDef = new TVariable((yyvsp[(2) - (5)].lex).string, *structure, true);
        if (! parseContext->symbolTable.insert(*userTypeDef)) {
            parseContext->error((yyvsp[(2) - (5)].lex).line, "redefinition", (yyvsp[(2) - (5)].lex).string->c_str(), "struct");
            parseContext->recover();
        }
        (yyval.interm.type).setBasic(EbtStruct, EvqTemporary, (yyvsp[(1) - (5)].lex).line);
        (yyval.interm.type).userDef = structure;
    }
    break;

  case 139:

    {
        TType* structure = new TType((yyvsp[(3) - (4)].interm.typeList), TString(""));
        (yyval.interm.type).setBasic(EbtStruct, EvqTemporary, (yyvsp[(1) - (4)].lex).line);
        (yyval.interm.type).userDef = structure;
    }
    break;

  case 140:

    {
        (yyval.interm.typeList) = (yyvsp[(1) - (1)].interm.typeList);
    }
    break;

  case 141:

    {
        (yyval.interm.typeList) = (yyvsp[(1) - (2)].interm.typeList);
        for (unsigned int i = 0; i < (yyvsp[(2) - (2)].interm.typeList)->size(); ++i) {
            for (unsigned int j = 0; j < (yyval.interm.typeList)->size(); ++j) {
                if ((*(yyval.interm.typeList))[j].type->getFieldName() == (*(yyvsp[(2) - (2)].interm.typeList))[i].type->getFieldName()) {
                    parseContext->error((*(yyvsp[(2) - (2)].interm.typeList))[i].line, "duplicate field name in structure:", "struct", (*(yyvsp[(2) - (2)].interm.typeList))[i].type->getFieldName().c_str());
                    parseContext->recover();
                }
            }
            (yyval.interm.typeList)->push_back((*(yyvsp[(2) - (2)].interm.typeList))[i]);
        }
    }
    break;

  case 142:

    {
        (yyval.interm.typeList) = (yyvsp[(2) - (3)].interm.typeList);

        if (parseContext->voidErrorCheck((yyvsp[(1) - (3)].interm.type).line, (*(yyvsp[(2) - (3)].interm.typeList))[0].type->getFieldName(), (yyvsp[(1) - (3)].interm.type))) {
            parseContext->recover();
        }
        for (unsigned int i = 0; i < (yyval.interm.typeList)->size(); ++i) {
            
            
            
            (*(yyval.interm.typeList))[i].type->setType((yyvsp[(1) - (3)].interm.type).type, (yyvsp[(1) - (3)].interm.type).size, (yyvsp[(1) - (3)].interm.type).matrix, (yyvsp[(1) - (3)].interm.type).userDef);

            
            if ((*(yyval.interm.typeList))[i].type->isArray()) {
                if (parseContext->arrayTypeErrorCheck((yyvsp[(1) - (3)].interm.type).line, (yyvsp[(1) - (3)].interm.type)))
                    parseContext->recover();
            }
            if ((yyvsp[(1) - (3)].interm.type).array)
                (*(yyval.interm.typeList))[i].type->setArraySize((yyvsp[(1) - (3)].interm.type).arraySize);
            if ((yyvsp[(1) - (3)].interm.type).userDef)
                (*(yyval.interm.typeList))[i].type->setTypeName((yyvsp[(1) - (3)].interm.type).userDef->getTypeName());
        }
    }
    break;

  case 143:

    {
        (yyval.interm.typeList) = NewPoolTTypeList();
        (yyval.interm.typeList)->push_back((yyvsp[(1) - (1)].interm.typeLine));
    }
    break;

  case 144:

    {
        (yyval.interm.typeList)->push_back((yyvsp[(3) - (3)].interm.typeLine));
    }
    break;

  case 145:

    {
        if (parseContext->reservedErrorCheck((yyvsp[(1) - (1)].lex).line, *(yyvsp[(1) - (1)].lex).string))
            parseContext->recover();

        (yyval.interm.typeLine).type = new TType(EbtVoid, EbpUndefined);
        (yyval.interm.typeLine).line = (yyvsp[(1) - (1)].lex).line;
        (yyval.interm.typeLine).type->setFieldName(*(yyvsp[(1) - (1)].lex).string);
    }
    break;

  case 146:

    {
        if (parseContext->reservedErrorCheck((yyvsp[(1) - (4)].lex).line, *(yyvsp[(1) - (4)].lex).string))
            parseContext->recover();

        (yyval.interm.typeLine).type = new TType(EbtVoid, EbpUndefined);
        (yyval.interm.typeLine).line = (yyvsp[(1) - (4)].lex).line;
        (yyval.interm.typeLine).type->setFieldName(*(yyvsp[(1) - (4)].lex).string);

        int size;
        if (parseContext->arraySizeErrorCheck((yyvsp[(2) - (4)].lex).line, (yyvsp[(3) - (4)].interm.intermTypedNode), size))
            parseContext->recover();
        (yyval.interm.typeLine).type->setArraySize(size);
    }
    break;

  case 147:

    { (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode); }
    break;

  case 148:

    { (yyval.interm.intermNode) = (yyvsp[(1) - (1)].interm.intermNode); }
    break;

  case 149:

    { (yyval.interm.intermNode) = (yyvsp[(1) - (1)].interm.intermAggregate); }
    break;

  case 150:

    { (yyval.interm.intermNode) = (yyvsp[(1) - (1)].interm.intermNode); }
    break;

  case 151:

    { (yyval.interm.intermNode) = (yyvsp[(1) - (1)].interm.intermNode); }
    break;

  case 152:

    { (yyval.interm.intermNode) = (yyvsp[(1) - (1)].interm.intermNode); }
    break;

  case 153:

    { (yyval.interm.intermNode) = (yyvsp[(1) - (1)].interm.intermNode); }
    break;

  case 154:

    { (yyval.interm.intermNode) = (yyvsp[(1) - (1)].interm.intermNode); }
    break;

  case 155:

    { (yyval.interm.intermNode) = (yyvsp[(1) - (1)].interm.intermNode); }
    break;

  case 156:

    { (yyval.interm.intermAggregate) = 0; }
    break;

  case 157:

    { parseContext->symbolTable.push(); }
    break;

  case 158:

    { parseContext->symbolTable.pop(); }
    break;

  case 159:

    {
        if ((yyvsp[(3) - (5)].interm.intermAggregate) != 0)
            (yyvsp[(3) - (5)].interm.intermAggregate)->setOperator(EOpSequence);
        (yyval.interm.intermAggregate) = (yyvsp[(3) - (5)].interm.intermAggregate);
    }
    break;

  case 160:

    { (yyval.interm.intermNode) = (yyvsp[(1) - (1)].interm.intermNode); }
    break;

  case 161:

    { (yyval.interm.intermNode) = (yyvsp[(1) - (1)].interm.intermNode); }
    break;

  case 162:

    {
        (yyval.interm.intermNode) = 0;
    }
    break;

  case 163:

    {
        if ((yyvsp[(2) - (3)].interm.intermAggregate))
            (yyvsp[(2) - (3)].interm.intermAggregate)->setOperator(EOpSequence);
        (yyval.interm.intermNode) = (yyvsp[(2) - (3)].interm.intermAggregate);
    }
    break;

  case 164:

    {
        (yyval.interm.intermAggregate) = parseContext->intermediate.makeAggregate((yyvsp[(1) - (1)].interm.intermNode), 0);
    }
    break;

  case 165:

    {
        (yyval.interm.intermAggregate) = parseContext->intermediate.growAggregate((yyvsp[(1) - (2)].interm.intermAggregate), (yyvsp[(2) - (2)].interm.intermNode), 0);
    }
    break;

  case 166:

    { (yyval.interm.intermNode) = 0; }
    break;

  case 167:

    { (yyval.interm.intermNode) = static_cast<TIntermNode*>((yyvsp[(1) - (2)].interm.intermTypedNode)); }
    break;

  case 168:

    {
        if (parseContext->boolErrorCheck((yyvsp[(1) - (5)].lex).line, (yyvsp[(3) - (5)].interm.intermTypedNode)))
            parseContext->recover();
        (yyval.interm.intermNode) = parseContext->intermediate.addSelection((yyvsp[(3) - (5)].interm.intermTypedNode), (yyvsp[(5) - (5)].interm.nodePair), (yyvsp[(1) - (5)].lex).line);
    }
    break;

  case 169:

    {
        (yyval.interm.nodePair).node1 = (yyvsp[(1) - (3)].interm.intermNode);
        (yyval.interm.nodePair).node2 = (yyvsp[(3) - (3)].interm.intermNode);
    }
    break;

  case 170:

    {
        (yyval.interm.nodePair).node1 = (yyvsp[(1) - (1)].interm.intermNode);
        (yyval.interm.nodePair).node2 = 0;
    }
    break;

  case 171:

    {
        (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode);
        if (parseContext->boolErrorCheck((yyvsp[(1) - (1)].interm.intermTypedNode)->getLine(), (yyvsp[(1) - (1)].interm.intermTypedNode)))
            parseContext->recover();
    }
    break;

  case 172:

    {
        TIntermNode* intermNode;
        if (parseContext->structQualifierErrorCheck((yyvsp[(2) - (4)].lex).line, (yyvsp[(1) - (4)].interm.type)))
            parseContext->recover();
        if (parseContext->boolErrorCheck((yyvsp[(2) - (4)].lex).line, (yyvsp[(1) - (4)].interm.type)))
            parseContext->recover();

        if (!parseContext->executeInitializer((yyvsp[(2) - (4)].lex).line, *(yyvsp[(2) - (4)].lex).string, (yyvsp[(1) - (4)].interm.type), (yyvsp[(4) - (4)].interm.intermTypedNode), intermNode))
            (yyval.interm.intermTypedNode) = (yyvsp[(4) - (4)].interm.intermTypedNode);
        else {
            parseContext->recover();
            (yyval.interm.intermTypedNode) = 0;
        }
    }
    break;

  case 173:

    { parseContext->symbolTable.push(); ++parseContext->loopNestingLevel; }
    break;

  case 174:

    {
        parseContext->symbolTable.pop();
        (yyval.interm.intermNode) = parseContext->intermediate.addLoop(0, (yyvsp[(6) - (6)].interm.intermNode), (yyvsp[(4) - (6)].interm.intermTypedNode), 0, true, (yyvsp[(1) - (6)].lex).line);
        --parseContext->loopNestingLevel;
    }
    break;

  case 175:

    { ++parseContext->loopNestingLevel; }
    break;

  case 176:

    {
        if (parseContext->boolErrorCheck((yyvsp[(8) - (8)].lex).line, (yyvsp[(6) - (8)].interm.intermTypedNode)))
            parseContext->recover();

        (yyval.interm.intermNode) = parseContext->intermediate.addLoop(0, (yyvsp[(3) - (8)].interm.intermNode), (yyvsp[(6) - (8)].interm.intermTypedNode), 0, false, (yyvsp[(4) - (8)].lex).line);
        --parseContext->loopNestingLevel;
    }
    break;

  case 177:

    { parseContext->symbolTable.push(); ++parseContext->loopNestingLevel; }
    break;

  case 178:

    {
        parseContext->symbolTable.pop();
        (yyval.interm.intermNode) = parseContext->intermediate.addLoop((yyvsp[(4) - (7)].interm.intermNode), (yyvsp[(7) - (7)].interm.intermNode), reinterpret_cast<TIntermTyped*>((yyvsp[(5) - (7)].interm.nodePair).node1), reinterpret_cast<TIntermTyped*>((yyvsp[(5) - (7)].interm.nodePair).node2), true, (yyvsp[(1) - (7)].lex).line);
        --parseContext->loopNestingLevel;
    }
    break;

  case 179:

    {
        (yyval.interm.intermNode) = (yyvsp[(1) - (1)].interm.intermNode);
    }
    break;

  case 180:

    {
        (yyval.interm.intermNode) = (yyvsp[(1) - (1)].interm.intermNode);
    }
    break;

  case 181:

    {
        (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode);
    }
    break;

  case 182:

    {
        (yyval.interm.intermTypedNode) = 0;
    }
    break;

  case 183:

    {
        (yyval.interm.nodePair).node1 = (yyvsp[(1) - (2)].interm.intermTypedNode);
        (yyval.interm.nodePair).node2 = 0;
    }
    break;

  case 184:

    {
        (yyval.interm.nodePair).node1 = (yyvsp[(1) - (3)].interm.intermTypedNode);
        (yyval.interm.nodePair).node2 = (yyvsp[(3) - (3)].interm.intermTypedNode);
    }
    break;

  case 185:

    {
        if (parseContext->loopNestingLevel <= 0) {
            parseContext->error((yyvsp[(1) - (2)].lex).line, "continue statement only allowed in loops", "", "");
            parseContext->recover();
        }
        (yyval.interm.intermNode) = parseContext->intermediate.addBranch(EOpContinue, (yyvsp[(1) - (2)].lex).line);
    }
    break;

  case 186:

    {
        if (parseContext->loopNestingLevel <= 0) {
            parseContext->error((yyvsp[(1) - (2)].lex).line, "break statement only allowed in loops", "", "");
            parseContext->recover();
        }
        (yyval.interm.intermNode) = parseContext->intermediate.addBranch(EOpBreak, (yyvsp[(1) - (2)].lex).line);
    }
    break;

  case 187:

    {
        (yyval.interm.intermNode) = parseContext->intermediate.addBranch(EOpReturn, (yyvsp[(1) - (2)].lex).line);
        if (parseContext->currentFunctionType->getBasicType() != EbtVoid) {
            parseContext->error((yyvsp[(1) - (2)].lex).line, "non-void function must return a value", "return", "");
            parseContext->recover();
        }
    }
    break;

  case 188:

    {
        (yyval.interm.intermNode) = parseContext->intermediate.addBranch(EOpReturn, (yyvsp[(2) - (3)].interm.intermTypedNode), (yyvsp[(1) - (3)].lex).line);
        parseContext->functionReturnsValue = true;
        if (parseContext->currentFunctionType->getBasicType() == EbtVoid) {
            parseContext->error((yyvsp[(1) - (3)].lex).line, "void function cannot return a value", "return", "");
            parseContext->recover();
        } else if (*(parseContext->currentFunctionType) != (yyvsp[(2) - (3)].interm.intermTypedNode)->getType()) {
            parseContext->error((yyvsp[(1) - (3)].lex).line, "function return is not matching type:", "return", "");
            parseContext->recover();
        }
    }
    break;

  case 189:

    {
        FRAG_ONLY("discard", (yyvsp[(1) - (2)].lex).line);
        (yyval.interm.intermNode) = parseContext->intermediate.addBranch(EOpKill, (yyvsp[(1) - (2)].lex).line);
    }
    break;

  case 190:

    {
        (yyval.interm.intermNode) = (yyvsp[(1) - (1)].interm.intermNode);
        parseContext->treeRoot = (yyval.interm.intermNode);
    }
    break;

  case 191:

    {
        (yyval.interm.intermNode) = parseContext->intermediate.growAggregate((yyvsp[(1) - (2)].interm.intermNode), (yyvsp[(2) - (2)].interm.intermNode), 0);
        parseContext->treeRoot = (yyval.interm.intermNode);
    }
    break;

  case 192:

    {
        (yyval.interm.intermNode) = (yyvsp[(1) - (1)].interm.intermNode);
    }
    break;

  case 193:

    {
        (yyval.interm.intermNode) = (yyvsp[(1) - (1)].interm.intermNode);
    }
    break;

  case 194:

    {
        TFunction& function = *((yyvsp[(1) - (1)].interm).function);
        TFunction* prevDec = static_cast<TFunction*>(parseContext->symbolTable.find(function.getMangledName()));
        
        
        
        
        
        if (prevDec->isDefined()) {
            
            
            
            parseContext->error((yyvsp[(1) - (1)].interm).line, "function already has a body", function.getName().c_str(), "");
            parseContext->recover();
        }
        prevDec->setDefined();

        
        
        
        if (function.getName() == "main") {
            if (function.getParamCount() > 0) {
                parseContext->error((yyvsp[(1) - (1)].interm).line, "function cannot take any parameter(s)", function.getName().c_str(), "");
                parseContext->recover();
            }
            if (function.getReturnType().getBasicType() != EbtVoid) {
                parseContext->error((yyvsp[(1) - (1)].interm).line, "", function.getReturnType().getBasicString(), "main function cannot return a value");
                parseContext->recover();
            }
        }

        
        
        
        parseContext->symbolTable.push();

        
        
        
        parseContext->currentFunctionType = &(prevDec->getReturnType());
        parseContext->functionReturnsValue = false;

        
        
        
        
        
        
        
        
        TIntermAggregate* paramNodes = new TIntermAggregate;
        for (int i = 0; i < function.getParamCount(); i++) {
            TParameter& param = function[i];
            if (param.name != 0) {
                TVariable *variable = new TVariable(param.name, *param.type);
                
                
                
                if (! parseContext->symbolTable.insert(*variable)) {
                    parseContext->error((yyvsp[(1) - (1)].interm).line, "redefinition", variable->getName().c_str(), "");
                    parseContext->recover();
                    delete variable;
                }
                
                
                
                param.name = 0;

                
                
                
                paramNodes = parseContext->intermediate.growAggregate(
                                               paramNodes,
                                               parseContext->intermediate.addSymbol(variable->getUniqueId(),
                                                                       variable->getName(),
                                                                       variable->getType(), (yyvsp[(1) - (1)].interm).line),
                                               (yyvsp[(1) - (1)].interm).line);
            } else {
                paramNodes = parseContext->intermediate.growAggregate(paramNodes, parseContext->intermediate.addSymbol(0, "", *param.type, (yyvsp[(1) - (1)].interm).line), (yyvsp[(1) - (1)].interm).line);
            }
        }
        parseContext->intermediate.setAggregateOperator(paramNodes, EOpParameters, (yyvsp[(1) - (1)].interm).line);
        (yyvsp[(1) - (1)].interm).intermAggregate = paramNodes;
        parseContext->loopNestingLevel = 0;
    }
    break;

  case 195:

    {
        
        
        if (parseContext->currentFunctionType->getBasicType() != EbtVoid && ! parseContext->functionReturnsValue) {
            parseContext->error((yyvsp[(1) - (3)].interm).line, "function does not return a value:", "", (yyvsp[(1) - (3)].interm).function->getName().c_str());
            parseContext->recover();
        }
        parseContext->symbolTable.pop();
        (yyval.interm.intermNode) = parseContext->intermediate.growAggregate((yyvsp[(1) - (3)].interm).intermAggregate, (yyvsp[(3) - (3)].interm.intermNode), 0);
        parseContext->intermediate.setAggregateOperator((yyval.interm.intermNode), EOpFunction, (yyvsp[(1) - (3)].interm).line);
        (yyval.interm.intermNode)->getAsAggregate()->setName((yyvsp[(1) - (3)].interm).function->getMangledName().c_str());
        (yyval.interm.intermNode)->getAsAggregate()->setType((yyvsp[(1) - (3)].interm).function->getReturnType());

        
        
        (yyval.interm.intermNode)->getAsAggregate()->setOptimize(parseContext->contextPragma.optimize);
        (yyval.interm.intermNode)->getAsAggregate()->setDebug(parseContext->contextPragma.debug);
        (yyval.interm.intermNode)->getAsAggregate()->addToPragmaTable(parseContext->contextPragma.pragmaTable);
    }
    break;



      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  



  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;





yyerrlab:
  
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



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
		      yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  

  goto yyerrlab1;





yyerrorlab:

  


  if ( 0)
     goto yyerrorlab;

  

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
      if (yyn != YYPACT_NINF)
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


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;


  
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;





yyacceptlab:
  yyresult = 0;
  goto yyreturn;




yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined(yyoverflow) || YYERROR_VERBOSE



yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  
#endif

yyreturn:
  if (yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
  

  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
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





