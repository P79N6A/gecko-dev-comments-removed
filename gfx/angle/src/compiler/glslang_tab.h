



































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
     IDENTIFIER = 299,
     TYPE_NAME = 300,
     FLOATCONSTANT = 301,
     INTCONSTANT = 302,
     BOOLCONSTANT = 303,
     FIELD_SELECTION = 304,
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

#define INVARIANT 258
#define HIGH_PRECISION 259
#define MEDIUM_PRECISION 260
#define LOW_PRECISION 261
#define PRECISION 262
#define ATTRIBUTE 263
#define CONST_QUAL 264
#define BOOL_TYPE 265
#define FLOAT_TYPE 266
#define INT_TYPE 267
#define BREAK 268
#define CONTINUE 269
#define DO 270
#define ELSE 271
#define FOR 272
#define IF 273
#define DISCARD 274
#define RETURN 275
#define BVEC2 276
#define BVEC3 277
#define BVEC4 278
#define IVEC2 279
#define IVEC3 280
#define IVEC4 281
#define VEC2 282
#define VEC3 283
#define VEC4 284
#define MATRIX2 285
#define MATRIX3 286
#define MATRIX4 287
#define IN_QUAL 288
#define OUT_QUAL 289
#define INOUT_QUAL 290
#define UNIFORM 291
#define VARYING 292
#define STRUCT 293
#define VOID_TYPE 294
#define WHILE 295
#define SAMPLER2D 296
#define SAMPLERCUBE 297
#define SAMPLER_EXTERNAL_OES 298
#define IDENTIFIER 299
#define TYPE_NAME 300
#define FLOATCONSTANT 301
#define INTCONSTANT 302
#define BOOLCONSTANT 303
#define FIELD_SELECTION 304
#define LEFT_OP 305
#define RIGHT_OP 306
#define INC_OP 307
#define DEC_OP 308
#define LE_OP 309
#define GE_OP 310
#define EQ_OP 311
#define NE_OP 312
#define AND_OP 313
#define OR_OP 314
#define XOR_OP 315
#define MUL_ASSIGN 316
#define DIV_ASSIGN 317
#define ADD_ASSIGN 318
#define MOD_ASSIGN 319
#define LEFT_ASSIGN 320
#define RIGHT_ASSIGN 321
#define AND_ASSIGN 322
#define XOR_ASSIGN 323
#define OR_ASSIGN 324
#define SUB_ASSIGN 325
#define LEFT_PAREN 326
#define RIGHT_PAREN 327
#define LEFT_BRACKET 328
#define RIGHT_BRACKET 329
#define LEFT_BRACE 330
#define RIGHT_BRACE 331
#define DOT 332
#define COMMA 333
#define COLON 334
#define EQUAL 335
#define SEMICOLON 336
#define BANG 337
#define DASH 338
#define TILDE 339
#define PLUS 340
#define STAR 341
#define SLASH 342
#define PERCENT 343
#define LEFT_ANGLE 344
#define RIGHT_ANGLE 345
#define VERTICAL_BAR 346
#define CARET 347
#define AMPERSAND 348
#define QUESTION 349




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
}


	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



