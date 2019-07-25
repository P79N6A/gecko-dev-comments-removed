













































#define YYBISON 1


#define YYBISON_VERSION "2.3"


#define YYSKELETON_NAME "yacc.c"


#define YYPURE 1


#define YYLSP_NEEDED 1


#define yyparse ppparse
#define yylex   pplex
#define yyerror pperror
#define yylval  pplval
#define yychar  ppchar
#define yydebug ppdebug
#define yynerrs ppnerrs
#define yylloc pplloc


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

#define HASH 258
#define HASH_UNDEF 259
#define HASH_IF 260
#define HASH_IFDEF 261
#define HASH_IFNDEF 262
#define HASH_ELSE 263
#define HASH_ELIF 264
#define HASH_ENDIF 265
#define DEFINED 266
#define HASH_ERROR 267
#define HASH_PRAGMA 268
#define HASH_EXTENSION 269
#define HASH_VERSION 270
#define HASH_LINE 271
#define HASH_DEFINE_OBJ 272
#define HASH_DEFINE_FUNC 273
#define INT_CONSTANT 274
#define FLOAT_CONSTANT 275
#define IDENTIFIER 276















#include "Context.h"

#define YYLEX_PARAM context->lexer()
#define YYDEBUG 1



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

#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE

{
    int ival;
    std::string* sval;
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





extern int yylex(YYSTYPE* lvalp, YYLTYPE* llocp, void* lexer);
static void yyerror(YYLTYPE* llocp,
                    pp::Context* context,
                    const char* reason);

static void pushConditionalBlock(pp::Context* context, bool condition);
static void popConditionalBlock(pp::Context* context);





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
YYID (int i)
#else
static int
YYID (i)
    int i;
#endif
{
  return i;
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
	 || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
	     && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))


union yyalloc
{
  yytype_int16 yyss;
  YYSTYPE yyvs;
    YYLTYPE yyls;
};


# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)



# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE) + sizeof (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)



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






# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif


#define YYFINAL  2

#define YYLAST   247


#define YYNTOKENS  47

#define YYNNTS  12

#define YYNRULES  62

#define YYNSTATES  91


#define YYUNDEFTOK  2
#define YYMAXUTOK   276

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)


static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      22,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    43,     2,     2,     2,    37,    40,     2,
      23,    24,    36,    33,    25,    34,    32,    35,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    44,    45,
      28,    42,    29,    46,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    26,     2,    27,    38,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    30,    39,    31,    41,     2,     2,     2,
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
      15,    16,    17,    18,    19,    20,    21
};

#if YYDEBUG


static const yytype_uint8 yyprhs[] =
{
       0,     0,     3,     4,     7,     9,    11,    13,    16,    19,
      23,    30,    34,    38,    42,    46,    50,    53,    56,    59,
      62,    65,    68,    71,    72,    74,    75,    77,    81,    83,
      86,    88,    91,    94,    99,   101,   103,   105,   107,   109,
     111,   113,   115,   117,   119,   121,   123,   125,   127,   129,
     131,   133,   135,   137,   139,   141,   143,   145,   147,   149,
     151,   153,   155
};


static const yytype_int8 yyrhs[] =
{
      48,     0,    -1,    -1,    48,    49,    -1,    50,    -1,    51,
      -1,    22,    -1,    55,    22,    -1,     3,    22,    -1,    17,
      52,    22,    -1,    18,    23,    53,    24,    52,    22,    -1,
       4,    21,    22,    -1,     5,    54,    22,    -1,     6,    21,
      22,    -1,     7,    21,    22,    -1,     9,    54,    22,    -1,
       8,    22,    -1,    10,    22,    -1,    12,    22,    -1,    13,
      22,    -1,    14,    22,    -1,    15,    22,    -1,    16,    22,
      -1,    -1,    55,    -1,    -1,    21,    -1,    53,    25,    21,
      -1,    56,    -1,    54,    56,    -1,    57,    -1,    55,    57,
      -1,    11,    21,    -1,    11,    23,    21,    24,    -1,    57,
      -1,    58,    -1,    19,    -1,    20,    -1,    21,    -1,    26,
      -1,    27,    -1,    28,    -1,    29,    -1,    23,    -1,    24,
      -1,    30,    -1,    31,    -1,    32,    -1,    33,    -1,    34,
      -1,    35,    -1,    36,    -1,    37,    -1,    38,    -1,    39,
      -1,    40,    -1,    41,    -1,    42,    -1,    43,    -1,    44,
      -1,    45,    -1,    25,    -1,    46,    -1
};


static const yytype_uint8 yyrline[] =
{
       0,    63,    63,    65,    69,    70,    74,    75,    84,    85,
      88,    91,    94,    97,   100,   103,   105,   107,   110,   111,
     112,   113,   114,   118,   119,   122,   123,   127,   133,   137,
     144,   148,   155,   157,   159,   163,   166,   169,   172,   178,
     179,   180,   181,   182,   183,   184,   185,   186,   187,   188,
     189,   190,   191,   192,   193,   194,   195,   196,   197,   198,
     199,   200,   201
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE


static const char *const yytname[] =
{
  "$end", "error", "$undefined", "HASH", "HASH_UNDEF", "HASH_IF",
  "HASH_IFDEF", "HASH_IFNDEF", "HASH_ELSE", "HASH_ELIF", "HASH_ENDIF",
  "DEFINED", "HASH_ERROR", "HASH_PRAGMA", "HASH_EXTENSION", "HASH_VERSION",
  "HASH_LINE", "HASH_DEFINE_OBJ", "HASH_DEFINE_FUNC", "INT_CONSTANT",
  "FLOAT_CONSTANT", "IDENTIFIER", "'\\n'", "'('", "')'", "','", "'['",
  "']'", "'<'", "'>'", "'{'", "'}'", "'.'", "'+'", "'-'", "'/'", "'*'",
  "'%'", "'^'", "'|'", "'&'", "'~'", "'='", "'!'", "':'", "';'", "'?'",
  "$accept", "input", "line", "text_line", "control_line",
  "replacement_list", "parameter_list", "conditional_list", "token_list",
  "conditional_token", "token", "operator", 0
};
#endif

# ifdef YYPRINT


static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,    10,    40,    41,    44,    91,    93,    60,    62,
     123,   125,    46,    43,    45,    47,    42,    37,    94,   124,
      38,   126,    61,    33,    58,    59,    63
};
# endif


static const yytype_uint8 yyr1[] =
{
       0,    47,    48,    48,    49,    49,    50,    50,    51,    51,
      51,    51,    51,    51,    51,    51,    51,    51,    51,    51,
      51,    51,    51,    52,    52,    53,    53,    53,    54,    54,
      55,    55,    56,    56,    56,    57,    57,    57,    57,    58,
      58,    58,    58,    58,    58,    58,    58,    58,    58,    58,
      58,    58,    58,    58,    58,    58,    58,    58,    58,    58,
      58,    58,    58
};


static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     2,     1,     1,     1,     2,     2,     3,
       6,     3,     3,     3,     3,     3,     2,     2,     2,     2,
       2,     2,     2,     0,     1,     0,     1,     3,     1,     2,
       1,     2,     2,     4,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1
};




static const yytype_uint8 yydefact[] =
{
       2,     0,     1,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    23,     0,    36,    37,
      38,     6,    43,    44,    61,    39,    40,    41,    42,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    62,     3,     4,     5,     0,
      30,    35,     8,     0,     0,     0,    28,    34,     0,     0,
      16,     0,    17,    18,    19,    20,    21,    22,     0,    24,
      25,     7,    31,    11,    32,     0,    12,    29,    13,    14,
      15,     9,    26,     0,     0,    23,     0,    33,     0,    27,
      10
};


static const yytype_int8 yydefgoto[] =
{
      -1,     1,    46,    47,    48,    68,    83,    55,    69,    56,
      57,    51
};



#define YYPACT_NINF -55
static const yytype_int16 yypact[] =
{
     -55,    73,   -55,   -17,   -18,   145,   -10,    -9,   -16,   145,
      -8,    22,    23,    24,    25,    27,   201,    28,   -55,   -55,
     -55,   -55,   -55,   -55,   -55,   -55,   -55,   -55,   -55,   -55,
     -55,   -55,   -55,   -55,   -55,   -55,   -55,   -55,   -55,   -55,
     -55,   -55,   -55,   -55,   -55,   -55,   -55,   -55,   -55,   173,
     -55,   -55,   -55,    30,   -19,    -3,   -55,   -55,    31,    32,
     -55,   109,   -55,   -55,   -55,   -55,   -55,   -55,    33,   201,
      29,   -55,   -55,   -55,   -55,    35,   -55,   -55,   -55,   -55,
     -55,   -55,   -55,   -15,   -11,   201,    36,   -55,    37,   -55,
     -55
};


static const yytype_int8 yypgoto[] =
{
     -55,   -55,   -55,   -55,   -55,   -27,   -55,    51,    60,   -54,
      -1,   -55
};





#define YYTABLE_NINF -1
static const yytype_uint8 yytable[] =
{
      50,    77,    74,    53,    75,    52,    60,    77,    54,    85,
      86,    58,    59,    87,    62,    50,    18,    19,    20,    76,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    63,    64,    65,    66,    72,    67,
      82,    70,    73,    78,    79,    81,    84,    89,    88,    90,
      61,    49,     0,     0,     0,     0,     0,     0,    72,     0,
       0,     0,     0,     2,     0,     0,     3,     4,     5,     6,
       7,     8,     9,    10,    50,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      54,     0,     0,     0,     0,     0,     0,     0,    18,    19,
      20,    80,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    54,     0,     0,     0,
       0,     0,     0,     0,    18,    19,    20,     0,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    18,    19,    20,    71,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      18,    19,    20,     0,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45
};

static const yytype_int8 yycheck[] =
{
       1,    55,    21,    21,    23,    22,    22,    61,    11,    24,
      25,    21,    21,    24,    22,    16,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    22,    22,    22,    22,    49,    22,
      21,    23,    22,    22,    22,    22,    21,    21,    85,    22,
       9,     1,    -1,    -1,    -1,    -1,    -1,    -1,    69,    -1,
      -1,    -1,    -1,     0,    -1,    -1,     3,     4,     5,     6,
       7,     8,     9,    10,    85,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      11,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    11,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    19,    20,    21,    -1,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      19,    20,    21,    -1,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46
};



static const yytype_uint8 yystos[] =
{
       0,    48,     0,     3,     4,     5,     6,     7,     8,     9,
      10,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    49,    50,    51,    55,
      57,    58,    22,    21,    11,    54,    56,    57,    21,    21,
      22,    54,    22,    22,    22,    22,    22,    22,    52,    55,
      23,    22,    57,    22,    21,    23,    22,    56,    22,    22,
      22,    22,    21,    53,    21,    24,    25,    24,    52,    21,
      22
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
      yyerror (&yylloc, context, YY_("syntax error: cannot back up")); \
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
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, pp::Context* context)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp, context)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
    pp::Context* context;
#endif
{
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
  switch (yytype)
    {
      default:
	break;
    }
}






#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, pp::Context* context)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep, yylocationp, context)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
    pp::Context* context;
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
yy_stack_print (yytype_int16 *bottom, yytype_int16 *top)
#else
static void
yy_stack_print (bottom, top)
    yytype_int16 *bottom;
    yytype_int16 *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
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
yy_reduce_print (YYSTYPE *yyvsp, YYLTYPE *yylsp, int yyrule, pp::Context* context)
#else
static void
yy_reduce_print (yyvsp, yylsp, yyrule, context)
    YYSTYPE *yyvsp;
    YYLTYPE *yylsp;
    int yyrule;
    pp::Context* context;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      fprintf (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       , &(yylsp[(yyi + 1) - (yynrhs)])		       , context);
      fprintf (stderr, "\n");
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
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, YYLTYPE *yylocationp, pp::Context* context)
#else
static void
yydestruct (yymsg, yytype, yyvaluep, yylocationp, context)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
    YYLTYPE *yylocationp;
    pp::Context* context;
#endif
{
  YYUSE (yyvaluep);
  YYUSE (yylocationp);
  YYUSE (context);

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
int yyparse (pp::Context* context);
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
yyparse (pp::Context* context)
#else
int
yyparse (context)
    pp::Context* context;
#endif
#endif
{
  
int yychar;


YYSTYPE yylval;


int yynerrs;

YYLTYPE yylloc;

  int yystate;
  int yyn;
  int yyresult;
  
  int yyerrstatus;
  
  int yytoken = 0;
#if YYERROR_VERBOSE
  
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

  







  
  yytype_int16 yyssa[YYINITDEPTH];
  yytype_int16 *yyss = yyssa;
  yytype_int16 *yyssp;

  
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;

  
  YYLTYPE yylsa[YYINITDEPTH];
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;
  
  YYLTYPE yyerror_range[2];

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  YYSIZE_T yystacksize = YYINITDEPTH;

  

  YYSTYPE yyval;
  YYLTYPE yyloc;

  

  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		

  




  yyssp = yyss;
  yyvsp = yyvs;
  yylsp = yyls;
#if YYLTYPE_IS_TRIVIAL
  
  yylloc.first_line   = yylloc.last_line   = 1;
  yylloc.first_column = yylloc.last_column = 0;
#endif

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
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);
	YYSTACK_RELOCATE (yyls);
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

  if (yyn == YYFINAL)
    YYACCEPT;

  

  if (yyerrstatus)
    yyerrstatus--;

  
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;
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
        case 7:

    {
        
        pp::TokenVector* out = context->output();
        out->insert(out->end(), (yyvsp[(1) - (2)].tlist)->begin(), (yyvsp[(1) - (2)].tlist)->end());
        delete (yyvsp[(1) - (2)].tlist);
    ;}
    break;

  case 9:

    {
        context->defineMacro((yylsp[(1) - (3)]).first_line, pp::Macro::kTypeObj, (yyvsp[(1) - (3)].sval), NULL, (yyvsp[(2) - (3)].tlist));
    ;}
    break;

  case 10:

    {
        context->defineMacro((yylsp[(1) - (6)]).first_line, pp::Macro::kTypeFunc, (yyvsp[(1) - (6)].sval), (yyvsp[(3) - (6)].tlist), (yyvsp[(5) - (6)].tlist));
    ;}
    break;

  case 11:

    {
        context->undefineMacro((yyvsp[(2) - (3)].sval));
    ;}
    break;

  case 12:

    {
        pushConditionalBlock(context, (yyvsp[(2) - (3)].tlist) ? true : false);
    ;}
    break;

  case 13:

    {
        pushConditionalBlock(context, context->isMacroDefined((yyvsp[(2) - (3)].sval)));
    ;}
    break;

  case 14:

    {
        pushConditionalBlock(context, !context->isMacroDefined((yyvsp[(2) - (3)].sval)));
    ;}
    break;

  case 15:

    {
    ;}
    break;

  case 16:

    {
    ;}
    break;

  case 17:

    {
        popConditionalBlock(context);
    ;}
    break;

  case 23:

    { (yyval.tlist) = NULL; ;}
    break;

  case 25:

    { (yyval.tlist) = NULL; ;}
    break;

  case 26:

    {
        (yyval.tlist) = new pp::TokenVector;
        (yyval.tlist)->push_back(new pp::Token((yylsp[(1) - (1)]).first_line, IDENTIFIER, (yyvsp[(1) - (1)].sval)));
    ;}
    break;

  case 27:

    {
        (yyval.tlist) = (yyvsp[(1) - (3)].tlist);
        (yyval.tlist)->push_back(new pp::Token((yylsp[(3) - (3)]).first_line, IDENTIFIER, (yyvsp[(3) - (3)].sval)));
    ;}
    break;

  case 28:

    {
        (yyval.tlist) = new pp::TokenVector;
        (yyval.tlist)->push_back((yyvsp[(1) - (1)].tval));
    ;}
    break;

  case 29:

    {
        (yyval.tlist) = (yyvsp[(1) - (2)].tlist);
        (yyval.tlist)->push_back((yyvsp[(2) - (2)].tval));
    ;}
    break;

  case 30:

    {
        (yyval.tlist) = new pp::TokenVector;
        (yyval.tlist)->push_back((yyvsp[(1) - (1)].tval));
    ;}
    break;

  case 31:

    {
        (yyval.tlist) = (yyvsp[(1) - (2)].tlist);
        (yyval.tlist)->push_back((yyvsp[(2) - (2)].tval));
    ;}
    break;

  case 32:

    {
    ;}
    break;

  case 33:

    {
    ;}
    break;

  case 35:

    {
        (yyval.tval) = new pp::Token((yylsp[(1) - (1)]).first_line, (yyvsp[(1) - (1)].ival), NULL);
    ;}
    break;

  case 36:

    {
        (yyval.tval) = new pp::Token((yylsp[(1) - (1)]).first_line, INT_CONSTANT, (yyvsp[(1) - (1)].sval));
    ;}
    break;

  case 37:

    {
        (yyval.tval) = new pp::Token((yylsp[(1) - (1)]).first_line, FLOAT_CONSTANT, (yyvsp[(1) - (1)].sval));
    ;}
    break;

  case 38:

    {
        (yyval.tval) = new pp::Token((yylsp[(1) - (1)]).first_line, IDENTIFIER, (yyvsp[(1) - (1)].sval));
    ;}
    break;

  case 39:

    { (yyval.ival) = '['; ;}
    break;

  case 40:

    { (yyval.ival) = ']'; ;}
    break;

  case 41:

    { (yyval.ival) = '<'; ;}
    break;

  case 42:

    { (yyval.ival) = '>'; ;}
    break;

  case 43:

    { (yyval.ival) = '('; ;}
    break;

  case 44:

    { (yyval.ival) = ')'; ;}
    break;

  case 45:

    { (yyval.ival) = '{'; ;}
    break;

  case 46:

    { (yyval.ival) = '}'; ;}
    break;

  case 47:

    { (yyval.ival) = '.'; ;}
    break;

  case 48:

    { (yyval.ival) = '+'; ;}
    break;

  case 49:

    { (yyval.ival) = '-'; ;}
    break;

  case 50:

    { (yyval.ival) = '/'; ;}
    break;

  case 51:

    { (yyval.ival) = '*'; ;}
    break;

  case 52:

    { (yyval.ival) = '%'; ;}
    break;

  case 53:

    { (yyval.ival) = '^'; ;}
    break;

  case 54:

    { (yyval.ival) = '|'; ;}
    break;

  case 55:

    { (yyval.ival) = '&'; ;}
    break;

  case 56:

    { (yyval.ival) = '~'; ;}
    break;

  case 57:

    { (yyval.ival) = '='; ;}
    break;

  case 58:

    { (yyval.ival) = '!'; ;}
    break;

  case 59:

    { (yyval.ival) = ':'; ;}
    break;

  case 60:

    { (yyval.ival) = ';'; ;}
    break;

  case 61:

    { (yyval.ival) = ','; ;}
    break;

  case 62:

    { (yyval.ival) = '?'; ;}
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
  
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (&yylloc, context, YY_("syntax error"));
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
	    yyerror (&yylloc, context, yymsg);
	  }
	else
	  {
	    yyerror (&yylloc, context, YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }

  yyerror_range[0] = yylloc;

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

  yyerror_range[0] = yylsp[1-yylen];
  

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

      yyerror_range[0] = *yylsp;
      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp, yylsp, context);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;

  yyerror_range[1] = yylloc;
  

  YYLLOC_DEFAULT (yyloc, (yyerror_range - 1), 2);
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

#ifndef yyoverflow



yyexhaustedlab:
  yyerror (&yylloc, context, YY_("memory exhausted"));
  yyresult = 2;
  
#endif

yyreturn:
  if (yychar != YYEOF && yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval, &yylloc, context);
  

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





void yyerror(YYLTYPE* llocp, pp::Context* context, const char* reason)
{
}

void pushConditionalBlock(pp::Context* context, bool condition)
{
}

void popConditionalBlock(pp::Context* context)
{
}

namespace pp {
bool Context::parse()
{
    yydebug = 1;
    return yyparse(this) == 0 ? true : false;
}
}  


