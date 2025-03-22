/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.3"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Using locations.  */
#define YYLSP_NEEDED 0

/* Substitute the variable and function names.  */
#define yyparse expr_yyparse
#define yylex   expr_yylex
#define yyerror expr_yyerror
#define yylval  expr_yylval
#define yychar  expr_yychar
#define yydebug expr_yydebug
#define yynerrs expr_yynerrs


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     NULL_CONST = 258,
     INTEGER_CONST = 259,
     MAXINT_PLUS_ONE_CONST = 260,
     DOUBLE_CONST = 261,
     BOOLEAN_CONST = 262,
     VARIABLE = 263,
     FUNCTION = 264,
     AND_OP = 265,
     OR_OP = 266,
     NOT_OP = 267,
     NE_OP = 268,
     LE_OP = 269,
     GE_OP = 270,
     LS_OP = 271,
     RS_OP = 272,
     IS_OP = 273,
     CASE_KW = 274,
     WHEN_KW = 275,
     THEN_KW = 276,
     ELSE_KW = 277,
     END_KW = 278,
     NOTNULL_OP = 279,
     ISNULL_OP = 280,
     UNARY = 281
   };
#endif
/* Tokens.  */
#define NULL_CONST 258
#define INTEGER_CONST 259
#define MAXINT_PLUS_ONE_CONST 260
#define DOUBLE_CONST 261
#define BOOLEAN_CONST 262
#define VARIABLE 263
#define FUNCTION 264
#define AND_OP 265
#define OR_OP 266
#define NOT_OP 267
#define NE_OP 268
#define LE_OP 269
#define GE_OP 270
#define LS_OP 271
#define RS_OP 272
#define IS_OP 273
#define CASE_KW 274
#define WHEN_KW 275
#define THEN_KW 276
#define ELSE_KW 277
#define END_KW 278
#define NOTNULL_OP 279
#define ISNULL_OP 280
#define UNARY 281




/* Copy the first part of user declarations.  */
#line 1 "/Users/rushilbhat/Documents/CS348/postgres/build/../src/bin/pgbench/exprparse.y"

/*-------------------------------------------------------------------------
 *
 * exprparse.y
 *	  bison grammar for a simple expression syntax
 *
 * Portions Copyright (c) 1996-2025, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/bin/pgbench/exprparse.y
 *
 *-------------------------------------------------------------------------
 */

#include "postgres_fe.h"

#include "pgbench.h"

#define PGBENCH_NARGS_VARIABLE	(-1)
#define PGBENCH_NARGS_CASE		(-2)
#define PGBENCH_NARGS_HASH		(-3)
#define PGBENCH_NARGS_PERMUTE	(-4)

static PgBenchExprList *make_elist(PgBenchExpr *expr, PgBenchExprList *list);
static PgBenchExpr *make_null_constant(void);
static PgBenchExpr *make_boolean_constant(bool bval);
static PgBenchExpr *make_integer_constant(int64 ival);
static PgBenchExpr *make_double_constant(double dval);
static PgBenchExpr *make_variable(char *varname);
static PgBenchExpr *make_op(yyscan_t yyscanner, const char *operator,
							PgBenchExpr *lexpr, PgBenchExpr *rexpr);
static PgBenchExpr *make_uop(yyscan_t yyscanner, const char *operator, PgBenchExpr *expr);
static int	find_func(yyscan_t yyscanner, const char *fname);
static PgBenchExpr *make_func(yyscan_t yyscanner, int fnumber, PgBenchExprList *args);
static PgBenchExpr *make_case(yyscan_t yyscanner, PgBenchExprList *when_then_list, PgBenchExpr *else_part);



/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif

#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 48 "/Users/rushilbhat/Documents/CS348/postgres/build/../src/bin/pgbench/exprparse.y"
{
	int64		ival;
	double		dval;
	bool		bval;
	char	   *str;
	PgBenchExpr *expr;
	PgBenchExprList *elist;
}
/* Line 193 of yacc.c.  */
#line 203 "exprparse.c"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 216 of yacc.c.  */
#line 216 "exprparse.c"

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
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
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

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
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
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
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

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
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

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  25
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   320

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  42
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  7
/* YYNRULES -- Number of rules.  */
#define YYNRULES  47
/* YYNRULES -- Number of states.  */
#define YYNSTATES  88

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   281

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,    30,     2,    37,    31,     2,
      40,    41,    35,    33,    39,    34,     2,    36,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      26,    28,    27,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    29,     2,    32,     2,     2,     2,
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
      25,    38
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint8 yyprhs[] =
{
       0,     0,     3,     5,     6,     8,    12,    16,    19,    22,
      25,    28,    31,    35,    39,    43,    47,    51,    55,    59,
      63,    67,    71,    75,    79,    83,    87,    91,    95,    99,
     103,   106,   109,   113,   118,   122,   127,   129,   131,   133,
     135,   137,   142,   144,   150,   155,   159,   165
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      43,     0,    -1,    45,    -1,    -1,    45,    -1,    44,    39,
      45,    -1,    40,    45,    41,    -1,    33,    45,    -1,    34,
      45,    -1,    34,     5,    -1,    32,    45,    -1,    12,    45,
      -1,    45,    33,    45,    -1,    45,    34,    45,    -1,    45,
      35,    45,    -1,    45,    36,    45,    -1,    45,    37,    45,
      -1,    45,    26,    45,    -1,    45,    14,    45,    -1,    45,
      27,    45,    -1,    45,    15,    45,    -1,    45,    28,    45,
      -1,    45,    13,    45,    -1,    45,    31,    45,    -1,    45,
      29,    45,    -1,    45,    30,    45,    -1,    45,    16,    45,
      -1,    45,    17,    45,    -1,    45,    10,    45,    -1,    45,
      11,    45,    -1,    45,    25,    -1,    45,    24,    -1,    45,
      18,     3,    -1,    45,    18,    12,     3,    -1,    45,    18,
       7,    -1,    45,    18,    12,     7,    -1,     3,    -1,     7,
      -1,     4,    -1,     6,    -1,     8,    -1,    48,    40,    44,
      41,    -1,    47,    -1,    46,    20,    45,    21,    45,    -1,
      20,    45,    21,    45,    -1,    19,    46,    23,    -1,    19,
      46,    22,    45,    23,    -1,     9,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint8 yyrline[] =
{
       0,    82,    82,    87,    88,    89,    92,    93,    95,    98,
     101,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,   119,   120,   121,
     123,   124,   128,   129,   134,   138,   144,   145,   146,   147,
     149,   150,   151,   155,   156,   159,   160,   162
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "NULL_CONST", "INTEGER_CONST",
  "MAXINT_PLUS_ONE_CONST", "DOUBLE_CONST", "BOOLEAN_CONST", "VARIABLE",
  "FUNCTION", "AND_OP", "OR_OP", "NOT_OP", "NE_OP", "LE_OP", "GE_OP",
  "LS_OP", "RS_OP", "IS_OP", "CASE_KW", "WHEN_KW", "THEN_KW", "ELSE_KW",
  "END_KW", "NOTNULL_OP", "ISNULL_OP", "'<'", "'>'", "'='", "'|'", "'#'",
  "'&'", "'~'", "'+'", "'-'", "'*'", "'/'", "'%'", "UNARY", "','", "'('",
  "')'", "$accept", "result", "elist", "expr", "when_then_list",
  "case_control", "function", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,    60,    62,    61,   124,
      35,    38,   126,    43,    45,    42,    47,    37,   281,    44,
      40,    41
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    42,    43,    44,    44,    44,    45,    45,    45,    45,
      45,    45,    45,    45,    45,    45,    45,    45,    45,    45,
      45,    45,    45,    45,    45,    45,    45,    45,    45,    45,
      45,    45,    45,    45,    45,    45,    45,    45,    45,    45,
      45,    45,    45,    46,    46,    47,    47,    48
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     0,     1,     3,     3,     2,     2,     2,
       2,     2,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       2,     2,     3,     4,     3,     4,     1,     1,     1,     1,
       1,     4,     1,     5,     4,     3,     5,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,    36,    38,    39,    37,    40,    47,     0,     0,     0,
       0,     0,     0,     0,     2,    42,     0,    11,     0,     0,
      10,     7,     9,     8,     0,     1,     0,     0,     0,     0,
       0,     0,     0,     0,    31,    30,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     3,     0,     0,
       0,    45,     6,    28,    29,    22,    18,    20,    26,    27,
      32,    34,     0,    17,    19,    21,    24,    25,    23,    12,
      13,    14,    15,    16,     0,     4,     0,     0,     0,    33,
      35,     0,    41,    44,     0,    46,     5,    43
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,    13,    74,    14,    19,    15,    16
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -33
static const yytype_int16 yypact[] =
{
      64,   -33,   -33,   -33,   -33,   -33,   -33,    64,   -19,    64,
      64,    46,    64,    13,   205,   -33,   -22,   258,    64,    -6,
      11,   -33,   -33,   -33,    92,   -33,    64,    64,    64,    64,
      64,    64,    64,     3,   -33,   -33,    64,    64,    64,    64,
      64,    64,    64,    64,    64,    64,    64,    64,   121,    64,
      64,   -33,   -33,   258,   233,   283,   283,   283,    11,    11,
     -33,   -33,     5,   283,   283,   283,    11,    11,    11,    -9,
      -9,   -33,   -33,   -33,   -32,   205,    64,   149,   177,   -33,
     -33,    64,   -33,   205,    64,   -33,   205,   205
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -33,   -33,   -33,    -7,   -33,   -33,   -33
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_int8 yytable[] =
{
      17,    18,    20,    21,    23,    24,    60,    81,    79,    82,
      61,    48,    80,    25,    49,    62,    50,    51,    47,    53,
      54,    55,    56,    57,    58,    59,    44,    45,    46,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      75,     0,    77,    78,    42,    43,    44,    45,    46,     1,
       2,    22,     3,     4,     5,     6,     0,     0,     7,     0,
       0,     0,     0,     0,     0,     8,     0,     1,     2,    83,
       3,     4,     5,     6,    86,     0,     7,    87,     9,    10,
      11,     0,     0,     8,     0,     0,    12,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     9,    10,    11,     0,
       0,     0,    26,    27,    12,    28,    29,    30,    31,    32,
      33,     0,     0,     0,     0,     0,    34,    35,    36,    37,
      38,    39,    40,    41,     0,    42,    43,    44,    45,    46,
       0,    26,    27,    52,    28,    29,    30,    31,    32,    33,
       0,     0,    76,     0,     0,    34,    35,    36,    37,    38,
      39,    40,    41,     0,    42,    43,    44,    45,    46,    26,
      27,     0,    28,    29,    30,    31,    32,    33,     0,     0,
      84,     0,     0,    34,    35,    36,    37,    38,    39,    40,
      41,     0,    42,    43,    44,    45,    46,    26,    27,     0,
      28,    29,    30,    31,    32,    33,     0,     0,     0,     0,
      85,    34,    35,    36,    37,    38,    39,    40,    41,     0,
      42,    43,    44,    45,    46,    26,    27,     0,    28,    29,
      30,    31,    32,    33,     0,     0,     0,     0,     0,    34,
      35,    36,    37,    38,    39,    40,    41,     0,    42,    43,
      44,    45,    46,    26,     0,     0,    28,    29,    30,    31,
      32,    33,     0,     0,     0,     0,     0,    34,    35,    36,
      37,    38,    39,    40,    41,     0,    42,    43,    44,    45,
      46,    28,    29,    30,    31,    32,    33,     0,     0,     0,
       0,     0,    34,    35,    36,    37,    38,    39,    40,    41,
       0,    42,    43,    44,    45,    46,    -1,    -1,    -1,    31,
      32,     0,     0,     0,     0,     0,     0,     0,     0,    -1,
      -1,    -1,    39,    40,    41,     0,    42,    43,    44,    45,
      46
};

static const yytype_int8 yycheck[] =
{
       7,    20,     9,    10,    11,    12,     3,    39,     3,    41,
       7,    18,     7,     0,    20,    12,    22,    23,    40,    26,
      27,    28,    29,    30,    31,    32,    35,    36,    37,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    -1,    49,    50,    33,    34,    35,    36,    37,     3,
       4,     5,     6,     7,     8,     9,    -1,    -1,    12,    -1,
      -1,    -1,    -1,    -1,    -1,    19,    -1,     3,     4,    76,
       6,     7,     8,     9,    81,    -1,    12,    84,    32,    33,
      34,    -1,    -1,    19,    -1,    -1,    40,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    32,    33,    34,    -1,
      -1,    -1,    10,    11,    40,    13,    14,    15,    16,    17,
      18,    -1,    -1,    -1,    -1,    -1,    24,    25,    26,    27,
      28,    29,    30,    31,    -1,    33,    34,    35,    36,    37,
      -1,    10,    11,    41,    13,    14,    15,    16,    17,    18,
      -1,    -1,    21,    -1,    -1,    24,    25,    26,    27,    28,
      29,    30,    31,    -1,    33,    34,    35,    36,    37,    10,
      11,    -1,    13,    14,    15,    16,    17,    18,    -1,    -1,
      21,    -1,    -1,    24,    25,    26,    27,    28,    29,    30,
      31,    -1,    33,    34,    35,    36,    37,    10,    11,    -1,
      13,    14,    15,    16,    17,    18,    -1,    -1,    -1,    -1,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    -1,
      33,    34,    35,    36,    37,    10,    11,    -1,    13,    14,
      15,    16,    17,    18,    -1,    -1,    -1,    -1,    -1,    24,
      25,    26,    27,    28,    29,    30,    31,    -1,    33,    34,
      35,    36,    37,    10,    -1,    -1,    13,    14,    15,    16,
      17,    18,    -1,    -1,    -1,    -1,    -1,    24,    25,    26,
      27,    28,    29,    30,    31,    -1,    33,    34,    35,    36,
      37,    13,    14,    15,    16,    17,    18,    -1,    -1,    -1,
      -1,    -1,    24,    25,    26,    27,    28,    29,    30,    31,
      -1,    33,    34,    35,    36,    37,    13,    14,    15,    16,
      17,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    26,
      27,    28,    29,    30,    31,    -1,    33,    34,    35,    36,
      37
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     6,     7,     8,     9,    12,    19,    32,
      33,    34,    40,    43,    45,    47,    48,    45,    20,    46,
      45,    45,     5,    45,    45,     0,    10,    11,    13,    14,
      15,    16,    17,    18,    24,    25,    26,    27,    28,    29,
      30,    31,    33,    34,    35,    36,    37,    40,    45,    20,
      22,    23,    41,    45,    45,    45,    45,    45,    45,    45,
       3,     7,    12,    45,    45,    45,    45,    45,    45,    45,
      45,    45,    45,    45,    44,    45,    21,    45,    45,     3,
       7,    39,    41,    45,    21,    23,    45,    45
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

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
      yyerror (expr_parse_result_p, yyscanner, YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

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


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (&yylval, YYLEX_PARAM)
#else
# define YYLEX yylex (&yylval, yyscanner)
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
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
		  Type, Value, expr_parse_result_p, yyscanner); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, PgBenchExpr **expr_parse_result_p, yyscan_t yyscanner)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep, expr_parse_result_p, yyscanner)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    PgBenchExpr **expr_parse_result_p;
    yyscan_t yyscanner;
#endif
{
  if (!yyvaluep)
    return;
  YYUSE (expr_parse_result_p);
  YYUSE (yyscanner);
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


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, PgBenchExpr **expr_parse_result_p, yyscan_t yyscanner)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep, expr_parse_result_p, yyscanner)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    PgBenchExpr **expr_parse_result_p;
    yyscan_t yyscanner;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep, expr_parse_result_p, yyscanner);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

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


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule, PgBenchExpr **expr_parse_result_p, yyscan_t yyscanner)
#else
static void
yy_reduce_print (yyvsp, yyrule, expr_parse_result_p, yyscanner)
    YYSTYPE *yyvsp;
    int yyrule;
    PgBenchExpr **expr_parse_result_p;
    yyscan_t yyscanner;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      fprintf (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       , expr_parse_result_p, yyscanner);
      fprintf (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule, expr_parse_result_p, yyscanner); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
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
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
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
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
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
	    /* Fall through.  */
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

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
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
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
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

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
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
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
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
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, PgBenchExpr **expr_parse_result_p, yyscan_t yyscanner)
#else
static void
yydestruct (yymsg, yytype, yyvaluep, expr_parse_result_p, yyscanner)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
    PgBenchExpr **expr_parse_result_p;
    yyscan_t yyscanner;
#endif
{
  YYUSE (yyvaluep);
  YYUSE (expr_parse_result_p);
  YYUSE (yyscanner);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (PgBenchExpr **expr_parse_result_p, yyscan_t yyscanner);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */






/*----------.
| yyparse.  |
`----------*/

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
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (PgBenchExpr **expr_parse_result_p, yyscan_t yyscanner)
#else
int
yyparse (expr_parse_result_p, yyscanner)
    PgBenchExpr **expr_parse_result_p;
    yyscan_t yyscanner;
#endif
#endif
{
  /* The look-ahead symbol.  */
int yychar;

/* The semantic value of the look-ahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;

  int yystate;
  int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Look-ahead token as an internal (translated) token number.  */
  int yytoken = 0;
#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  yytype_int16 yyssa[YYINITDEPTH];
  yytype_int16 *yyss = yyssa;
  yytype_int16 *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
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

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     look-ahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to look-ahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a look-ahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid look-ahead symbol.  */
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

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
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

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
#line 82 "/Users/rushilbhat/Documents/CS348/postgres/build/../src/bin/pgbench/exprparse.y"
    {
								*expr_parse_result_p = (yyvsp[(1) - (1)].expr);
								(void) yynerrs; /* suppress compiler warning */
							;}
    break;

  case 3:
#line 87 "/Users/rushilbhat/Documents/CS348/postgres/build/../src/bin/pgbench/exprparse.y"
    { (yyval.elist) = NULL; ;}
    break;

  case 4:
#line 88 "/Users/rushilbhat/Documents/CS348/postgres/build/../src/bin/pgbench/exprparse.y"
    { (yyval.elist) = make_elist((yyvsp[(1) - (1)].expr), NULL); ;}
    break;

  case 5:
#line 89 "/Users/rushilbhat/Documents/CS348/postgres/build/../src/bin/pgbench/exprparse.y"
    { (yyval.elist) = make_elist((yyvsp[(3) - (3)].expr), (yyvsp[(1) - (3)].elist)); ;}
    break;

  case 6:
#line 92 "/Users/rushilbhat/Documents/CS348/postgres/build/../src/bin/pgbench/exprparse.y"
    { (yyval.expr) = (yyvsp[(2) - (3)].expr); ;}
    break;

  case 7:
#line 93 "/Users/rushilbhat/Documents/CS348/postgres/build/../src/bin/pgbench/exprparse.y"
    { (yyval.expr) = (yyvsp[(2) - (2)].expr); ;}
    break;

  case 8:
#line 95 "/Users/rushilbhat/Documents/CS348/postgres/build/../src/bin/pgbench/exprparse.y"
    { (yyval.expr) = make_op(yyscanner, "-",
										   make_integer_constant(0), (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 9:
#line 99 "/Users/rushilbhat/Documents/CS348/postgres/build/../src/bin/pgbench/exprparse.y"
    { (yyval.expr) = make_integer_constant(PG_INT64_MIN); ;}
    break;

  case 10:
#line 101 "/Users/rushilbhat/Documents/CS348/postgres/build/../src/bin/pgbench/exprparse.y"
    { (yyval.expr) = make_op(yyscanner, "#",
										   make_integer_constant(~INT64CONST(0)), (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 11:
#line 103 "/Users/rushilbhat/Documents/CS348/postgres/build/../src/bin/pgbench/exprparse.y"
    { (yyval.expr) = make_uop(yyscanner, "!not", (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 12:
#line 104 "/Users/rushilbhat/Documents/CS348/postgres/build/../src/bin/pgbench/exprparse.y"
    { (yyval.expr) = make_op(yyscanner, "+", (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 13:
#line 105 "/Users/rushilbhat/Documents/CS348/postgres/build/../src/bin/pgbench/exprparse.y"
    { (yyval.expr) = make_op(yyscanner, "-", (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 14:
#line 106 "/Users/rushilbhat/Documents/CS348/postgres/build/../src/bin/pgbench/exprparse.y"
    { (yyval.expr) = make_op(yyscanner, "*", (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 15:
#line 107 "/Users/rushilbhat/Documents/CS348/postgres/build/../src/bin/pgbench/exprparse.y"
    { (yyval.expr) = make_op(yyscanner, "/", (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 16:
#line 108 "/Users/rushilbhat/Documents/CS348/postgres/build/../src/bin/pgbench/exprparse.y"
    { (yyval.expr) = make_op(yyscanner, "mod", (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 17:
#line 109 "/Users/rushilbhat/Documents/CS348/postgres/build/../src/bin/pgbench/exprparse.y"
    { (yyval.expr) = make_op(yyscanner, "<", (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 18:
#line 110 "/Users/rushilbhat/Documents/CS348/postgres/build/../src/bin/pgbench/exprparse.y"
    { (yyval.expr) = make_op(yyscanner, "<=", (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 19:
#line 111 "/Users/rushilbhat/Documents/CS348/postgres/build/../src/bin/pgbench/exprparse.y"
    { (yyval.expr) = make_op(yyscanner, "<", (yyvsp[(3) - (3)].expr), (yyvsp[(1) - (3)].expr)); ;}
    break;

  case 20:
#line 112 "/Users/rushilbhat/Documents/CS348/postgres/build/../src/bin/pgbench/exprparse.y"
    { (yyval.expr) = make_op(yyscanner, "<=", (yyvsp[(3) - (3)].expr), (yyvsp[(1) - (3)].expr)); ;}
    break;

  case 21:
#line 113 "/Users/rushilbhat/Documents/CS348/postgres/build/../src/bin/pgbench/exprparse.y"
    { (yyval.expr) = make_op(yyscanner, "=", (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 22:
#line 114 "/Users/rushilbhat/Documents/CS348/postgres/build/../src/bin/pgbench/exprparse.y"
    { (yyval.expr) = make_op(yyscanner, "<>", (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 23:
#line 115 "/Users/rushilbhat/Documents/CS348/postgres/build/../src/bin/pgbench/exprparse.y"
    { (yyval.expr) = make_op(yyscanner, "&", (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 24:
#line 116 "/Users/rushilbhat/Documents/CS348/postgres/build/../src/bin/pgbench/exprparse.y"
    { (yyval.expr) = make_op(yyscanner, "|", (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 25:
#line 117 "/Users/rushilbhat/Documents/CS348/postgres/build/../src/bin/pgbench/exprparse.y"
    { (yyval.expr) = make_op(yyscanner, "#", (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 26:
#line 118 "/Users/rushilbhat/Documents/CS348/postgres/build/../src/bin/pgbench/exprparse.y"
    { (yyval.expr) = make_op(yyscanner, "<<", (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 27:
#line 119 "/Users/rushilbhat/Documents/CS348/postgres/build/../src/bin/pgbench/exprparse.y"
    { (yyval.expr) = make_op(yyscanner, ">>", (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 28:
#line 120 "/Users/rushilbhat/Documents/CS348/postgres/build/../src/bin/pgbench/exprparse.y"
    { (yyval.expr) = make_op(yyscanner, "!and", (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 29:
#line 121 "/Users/rushilbhat/Documents/CS348/postgres/build/../src/bin/pgbench/exprparse.y"
    { (yyval.expr) = make_op(yyscanner, "!or", (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 30:
#line 123 "/Users/rushilbhat/Documents/CS348/postgres/build/../src/bin/pgbench/exprparse.y"
    { (yyval.expr) = make_op(yyscanner, "!is", (yyvsp[(1) - (2)].expr), make_null_constant()); ;}
    break;

  case 31:
#line 124 "/Users/rushilbhat/Documents/CS348/postgres/build/../src/bin/pgbench/exprparse.y"
    {
								(yyval.expr) = make_uop(yyscanner, "!not",
											  make_op(yyscanner, "!is", (yyvsp[(1) - (2)].expr), make_null_constant()));
							;}
    break;

  case 32:
#line 128 "/Users/rushilbhat/Documents/CS348/postgres/build/../src/bin/pgbench/exprparse.y"
    { (yyval.expr) = make_op(yyscanner, "!is", (yyvsp[(1) - (3)].expr), make_null_constant()); ;}
    break;

  case 33:
#line 130 "/Users/rushilbhat/Documents/CS348/postgres/build/../src/bin/pgbench/exprparse.y"
    {
								(yyval.expr) = make_uop(yyscanner, "!not",
											  make_op(yyscanner, "!is", (yyvsp[(1) - (4)].expr), make_null_constant()));
							;}
    break;

  case 34:
#line 135 "/Users/rushilbhat/Documents/CS348/postgres/build/../src/bin/pgbench/exprparse.y"
    {
								(yyval.expr) = make_op(yyscanner, "!is", (yyvsp[(1) - (3)].expr), make_boolean_constant((yyvsp[(3) - (3)].bval)));
							;}
    break;

  case 35:
#line 139 "/Users/rushilbhat/Documents/CS348/postgres/build/../src/bin/pgbench/exprparse.y"
    {
								(yyval.expr) = make_uop(yyscanner, "!not",
											  make_op(yyscanner, "!is", (yyvsp[(1) - (4)].expr), make_boolean_constant((yyvsp[(4) - (4)].bval))));
							;}
    break;

  case 36:
#line 144 "/Users/rushilbhat/Documents/CS348/postgres/build/../src/bin/pgbench/exprparse.y"
    { (yyval.expr) = make_null_constant(); ;}
    break;

  case 37:
#line 145 "/Users/rushilbhat/Documents/CS348/postgres/build/../src/bin/pgbench/exprparse.y"
    { (yyval.expr) = make_boolean_constant((yyvsp[(1) - (1)].bval)); ;}
    break;

  case 38:
#line 146 "/Users/rushilbhat/Documents/CS348/postgres/build/../src/bin/pgbench/exprparse.y"
    { (yyval.expr) = make_integer_constant((yyvsp[(1) - (1)].ival)); ;}
    break;

  case 39:
#line 147 "/Users/rushilbhat/Documents/CS348/postgres/build/../src/bin/pgbench/exprparse.y"
    { (yyval.expr) = make_double_constant((yyvsp[(1) - (1)].dval)); ;}
    break;

  case 40:
#line 149 "/Users/rushilbhat/Documents/CS348/postgres/build/../src/bin/pgbench/exprparse.y"
    { (yyval.expr) = make_variable((yyvsp[(1) - (1)].str)); ;}
    break;

  case 41:
#line 150 "/Users/rushilbhat/Documents/CS348/postgres/build/../src/bin/pgbench/exprparse.y"
    { (yyval.expr) = make_func(yyscanner, (yyvsp[(1) - (4)].ival), (yyvsp[(3) - (4)].elist)); ;}
    break;

  case 42:
#line 151 "/Users/rushilbhat/Documents/CS348/postgres/build/../src/bin/pgbench/exprparse.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 43:
#line 155 "/Users/rushilbhat/Documents/CS348/postgres/build/../src/bin/pgbench/exprparse.y"
    { (yyval.elist) = make_elist((yyvsp[(5) - (5)].expr), make_elist((yyvsp[(3) - (5)].expr), (yyvsp[(1) - (5)].elist))); ;}
    break;

  case 44:
#line 156 "/Users/rushilbhat/Documents/CS348/postgres/build/../src/bin/pgbench/exprparse.y"
    { (yyval.elist) = make_elist((yyvsp[(4) - (4)].expr), make_elist((yyvsp[(2) - (4)].expr), NULL)); ;}
    break;

  case 45:
#line 159 "/Users/rushilbhat/Documents/CS348/postgres/build/../src/bin/pgbench/exprparse.y"
    { (yyval.expr) = make_case(yyscanner, (yyvsp[(2) - (3)].elist), make_null_constant()); ;}
    break;

  case 46:
#line 160 "/Users/rushilbhat/Documents/CS348/postgres/build/../src/bin/pgbench/exprparse.y"
    { (yyval.expr) = make_case(yyscanner, (yyvsp[(2) - (5)].elist), (yyvsp[(4) - (5)].expr)); ;}
    break;

  case 47:
#line 162 "/Users/rushilbhat/Documents/CS348/postgres/build/../src/bin/pgbench/exprparse.y"
    { (yyval.ival) = find_func(yyscanner, (yyvsp[(1) - (1)].str)); pg_free((yyvsp[(1) - (1)].str)); ;}
    break;


/* Line 1267 of yacc.c.  */
#line 1787 "exprparse.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (expr_parse_result_p, yyscanner, YY_("syntax error"));
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
	    yyerror (expr_parse_result_p, yyscanner, yymsg);
	  }
	else
	  {
	    yyerror (expr_parse_result_p, yyscanner, YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse look-ahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval, expr_parse_result_p, yyscanner);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse look-ahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

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

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp, expr_parse_result_p, yyscanner);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (expr_parse_result_p, yyscanner, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEOF && yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval, expr_parse_result_p, yyscanner);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp, expr_parse_result_p, yyscanner);
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
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}


#line 165 "/Users/rushilbhat/Documents/CS348/postgres/build/../src/bin/pgbench/exprparse.y"


static PgBenchExpr *
make_null_constant(void)
{
	PgBenchExpr *expr = pg_malloc(sizeof(PgBenchExpr));

	expr->etype = ENODE_CONSTANT;
	expr->u.constant.type = PGBT_NULL;
	expr->u.constant.u.ival = 0;
	return expr;
}

static PgBenchExpr *
make_integer_constant(int64 ival)
{
	PgBenchExpr *expr = pg_malloc(sizeof(PgBenchExpr));

	expr->etype = ENODE_CONSTANT;
	expr->u.constant.type = PGBT_INT;
	expr->u.constant.u.ival = ival;
	return expr;
}

static PgBenchExpr *
make_double_constant(double dval)
{
	PgBenchExpr *expr = pg_malloc(sizeof(PgBenchExpr));

	expr->etype = ENODE_CONSTANT;
	expr->u.constant.type = PGBT_DOUBLE;
	expr->u.constant.u.dval = dval;
	return expr;
}

static PgBenchExpr *
make_boolean_constant(bool bval)
{
	PgBenchExpr *expr = pg_malloc(sizeof(PgBenchExpr));

	expr->etype = ENODE_CONSTANT;
	expr->u.constant.type = PGBT_BOOLEAN;
	expr->u.constant.u.bval = bval;
	return expr;
}

static PgBenchExpr *
make_variable(char *varname)
{
	PgBenchExpr *expr = pg_malloc(sizeof(PgBenchExpr));

	expr->etype = ENODE_VARIABLE;
	expr->u.variable.varname = varname;
	return expr;
}

/* binary operators */
static PgBenchExpr *
make_op(yyscan_t yyscanner, const char *operator,
		PgBenchExpr *lexpr, PgBenchExpr *rexpr)
{
	return make_func(yyscanner, find_func(yyscanner, operator),
					 make_elist(rexpr, make_elist(lexpr, NULL)));
}

/* unary operator */
static PgBenchExpr *
make_uop(yyscan_t yyscanner, const char *operator, PgBenchExpr *expr)
{
	return make_func(yyscanner, find_func(yyscanner, operator), make_elist(expr, NULL));
}

/*
 * List of available functions:
 * - fname: function name, "!..." for special internal functions
 * - nargs: number of arguments. Special cases:
 *			- PGBENCH_NARGS_VARIABLE is a special value for least & greatest
 *			  meaning #args >= 1;
 *			- PGBENCH_NARGS_CASE is for the "CASE WHEN ..." function, which
 *			  has #args >= 3 and odd;
 *			- PGBENCH_NARGS_HASH is for hash functions, which have one required
 *			  and one optional argument;
 * - tag: function identifier from PgBenchFunction enum
 */
static const struct
{
	const char *fname;
	int			nargs;
	PgBenchFunction tag;
}			PGBENCH_FUNCTIONS[] =

{
	/* parsed as operators, executed as functions */
	{
		"+", 2, PGBENCH_ADD
	},
	{
		"-", 2, PGBENCH_SUB
	},
	{
		"*", 2, PGBENCH_MUL
	},
	{
		"/", 2, PGBENCH_DIV
	},
	{
		"mod", 2, PGBENCH_MOD
	},
	/* actual functions */
	{
		"abs", 1, PGBENCH_ABS
	},
	{
		"least", PGBENCH_NARGS_VARIABLE, PGBENCH_LEAST
	},
	{
		"greatest", PGBENCH_NARGS_VARIABLE, PGBENCH_GREATEST
	},
	{
		"debug", 1, PGBENCH_DEBUG
	},
	{
		"pi", 0, PGBENCH_PI
	},
	{
		"sqrt", 1, PGBENCH_SQRT
	},
	{
		"ln", 1, PGBENCH_LN
	},
	{
		"exp", 1, PGBENCH_EXP
	},
	{
		"int", 1, PGBENCH_INT
	},
	{
		"double", 1, PGBENCH_DOUBLE
	},
	{
		"random", 2, PGBENCH_RANDOM
	},
	{
		"random_gaussian", 3, PGBENCH_RANDOM_GAUSSIAN
	},
	{
		"random_exponential", 3, PGBENCH_RANDOM_EXPONENTIAL
	},
	{
		"random_zipfian", 3, PGBENCH_RANDOM_ZIPFIAN
	},
	{
		"pow", 2, PGBENCH_POW
	},
	{
		"power", 2, PGBENCH_POW
	},
	/* logical operators */
	{
		"!and", 2, PGBENCH_AND
	},
	{
		"!or", 2, PGBENCH_OR
	},
	{
		"!not", 1, PGBENCH_NOT
	},
	/* bitwise integer operators */
	{
		"&", 2, PGBENCH_BITAND
	},
	{
		"|", 2, PGBENCH_BITOR
	},
	{
		"#", 2, PGBENCH_BITXOR
	},
	{
		"<<", 2, PGBENCH_LSHIFT
	},
	{
		">>", 2, PGBENCH_RSHIFT
	},
	/* comparison operators */
	{
		"=", 2, PGBENCH_EQ
	},
	{
		"<>", 2, PGBENCH_NE
	},
	{
		"<=", 2, PGBENCH_LE
	},
	{
		"<", 2, PGBENCH_LT
	},
	{
		"!is", 2, PGBENCH_IS
	},
	/* "case when ... then ... else ... end" construction */
	{
		"!case_end", PGBENCH_NARGS_CASE, PGBENCH_CASE
	},
	{
		"hash", PGBENCH_NARGS_HASH, PGBENCH_HASH_MURMUR2
	},
	{
		"hash_murmur2", PGBENCH_NARGS_HASH, PGBENCH_HASH_MURMUR2
	},
	{
		"hash_fnv1a", PGBENCH_NARGS_HASH, PGBENCH_HASH_FNV1A
	},
	{
		"permute", PGBENCH_NARGS_PERMUTE, PGBENCH_PERMUTE
	},
	/* keep as last array element */
	{
		NULL, 0, 0
	}
};

/*
 * Find a function from its name
 *
 * return the index of the function from the PGBENCH_FUNCTIONS array
 * or fail if the function is unknown.
 */
static int
find_func(yyscan_t yyscanner, const char *fname)
{
	int			i = 0;

	while (PGBENCH_FUNCTIONS[i].fname)
	{
		if (pg_strcasecmp(fname, PGBENCH_FUNCTIONS[i].fname) == 0)
			return i;
		i++;
	}

	expr_yyerror_more(yyscanner, "unexpected function name", fname);

	/* not reached */
	return -1;
}

/* Expression linked list builder */
static PgBenchExprList *
make_elist(PgBenchExpr *expr, PgBenchExprList *list)
{
	PgBenchExprLink *cons;

	if (list == NULL)
	{
		list = pg_malloc(sizeof(PgBenchExprList));
		list->head = NULL;
		list->tail = NULL;
	}

	cons = pg_malloc(sizeof(PgBenchExprLink));
	cons->expr = expr;
	cons->next = NULL;

	if (list->head == NULL)
		list->head = cons;
	else
		list->tail->next = cons;

	list->tail = cons;

	return list;
}

/* Return the length of an expression list */
static int
elist_length(PgBenchExprList *list)
{
	PgBenchExprLink *link = list != NULL ? list->head : NULL;
	int			len = 0;

	for (; link != NULL; link = link->next)
		len++;

	return len;
}

/* Build function call expression */
static PgBenchExpr *
make_func(yyscan_t yyscanner, int fnumber, PgBenchExprList *args)
{
	int			len = elist_length(args);

	PgBenchExpr *expr = pg_malloc(sizeof(PgBenchExpr));

	Assert(fnumber >= 0);

	/* validate arguments number including few special cases */
	switch (PGBENCH_FUNCTIONS[fnumber].nargs)
	{
			/* check at least one arg for least & greatest */
		case PGBENCH_NARGS_VARIABLE:
			if (len == 0)
				expr_yyerror_more(yyscanner, "at least one argument expected",
								  PGBENCH_FUNCTIONS[fnumber].fname);
			break;

			/* case (when ... then ...)+ (else ...)? end */
		case PGBENCH_NARGS_CASE:
			/* 'else' branch is always present, but could be a NULL-constant */
			if (len < 3 || len % 2 != 1)
				expr_yyerror_more(yyscanner,
								  "odd and >= 3 number of arguments expected",
								  "case control structure");
			break;

			/* hash functions with optional seed argument */
		case PGBENCH_NARGS_HASH:
			if (len < 1 || len > 2)
				expr_yyerror_more(yyscanner, "unexpected number of arguments",
								  PGBENCH_FUNCTIONS[fnumber].fname);

			if (len == 1)
			{
				PgBenchExpr *var = make_variable("default_seed");

				args = make_elist(var, args);
			}
			break;

			/* pseudorandom permutation function with optional seed argument */
		case PGBENCH_NARGS_PERMUTE:
			if (len < 2 || len > 3)
				expr_yyerror_more(yyscanner, "unexpected number of arguments",
								  PGBENCH_FUNCTIONS[fnumber].fname);

			if (len == 2)
			{
				PgBenchExpr *var = make_variable("default_seed");

				args = make_elist(var, args);
			}
			break;

			/* common case: positive arguments number */
		default:
			Assert(PGBENCH_FUNCTIONS[fnumber].nargs >= 0);

			if (PGBENCH_FUNCTIONS[fnumber].nargs != len)
				expr_yyerror_more(yyscanner, "unexpected number of arguments",
								  PGBENCH_FUNCTIONS[fnumber].fname);
	}

	expr->etype = ENODE_FUNCTION;
	expr->u.function.function = PGBENCH_FUNCTIONS[fnumber].tag;

	/* only the link is used, the head/tail is not useful anymore */
	expr->u.function.args = args != NULL ? args->head : NULL;
	if (args)
		pg_free(args);

	return expr;
}

static PgBenchExpr *
make_case(yyscan_t yyscanner, PgBenchExprList *when_then_list, PgBenchExpr *else_part)
{
	return make_func(yyscanner,
					 find_func(yyscanner, "!case_end"),
					 make_elist(else_part, when_then_list));
}

