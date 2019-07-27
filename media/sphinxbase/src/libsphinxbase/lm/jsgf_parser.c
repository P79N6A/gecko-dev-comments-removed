












































#define YYBISON 1


#define YYBISON_VERSION "2.4.1"


#define YYSKELETON_NAME "yacc.c"


#define YYPURE 1


#define YYPUSH 0


#define YYPULL 1


#define YYLSP_NEEDED 0






#line 37 "jsgf_parser.y"

#define YYERROR_VERBOSE

#include <stdio.h>
#include <string.h>

#include <sphinxbase/hash_table.h>
#include <sphinxbase/ckd_alloc.h>
#include <sphinxbase/err.h>

#include "jsgf_internal.h"
#include "jsgf_parser.h"
#include "jsgf_scanner.h"


#if defined _MSC_VER
#pragma warning(disable: 4273)
#endif

void yyerror(yyscan_t lex, jsgf_t *jsgf, const char *s);




#line 97 "jsgf_parser.c"


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




#line 163 "jsgf_parser.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif






#line 175 "jsgf_parser.c"

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


#define YYFINAL  7

#define YYLAST   54


#define YYNTOKENS  20

#define YYNNTS  16

#define YYNRULES  33

#define YYNSTATES  58


#define YYUNDEFTOK  2
#define YYMAXUTOK   265

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)


static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      14,    15,    18,    19,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,    11,
       2,    12,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    16,     2,    17,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    13,     2,     2,     2,     2,     2,
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
       5,     6,     7,     8,     9,    10
};

#if YYDEBUG


static const yytype_uint8 yyprhs[] =
{
       0,     0,     3,     5,     8,    12,    15,    18,    22,    27,
      33,    37,    39,    42,    46,    48,    51,    56,    62,    64,
      68,    70,    73,    75,    78,    80,    83,    87,    91,    93,
      95,    97,    99,   102
};


static const yytype_int8 yyrhs[] =
{
      21,     0,    -1,    22,    -1,    22,    27,    -1,    22,    25,
      27,    -1,    23,    24,    -1,     3,    11,    -1,     3,     7,
      11,    -1,     3,     7,     7,    11,    -1,     3,     7,     7,
       7,    11,    -1,     4,     7,    11,    -1,    26,    -1,    25,
      26,    -1,     5,     8,    11,    -1,    28,    -1,    27,    28,
      -1,     8,    12,    29,    11,    -1,     6,     8,    12,    29,
      11,    -1,    30,    -1,    29,    13,    30,    -1,    31,    -1,
      30,    31,    -1,    32,    -1,    31,     9,    -1,    35,    -1,
      10,    35,    -1,    14,    29,    15,    -1,    16,    29,    17,
      -1,     7,    -1,     8,    -1,    33,    -1,    34,    -1,    35,
      18,    -1,    35,    19,    -1
};


static const yytype_uint8 yyrline[] =
{
       0,    82,    82,    83,    84,    87,    90,    91,    92,    93,
      97,   100,   101,   104,   107,   108,   111,   112,   115,   116,
     121,   123,   127,   128,   132,   133,   136,   139,   142,   143,
     144,   145,   146,   147
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE


static const char *const yytname[] =
{
  "$end", "error", "$undefined", "HEADER", "GRAMMAR", "IMPORT", "PUBLIC",
  "TOKEN", "RULENAME", "TAG", "WEIGHT", "';'", "'='", "'|'", "'('", "')'",
  "'['", "']'", "'*'", "'+'", "$accept", "grammar", "header",
  "jsgf_header", "grammar_header", "import_header", "import_statement",
  "rule_list", "rule", "alternate_list", "rule_expansion",
  "tagged_rule_item", "rule_item", "rule_group", "rule_optional",
  "rule_atom", 0
};
#endif

# ifdef YYPRINT


static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,    59,    61,   124,    40,    41,    91,    93,    42,    43
};
# endif


static const yytype_uint8 yyr1[] =
{
       0,    20,    21,    21,    21,    22,    23,    23,    23,    23,
      24,    25,    25,    26,    27,    27,    28,    28,    29,    29,
      30,    30,    31,    31,    32,    32,    33,    34,    35,    35,
      35,    35,    35,    35
};


static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     2,     3,     2,     2,     3,     4,     5,
       3,     1,     2,     3,     1,     2,     4,     5,     1,     3,
       1,     2,     1,     2,     1,     2,     3,     3,     1,     1,
       1,     1,     2,     2
};




static const yytype_uint8 yydefact[] =
{
       0,     0,     0,     2,     0,     0,     6,     1,     0,     0,
       0,     0,    11,     3,    14,     0,     5,     0,     7,     0,
       0,     0,    12,     4,    15,     0,     0,     8,    13,     0,
      28,    29,     0,     0,     0,     0,    18,    20,    22,    30,
      31,    24,    10,     9,     0,    25,     0,     0,    16,     0,
      21,    23,    32,    33,    17,    26,    27,    19
};


static const yytype_int8 yydefgoto[] =
{
      -1,     2,     3,     4,    16,    11,    12,    13,    14,    35,
      36,    37,    38,    39,    40,    41
};



#define YYPACT_NINF -37
static const yytype_int8 yypact[] =
{
      -1,    -2,    36,    22,    35,     8,   -37,   -37,    32,    33,
      30,    22,   -37,    17,   -37,    37,   -37,    13,   -37,    34,
      31,    -4,   -37,    17,   -37,    38,    39,   -37,   -37,    -4,
     -37,   -37,     0,    -4,    -4,    18,    -4,    42,   -37,   -37,
     -37,    19,   -37,   -37,    21,    19,    20,     9,   -37,    -4,
      42,   -37,   -37,   -37,   -37,   -37,   -37,    -4
};


static const yytype_int8 yypgoto[] =
{
     -37,   -37,   -37,   -37,   -37,   -37,    41,    43,   -12,   -16,
      -3,   -36,   -37,   -37,   -37,    15
};





#define YYTABLE_NINF -1
static const yytype_uint8 yytable[] =
{
      50,    24,     1,    30,    31,     5,    32,    30,    31,     6,
      33,    24,    34,    44,    33,    17,    34,    46,    47,    18,
      26,    50,    49,     9,    27,    10,    56,     8,     9,    48,
      10,    49,    54,    49,    49,    55,     7,    52,    53,    15,
      19,    20,    21,    29,    25,    28,    57,    45,     0,    42,
      43,    51,    22,     0,    23
};

static const yytype_int8 yycheck[] =
{
      36,    13,     3,     7,     8,     7,    10,     7,     8,    11,
      14,    23,    16,    29,    14,     7,    16,    33,    34,    11,
       7,    57,    13,     6,    11,     8,    17,     5,     6,    11,
       8,    13,    11,    13,    13,    15,     0,    18,    19,     4,
       8,     8,    12,    12,     7,    11,    49,    32,    -1,    11,
      11,     9,    11,    -1,    11
};



static const yytype_uint8 yystos[] =
{
       0,     3,    21,    22,    23,     7,    11,     0,     5,     6,
       8,    25,    26,    27,    28,     4,    24,     7,    11,     8,
       8,    12,    26,    27,    28,     7,     7,    11,    11,    12,
       7,     8,    10,    14,    16,    29,    30,    31,    32,    33,
      34,    35,    11,    11,    29,    35,    29,    29,    11,    13,
      31,     9,    18,    19,    11,    15,    17,    30
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
      yyerror (yyscanner, jsgf, YY_("syntax error: cannot back up")); \
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
# define YYLEX yylex (&yylval, yyscanner)
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
		  Type, Value, yyscanner, jsgf); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))







#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, void* yyscanner, jsgf_t *jsgf)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep, yyscanner, jsgf)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    void* yyscanner;
    jsgf_t *jsgf;
#endif
{
  if (!yyvaluep)
    return;
  YYUSE (yyscanner);
  YYUSE (jsgf);
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
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, void* yyscanner, jsgf_t *jsgf)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep, yyscanner, jsgf)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    void* yyscanner;
    jsgf_t *jsgf;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep, yyscanner, jsgf);
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
yy_reduce_print (YYSTYPE *yyvsp, int yyrule, void* yyscanner, jsgf_t *jsgf)
#else
static void
yy_reduce_print (yyvsp, yyrule, yyscanner, jsgf)
    YYSTYPE *yyvsp;
    int yyrule;
    void* yyscanner;
    jsgf_t *jsgf;
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
		       		       , yyscanner, jsgf);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule, yyscanner, jsgf); \
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
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, void* yyscanner, jsgf_t *jsgf)
#else
static void
yydestruct (yymsg, yytype, yyvaluep, yyscanner, jsgf)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
    void* yyscanner;
    jsgf_t *jsgf;
#endif
{
  YYUSE (yyvaluep);
  YYUSE (yyscanner);
  YYUSE (jsgf);

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
int yyparse (void* yyscanner, jsgf_t *jsgf);
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
yyparse (void* yyscanner, jsgf_t *jsgf)
#else
int
yyparse (yyscanner, jsgf)
    void* yyscanner;
    jsgf_t *jsgf;
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
        case 5:


#line 87 "jsgf_parser.y"
    { jsgf->name = (yyvsp[(2) - (2)].name); }
    break;

  case 7:


#line 91 "jsgf_parser.y"
    { jsgf->version = (yyvsp[(2) - (3)].name); }
    break;

  case 8:


#line 92 "jsgf_parser.y"
    { jsgf->version = (yyvsp[(2) - (4)].name); jsgf->charset = (yyvsp[(3) - (4)].name); }
    break;

  case 9:


#line 93 "jsgf_parser.y"
    { jsgf->version = (yyvsp[(2) - (5)].name); jsgf->charset = (yyvsp[(3) - (5)].name);
					 jsgf->locale = (yyvsp[(4) - (5)].name); }
    break;

  case 10:


#line 97 "jsgf_parser.y"
    { (yyval.name) = (yyvsp[(2) - (3)].name); }
    break;

  case 13:


#line 104 "jsgf_parser.y"
    { jsgf_import_rule(jsgf, (yyvsp[(2) - (3)].name)); ckd_free((yyvsp[(2) - (3)].name)); }
    break;

  case 16:


#line 111 "jsgf_parser.y"
    { jsgf_define_rule(jsgf, (yyvsp[(1) - (4)].name), (yyvsp[(3) - (4)].rhs), 0); ckd_free((yyvsp[(1) - (4)].name)); }
    break;

  case 17:


#line 112 "jsgf_parser.y"
    { jsgf_define_rule(jsgf, (yyvsp[(2) - (5)].name), (yyvsp[(4) - (5)].rhs), 1); ckd_free((yyvsp[(2) - (5)].name)); }
    break;

  case 18:


#line 115 "jsgf_parser.y"
    { (yyval.rhs) = (yyvsp[(1) - (1)].rhs); (yyval.rhs)->atoms = glist_reverse((yyval.rhs)->atoms); }
    break;

  case 19:


#line 116 "jsgf_parser.y"
    { (yyval.rhs) = (yyvsp[(3) - (3)].rhs);
                                              (yyval.rhs)->atoms = glist_reverse((yyval.rhs)->atoms);
                                              (yyval.rhs)->alt = (yyvsp[(1) - (3)].rhs); }
    break;

  case 20:


#line 121 "jsgf_parser.y"
    { (yyval.rhs) = ckd_calloc(1, sizeof(*(yyval.rhs)));
				   (yyval.rhs)->atoms = glist_add_ptr((yyval.rhs)->atoms, (yyvsp[(1) - (1)].atom)); }
    break;

  case 21:


#line 123 "jsgf_parser.y"
    { (yyval.rhs) = (yyvsp[(1) - (2)].rhs);
					    (yyval.rhs)->atoms = glist_add_ptr((yyval.rhs)->atoms, (yyvsp[(2) - (2)].atom)); }
    break;

  case 23:


#line 128 "jsgf_parser.y"
    { (yyval.atom) = (yyvsp[(1) - (2)].atom);
				 (yyval.atom)->tags = glist_add_ptr((yyval.atom)->tags, (yyvsp[(2) - (2)].name)); }
    break;

  case 25:


#line 133 "jsgf_parser.y"
    { (yyval.atom) = (yyvsp[(2) - (2)].atom); (yyval.atom)->weight = (yyvsp[(1) - (2)].weight); }
    break;

  case 26:


#line 136 "jsgf_parser.y"
    { (yyval.rule) = jsgf_define_rule(jsgf, NULL, (yyvsp[(2) - (3)].rhs), 0); }
    break;

  case 27:


#line 139 "jsgf_parser.y"
    { (yyval.rule) = jsgf_optional_new(jsgf, (yyvsp[(2) - (3)].rhs)); }
    break;

  case 28:


#line 142 "jsgf_parser.y"
    { (yyval.atom) = jsgf_atom_new((yyvsp[(1) - (1)].name), 1.0); ckd_free((yyvsp[(1) - (1)].name)); }
    break;

  case 29:


#line 143 "jsgf_parser.y"
    { (yyval.atom) = jsgf_atom_new((yyvsp[(1) - (1)].name), 1.0); ckd_free((yyvsp[(1) - (1)].name)); }
    break;

  case 30:


#line 144 "jsgf_parser.y"
    { (yyval.atom) = jsgf_atom_new((yyvsp[(1) - (1)].rule)->name, 1.0); }
    break;

  case 31:


#line 145 "jsgf_parser.y"
    { (yyval.atom) = jsgf_atom_new((yyvsp[(1) - (1)].rule)->name, 1.0); }
    break;

  case 32:


#line 146 "jsgf_parser.y"
    { (yyval.atom) = jsgf_kleene_new(jsgf, (yyvsp[(1) - (2)].atom), 0); }
    break;

  case 33:


#line 147 "jsgf_parser.y"
    { (yyval.atom) = jsgf_kleene_new(jsgf, (yyvsp[(1) - (2)].atom), 1); }
    break;




#line 1580 "jsgf_parser.c"
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
      yyerror (yyscanner, jsgf, YY_("syntax error"));
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
	    yyerror (yyscanner, jsgf, yymsg);
	  }
	else
	  {
	    yyerror (yyscanner, jsgf, YY_("syntax error"));
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
		      yytoken, &yylval, yyscanner, jsgf);
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
		  yystos[yystate], yyvsp, yyscanner, jsgf);
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
  yyerror (yyscanner, jsgf, YY_("memory exhausted"));
  yyresult = 2;
  
#endif

yyreturn:
  if (yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval, yyscanner, jsgf);
  

  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp, yyscanner, jsgf);
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




#line 150 "jsgf_parser.y"


void
yyerror(yyscan_t lex, jsgf_t *jsgf, const char *s)
{
    E_ERROR("%s at line %d current token '%s'\n", s, yyget_lineno(lex), yyget_text(lex));
}

