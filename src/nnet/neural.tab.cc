/* A Bison parser, made from nnet/neural.y
   by GNU bison 1.35.  */

#define YYBISON 1  /* Identify Bison output.  */

#define yyparse neuralparse
#define yylex neurallex
#define yyerror neuralerror
#define yylval neurallval
#define yychar neuralchar
#define yydebug neuraldebug
#define yynerrs neuralnerrs
# define	EQ	257
# define	NOTEQ	258
# define	LESSEQ	259
# define	GREATEREQ	260
# define	AND	261
# define	OR	262
# define	NOT	263
# define	FLOAT_NUM	264
# define	INT_NUM	265
# define	STRING	266
# define	QUOTED_STRING	267

#line 1 "nnet/neural.y"
 
#include "../common.h"
#include "neural.h"
#include "utils.h" // toString
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

#define YYPARSE_PARAM neuralnet
//to make debug information visible, uncomment the next bison_row 
//and assign somewhere yydebug=1
//#define YYDEBUG 1

const int DECDIGIT_NULL = -1;
const int MSG_LENGTH = 1000;

CExpression *bison_input_result;

CString bison_row_buf;
const char *bison_row;
int row_num;
CNeuralNet *bison_nnet;
// chartofloat's from the "in" function
int chartofloat_in;

int yylex ();
int yyerror (char *s);

void
resolve_vars(char *line, hash *vars, text *file = NULL); //defined in block.cc

inline UNIT get_level (const char *level_name)
{
	return str2enum (level_name, scfg->unit_levels, U_DEFAULT);
}

CExpression *
add_infix_bin_func (TFunc ifunc, CExpression *par1, CExpression *par2)
{
	CExpression *result = new CExpression;
	result->owner = bison_nnet;
	result->function = ifunc;
	result->args.push_back (*par1);
	result->args.push_back (*par2);
	delete par1;
	delete par2;
	return result;
}

CExpression *
add_infix_un_func (TFunc ifunc, CExpression *par1)
{
	CExpression *result = new CExpression;
	result->owner = bison_nnet;
	result->function = ifunc;
	result->args.push_back (*par1);
	delete par1;
	return result;
}

CExpression *
add_prefix_func (TFunc ifunc, const char *chtof, 
	CExpression *par1 = NULL, CExpression *par2 = NULL, CExpression *par3 = NULL)
{
	CExpression *result = new CExpression;
	result->owner = bison_nnet;
	result->function = ifunc;
	if (chtof != NULL) {
		TChar2Floats::iterator ich2f;
		for (ich2f = bison_nnet->char2floats.begin(); ich2f != bison_nnet->char2floats.end(); ++ich2f)
			if (!strcmp(ich2f->name.c_str(), chtof))
				break;
		if (ich2f == bison_nnet->char2floats.end())
			shriek (812, fmt ("BISON:add_prefix_func:Unknown function %s", chtof));
		result->which_char2float = ich2f;
	}
	if (par1) { result->args.push_back (*par1); delete par1; }
	if (par2) { result->args.push_back (*par2); delete par2; }
	if (par3) { result->args.push_back (*par3); delete par3; }
	return result;
}

CExpression *
make_tree (TTypedValue val = TTypedValue())
{
	CExpression *result = new CExpression;
	result->owner = bison_nnet;
	result->function = fu_value;
	result->which_value = val;
	return result;
}


#line 102 "nnet/neural.y"
#ifndef YYSTYPE
typedef union {
	long int int_val;
	double float_val;
	char string_val [MSG_LENGTH];
	unit *unit_val;
	TTypedValue *typed_val;
	CExpression *func_val;
	TFunc func_index;
} yystype;
# define YYSTYPE yystype
# define YYSTYPE_IS_TRIVIAL 1
#endif
#ifndef YYDEBUG
# define YYDEBUG 0
#endif



#define	YYFINAL		119
#define	YYFLAG		-32768
#define	YYNTBASE	34

/* YYTRANSLATE(YYLEX) -- Bison token number corresponding to YYLEX. */
#define YYTRANSLATE(x) ((unsigned)(x) <= 277 ? yytranslate[x] : 43)

/* YYTRANSLATE[YYLEX] -- Bison token number corresponding to YYLEX. */
static const char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    28,     2,     2,     2,     2,     2,     2,
      24,    25,    13,    11,    27,    12,     2,    14,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       9,     2,    10,     2,     2,     2,     2,     2,     2,     2,
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
       2,     2,     2,     2,     2,     2,     1,     3,     4,     5,
       6,     7,     8,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    26,    29,    30,    31,    32,    33
};

#if YYDEBUG
static const short yyprhs[] =
{
       0,     0,     1,     4,     6,     8,    11,    14,    16,    21,
      30,    34,    42,    48,    54,    60,    64,    68,    72,    75,
      80,    83,    87,    91,    95,    99,   103,   107,   111,   115,
     119,   123,   127,   131,   138,   140,   144,   146,   153,   160,
     165,   170,   174,   176,   180,   182
};
static const short yyrhs[] =
{
      -1,    35,    36,     0,    39,     0,     1,     0,    20,    38,
       0,    21,    38,     0,    22,     0,    23,    24,    42,    25,
       0,    26,    24,    19,    27,    42,    27,    42,    25,     0,
      24,    37,    25,     0,    24,    37,    27,    42,    27,    40,
      25,     0,    24,    37,    27,    42,    25,     0,    24,    42,
      27,    40,    25,     0,    24,    37,    27,    40,    25,     0,
      24,    37,    25,     0,    24,    42,    25,     0,    24,    40,
      25,     0,    24,    25,     0,    18,    24,    37,    25,     0,
      28,    39,     0,    39,     8,    39,     0,    39,     7,    39,
       0,    39,     3,    39,     0,    39,     4,    39,     0,    39,
       9,    39,     0,    39,     5,    39,     0,    39,    10,    39,
       0,    39,     6,    39,     0,    39,    11,    39,     0,    39,
      12,    39,     0,    39,    13,    39,     0,    39,    14,    39,
       0,    29,    24,    37,    27,    40,    25,     0,    40,     0,
      24,    39,    25,     0,    16,     0,    30,    24,    42,    27,
      42,    25,     0,    31,    24,    42,    27,    42,    25,     0,
      32,    24,    37,    25,     0,    33,    24,    37,    25,     0,
      24,    40,    25,     0,    17,     0,    24,    41,    25,     0,
      19,     0,    41,     0
};

#endif

#if YYDEBUG
/* YYRLINE[YYN] -- source line where rule number YYN was defined. */
static const short yyrline[] =
{
       0,   157,   157,   164,   166,   169,   170,   171,   172,   173,
     174,   178,   180,   181,   182,   183,   184,   185,   186,   190,
     191,   192,   193,   194,   195,   196,   197,   198,   199,   200,
     201,   202,   203,   204,   205,   206,   207,   210,   211,   212,
     213,   214,   215,   217,   218,   220
};
#endif

#define YYNTOKENS 34
#define YYNNTS 9
#define YYNRULES 45
#define YYNSTATES 120
#define YYMAXUTOK 277

/* YYTNAME[TOKEN_NUM] -- String name of the token TOKEN_NUM. */
static const char *const yytname[] =
{
  "$", "error", "$undefined.", "\"==\"", "\"!=\"", "\"<=\"", "\">=\"", 
  "\"AND\"", "\"OR\"", "'<'", "'>'", "'+'", "'-'", "'*'", "'/'", "NOT", 
  "FLOAT_NUM", "INT_NUM", "STRING", "QUOTED_STRING", "\"next\"", 
  "\"prev\"", "\"this\"", "\"ancestor\"", "'('", "')'", "\"maxfloat\"", 
  "','", "'!'", "\"neural\"", "\"count\"", "\"index\"", "\"basal_f\"", 
  "\"cont\"", "all", "@1", "input_exp", "unit_exp", "prev_next_params", 
  "float_exp", "int_exp", "string_exp", "unit_level", 0
};
/* YYTOKNUM[YYLEX] -- Index in YYTNAME corresponding to YYLEX. */
static const short yytoknum[] =
{
       0,   256,     2,   257,   258,   259,   260,   261,   262,    60,
      62,    43,    45,    42,    47,   263,   264,   265,   266,   267,
     268,   269,   270,   271,    40,    41,   272,    44,    33,   273,
     274,   275,   276,   277,    -1
};
/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives. */
static const short yyr1[] =
{
       0,    35,    34,    36,    36,    37,    37,    37,    37,    37,
      37,    38,    38,    38,    38,    38,    38,    38,    38,    39,
      39,    39,    39,    39,    39,    39,    39,    39,    39,    39,
      39,    39,    39,    39,    39,    39,    39,    40,    40,    40,
      40,    40,    40,    41,    41,    42
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN. */
static const short yyr2[] =
{
       0,     0,     2,     1,     1,     2,     2,     1,     4,     8,
       3,     7,     5,     5,     5,     3,     3,     3,     2,     4,
       2,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     6,     1,     3,     1,     6,     6,     4,
       4,     3,     1,     3,     1,     1
};

/* YYDEFACT[S] -- default rule to reduce with in state S when YYTABLE
   doesn't specify something else to do.  Zero means the default is an
   error. */
static const short yydefact[] =
{
       1,     0,     4,    36,    42,     0,     0,     0,     0,     0,
       0,     0,     0,     2,     3,    34,     0,     0,    34,    20,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     7,
       0,     0,     0,     0,    35,    41,     0,    44,     0,    45,
       0,     0,     0,     0,    23,    24,    26,    28,    22,    21,
      25,    27,    29,    30,    31,    32,     0,     5,     6,     0,
       0,     0,    19,     0,     0,     0,     0,    39,    40,     0,
      18,     0,     0,     0,     0,    10,     0,     0,     0,    43,
       0,     0,     0,    15,     0,    17,    16,     0,     8,     0,
      33,    37,    38,     0,     0,     0,     0,     0,    14,    12,
       0,    13,     0,     0,     0,    11,     9,     0,     0,     0
};

static const short yydefgoto[] =
{
     117,     1,    13,    70,    67,    14,    15,    49,    50
};

static const short yypact[] =
{
  -32768,     9,-32768,-32768,-32768,   -17,   101,   101,   -12,     8,
      12,    20,    23,-32768,   166,-32768,   100,    10,   -21,-32768,
     100,   -13,   -13,   100,   100,   101,   101,   101,   101,   101,
     101,   101,   101,   101,   101,   101,   101,    24,    24,-32768,
      47,   100,    48,    55,-32768,-32768,    56,-32768,   -13,-32768,
      57,    58,    64,    65,    89,    89,    17,    17,   190,   178,
      17,    17,    40,    40,-32768,-32768,   118,-32768,-32768,   -13,
      68,    63,-32768,    45,    71,   -13,   -13,-32768,-32768,   135,
  -32768,    18,   102,    34,   103,-32768,   109,    45,   120,-32768,
     121,   122,   -21,-32768,    33,-32768,-32768,    45,-32768,   -13,
  -32768,-32768,-32768,    33,   128,    43,   137,   133,-32768,-32768,
      45,-32768,   -13,   138,   139,-32768,-32768,   186,   197,-32768
};

static const short yypgoto[] =
{
  -32768,-32768,-32768,   -15,   160,    80,    -6,   -45,   -20
};


#define	YYLAST		204


static const short yytable[] =
{
      18,    43,    51,    74,    45,    46,    47,    16,    52,    53,
       2,    48,    20,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,     3,     4,     5,    33,    34,
      35,    36,    21,     6,    74,    44,    22,     7,     8,     9,
      10,    11,    12,    93,    23,    94,    83,    24,    66,    84,
       4,    81,    47,    35,    36,    90,    91,   103,    74,    96,
      82,    97,     4,     9,    10,    11,    12,    88,   109,    87,
     110,    69,    71,    92,   105,     9,    10,    11,    12,   107,
      72,    92,    86,    73,    75,    76,    17,    19,   104,    77,
      78,   106,   114,    85,    27,    28,    89,    92,    31,    32,
      33,    34,    35,    36,   113,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,     3,     4,     5,
      37,    38,    39,    40,    41,     6,    42,    95,    98,     7,
       8,     9,    10,    11,    12,     4,    99,    47,    37,    38,
      39,    40,    79,    80,    42,   100,   101,   102,     9,    10,
      11,    12,     4,   108,    47,    37,    38,    39,    40,    79,
     112,    42,   111,   115,   116,     9,    10,    11,    12,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    25,    26,    27,    28,    29,   118,    31,    32,    33,
      34,    35,    36,    25,    26,    27,    28,   119,    68,    31,
      32,    33,    34,    35,    36
};

static const short yycheck[] =
{
       6,    16,    22,    48,    25,    20,    19,    24,    23,    24,
       1,    24,    24,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,    16,    17,    18,    11,    12,
      13,    14,    24,    24,    79,    25,    24,    28,    29,    30,
      31,    32,    33,    25,    24,    27,    66,    24,    24,    69,
      17,    66,    19,    13,    14,    75,    76,    24,   103,    25,
      66,    27,    17,    30,    31,    32,    33,    73,    25,    24,
      27,    24,    24,    79,    94,    30,    31,    32,    33,    99,
      25,    87,    19,    27,    27,    27,     6,     7,    94,    25,
      25,    97,   112,    25,     5,     6,    25,   103,     9,    10,
      11,    12,    13,    14,   110,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    16,    17,    18,
      20,    21,    22,    23,    24,    24,    26,    25,    25,    28,
      29,    30,    31,    32,    33,    17,    27,    19,    20,    21,
      22,    23,    24,    25,    26,    25,    25,    25,    30,    31,
      32,    33,    17,    25,    19,    20,    21,    22,    23,    24,
      27,    26,    25,    25,    25,    30,    31,    32,    33,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,     3,     4,     5,     6,     7,     0,     9,    10,    11,
      12,    13,    14,     3,     4,     5,     6,     0,    38,     9,
      10,    11,    12,    13,    14
};
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "/usr/share/bison/bison.simple"

/* Skeleton output parser for bison,

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002 Free Software
   Foundation, Inc.

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
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* This is the parser code that is written into each bison parser when
   the %semantic_parser declaration is not specified in the grammar.
   It was written by Richard Stallman by simplifying the hairy parser
   used when %semantic_parser is specified.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

#if ! defined (yyoverflow) || defined (YYERROR_VERBOSE)

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# if YYSTACK_USE_ALLOCA
#  define YYSTACK_ALLOC alloca
# else
#  ifndef YYSTACK_USE_ALLOCA
#   if defined (alloca) || defined (_ALLOCA_H)
#    define YYSTACK_ALLOC alloca
#   else
#    ifdef __GNUC__
#     define YYSTACK_ALLOC __builtin_alloca
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
# else
#  if defined (__STDC__) || defined (__cplusplus)
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   define YYSIZE_T size_t
#  endif
#  define YYSTACK_ALLOC malloc
#  define YYSTACK_FREE free
# endif
#endif /* ! defined (yyoverflow) || defined (YYERROR_VERBOSE) */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (YYLTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short yyss;
  YYSTYPE yyvs;
# if YYLSP_NEEDED
  YYLTYPE yyls;
# endif
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAX (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# if YYLSP_NEEDED
#  define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE) + sizeof (YYLTYPE))	\
      + 2 * YYSTACK_GAP_MAX)
# else
#  define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE))				\
      + YYSTACK_GAP_MAX)
# endif

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  register YYSIZE_T yyi;		\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (0)
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
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAX;	\
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif


#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# if defined (__STDC__) || defined (__cplusplus)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# endif
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	goto yyacceptlab
#define YYABORT 	goto yyabortlab
#define YYERROR		goto yyerrlab1
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
      yychar1 = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { 								\
      yyerror ("syntax error: cannot back up");			\
      YYERROR;							\
    }								\
while (0)

#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).

   When YYLLOC_DEFAULT is run, CURRENT is set the location of the
   first token.  By default, to implement support for ranges, extend
   its range to the last symbol.  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)       	\
   Current.last_line   = Rhs[N].last_line;	\
   Current.last_column = Rhs[N].last_column;
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#if YYPURE
# if YYLSP_NEEDED
#  ifdef YYLEX_PARAM
#   define YYLEX		yylex (&yylval, &yylloc, YYLEX_PARAM)
#  else
#   define YYLEX		yylex (&yylval, &yylloc)
#  endif
# else /* !YYLSP_NEEDED */
#  ifdef YYLEX_PARAM
#   define YYLEX		yylex (&yylval, YYLEX_PARAM)
#  else
#   define YYLEX		yylex (&yylval)
#  endif
# endif /* !YYLSP_NEEDED */
#else /* !YYPURE */
# define YYLEX			yylex ()
#endif /* !YYPURE */


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
} while (0)
/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
#endif /* !YYDEBUG */

/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   SIZE_MAX < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#if YYMAXDEPTH == 0
# undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif

#ifdef YYERROR_VERBOSE

# ifndef yystrlen
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
yystrlen (const char *yystr)
#   else
yystrlen (yystr)
     const char *yystr;
#   endif
{
  register const char *yys = yystr;

  while (*yys++ != '\0')
    continue;

  return yys - yystr - 1;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
yystpcpy (char *yydest, const char *yysrc)
#   else
yystpcpy (yydest, yysrc)
     char *yydest;
     const char *yysrc;
#   endif
{
  register char *yyd = yydest;
  register const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif
#endif

#line 315 "/usr/share/bison/bison.simple"


/* The user can define YYPARSE_PARAM as the name of an argument to be passed
   into yyparse.  The argument should have type void *.
   It should actually point to an object.
   Grammar actions can access the variable by casting it
   to the proper pointer type.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
#  define YYPARSE_PARAM_ARG void *YYPARSE_PARAM
#  define YYPARSE_PARAM_DECL
# else
#  define YYPARSE_PARAM_ARG YYPARSE_PARAM
#  define YYPARSE_PARAM_DECL void *YYPARSE_PARAM;
# endif
#else /* !YYPARSE_PARAM */
# define YYPARSE_PARAM_ARG
# define YYPARSE_PARAM_DECL
#endif /* !YYPARSE_PARAM */

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
# ifdef YYPARSE_PARAM
int yyparse (void *);
# else
int yyparse (void);
# endif
#endif

/* YY_DECL_VARIABLES -- depending whether we use a pure parser,
   variables are global, or local to YYPARSE.  */

#define YY_DECL_NON_LSP_VARIABLES			\
/* The lookahead symbol.  */				\
int yychar;						\
							\
/* The semantic value of the lookahead symbol. */	\
YYSTYPE yylval;						\
							\
/* Number of parse errors so far.  */			\
int yynerrs;

#if YYLSP_NEEDED
# define YY_DECL_VARIABLES			\
YY_DECL_NON_LSP_VARIABLES			\
						\
/* Location data for the lookahead symbol.  */	\
YYLTYPE yylloc;
#else
# define YY_DECL_VARIABLES			\
YY_DECL_NON_LSP_VARIABLES
#endif


/* If nonreentrant, generate the variables here. */

#if !YYPURE
YY_DECL_VARIABLES
#endif  /* !YYPURE */

int
yyparse (YYPARSE_PARAM_ARG)
     YYPARSE_PARAM_DECL
{
  /* If reentrant, generate the variables here. */
#if YYPURE
  YY_DECL_VARIABLES
#endif  /* !YYPURE */

  register int yystate;
  register int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Lookahead token as an internal (translated) token number.  */
  int yychar1 = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack. */
  short	yyssa[YYINITDEPTH];
  short *yyss = yyssa;
  register short *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  register YYSTYPE *yyvsp;

#if YYLSP_NEEDED
  /* The location stack.  */
  YYLTYPE yylsa[YYINITDEPTH];
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;
#endif

#if YYLSP_NEEDED
# define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)
#else
# define YYPOPSTACK   (yyvsp--, yyssp--)
#endif

  YYSIZE_T yystacksize = YYINITDEPTH;


  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
#if YYLSP_NEEDED
  YYLTYPE yyloc;
#endif

  /* When reducing, the number of symbols on the RHS of the reduced
     rule. */
  int yylen;

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
#if YYLSP_NEEDED
  yylsp = yyls;
#endif
  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed. so pushing a state here evens the stacks.
     */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyssp >= yyss + yystacksize - 1)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  */
# if YYLSP_NEEDED
	YYLTYPE *yyls1 = yyls;
	/* This used to be a conditional around just the two extra args,
	   but that might be undefined if yyoverflow is a macro.  */
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yyls1, yysize * sizeof (*yylsp),
		    &yystacksize);
	yyls = yyls1;
# else
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);
# endif
	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyoverflowlab;
# else
      /* Extend the stack our own way.  */
      if (yystacksize >= YYMAXDEPTH)
	goto yyoverflowlab;
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
	yystacksize = YYMAXDEPTH;

      {
	short *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyoverflowlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);
# if YYLSP_NEEDED
	YYSTACK_RELOCATE (yyls);
# endif
# undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
#if YYLSP_NEEDED
      yylsp = yyls + yysize - 1;
#endif

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyssp >= yyss + yystacksize - 1)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* yychar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (yychar <= 0)		/* This means end of input. */
    {
      yychar1 = 0;
      yychar = YYEOF;		/* Don't call YYLEX any more */

      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yychar1 = YYTRANSLATE (yychar);

#if YYDEBUG
     /* We have to keep this `#if YYDEBUG', since we use variables
	which are defined only if `YYDEBUG' is set.  */
      if (yydebug)
	{
	  YYFPRINTF (stderr, "Next token is %d (%s",
		     yychar, yytname[yychar1]);
	  /* Give the individual parser a way to print the precise
	     meaning of a token, for further debugging info.  */
# ifdef YYPRINT
	  YYPRINT (stderr, yychar, yylval);
# endif
	  YYFPRINTF (stderr, ")\n");
	}
#endif
    }

  yyn += yychar1;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != yychar1)
    goto yydefault;

  yyn = yytable[yyn];

  /* yyn is what to do for this token type in this state.
     Negative => reduce, -yyn is rule number.
     Positive => shift, yyn is new state.
       New state is final state => don't bother to shift,
       just return success.
     0, or most negative number => error.  */

  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrlab;

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */
  YYDPRINTF ((stderr, "Shifting token %d (%s), ",
	      yychar, yytname[yychar1]));

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
#if YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
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

     Otherwise, the following line sets YYVAL to the semantic value of
     the lookahead token.  This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

#if YYLSP_NEEDED
  /* Similarly for the default location.  Let the user run additional
     commands if for instance locations are ranges.  */
  yyloc = yylsp[1-yylen];
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
#endif

#if YYDEBUG
  /* We have to keep this `#if YYDEBUG', since we use variables which
     are defined only if `YYDEBUG' is set.  */
  if (yydebug)
    {
      int yyi;

      YYFPRINTF (stderr, "Reducing via rule %d (line %d), ",
		 yyn, yyrline[yyn]);

      /* Print the symbols being reduced, and their result.  */
      for (yyi = yyprhs[yyn]; yyrhs[yyi] > 0; yyi++)
	YYFPRINTF (stderr, "%s ", yytname[yyrhs[yyi]]);
      YYFPRINTF (stderr, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif

  switch (yyn) {

case 1:
#line 157 "nnet/neural.y"
{ 
		//yydebug = 1;
	 ;
    break;}
case 2:
#line 160 "nnet/neural.y"
{ bison_input_result = yyvsp[0].func_val; ;
    break;}
case 4:
#line 166 "nnet/neural.y"
{ yyval.func_val = NULL; ;
    break;}
case 5:
#line 169 "nnet/neural.y"
{ yyval.func_val = yyvsp[0].func_val; ;
    break;}
case 6:
#line 170 "nnet/neural.y"
{ yyval.func_val = yyvsp[0].func_val; yyval.func_val->function = fu_prev; ;
    break;}
case 7:
#line 171 "nnet/neural.y"
{ yyval.func_val = add_prefix_func (fu_this,0,NULL); ;
    break;}
case 8:
#line 172 "nnet/neural.y"
{ yyval.func_val = add_prefix_func (fu_ancestor,0,yyvsp[-1].func_val); ;
    break;}
case 9:
#line 173 "nnet/neural.y"
{ yyval.func_val = add_prefix_func (fu_maxfloat,yyvsp[-5].string_val,yyvsp[-3].func_val,yyvsp[-1].func_val); ;
    break;}
case 10:
#line 174 "nnet/neural.y"
{ yyval.func_val = yyvsp[-1].func_val; ;
    break;}
case 11:
#line 179 "nnet/neural.y"
{ yyval.func_val = add_prefix_func (fu_next,0,yyvsp[-5].func_val,yyvsp[-3].func_val,yyvsp[-1].func_val); ;
    break;}
case 12:
#line 180 "nnet/neural.y"
{ yyval.func_val = add_prefix_func (fu_next,0,yyvsp[-3].func_val,yyvsp[-1].func_val, make_tree (1)); ;
    break;}
case 13:
#line 181 "nnet/neural.y"
{ yyval.func_val = add_prefix_func (fu_next,0,add_prefix_func (fu_this,0,NULL),yyvsp[-3].func_val,yyvsp[-1].func_val); ;
    break;}
case 14:
#line 182 "nnet/neural.y"
{ yyval.func_val = add_prefix_func (fu_next,0,yyvsp[-3].func_val,make_tree (),yyvsp[-1].func_val); ;
    break;}
case 15:
#line 183 "nnet/neural.y"
{ yyval.func_val = add_prefix_func (fu_next,0,yyvsp[-1].func_val,make_tree (), make_tree(1)); ;
    break;}
case 16:
#line 184 "nnet/neural.y"
{ yyval.func_val = add_prefix_func (fu_next,0,add_prefix_func (fu_this,0,NULL),yyvsp[-1].func_val, make_tree(1)); ;
    break;}
case 17:
#line 185 "nnet/neural.y"
{ yyval.func_val = add_prefix_func (fu_next,0,add_prefix_func (fu_this,0,NULL),make_tree(), yyvsp[-1].func_val); ;
    break;}
case 18:
#line 186 "nnet/neural.y"
{ yyval.func_val = add_prefix_func (fu_next,0,add_prefix_func (fu_this,0,NULL),make_tree(), make_tree(1)); ;
    break;}
case 19:
#line 190 "nnet/neural.y"
{ yyval.func_val = add_prefix_func (fu_chartofloat,yyvsp[-3].string_val,yyvsp[-1].func_val); ;
    break;}
case 20:
#line 191 "nnet/neural.y"
{ yyval.func_val = add_infix_un_func (fu_not,yyvsp[0].func_val);	;
    break;}
case 21:
#line 192 "nnet/neural.y"
{ yyval.func_val = add_infix_bin_func (fu_or,yyvsp[-2].func_val,yyvsp[0].func_val); ;
    break;}
case 22:
#line 193 "nnet/neural.y"
{ yyval.func_val = add_infix_bin_func (fu_and,yyvsp[-2].func_val,yyvsp[0].func_val); ;
    break;}
case 23:
#line 194 "nnet/neural.y"
{ yyval.func_val = add_infix_bin_func (fu_equals,yyvsp[-2].func_val,yyvsp[0].func_val); ;
    break;}
case 24:
#line 195 "nnet/neural.y"
{ yyval.func_val = add_infix_bin_func (fu_notequals,yyvsp[-2].func_val,yyvsp[0].func_val); ;
    break;}
case 25:
#line 196 "nnet/neural.y"
{ yyval.func_val = add_infix_bin_func (fu_less,yyvsp[-2].func_val,yyvsp[0].func_val); ;
    break;}
case 26:
#line 197 "nnet/neural.y"
{ yyval.func_val = add_infix_bin_func (fu_lessorequals,yyvsp[-2].func_val,yyvsp[0].func_val); ;
    break;}
case 27:
#line 198 "nnet/neural.y"
{ yyval.func_val = add_infix_bin_func (fu_greater,yyvsp[-2].func_val,yyvsp[0].func_val); ;
    break;}
case 28:
#line 199 "nnet/neural.y"
{ yyval.func_val = add_infix_bin_func (fu_greaterorequals,yyvsp[-2].func_val,yyvsp[0].func_val); ;
    break;}
case 29:
#line 200 "nnet/neural.y"
{ yyval.func_val = add_infix_bin_func (fu_add,yyvsp[-2].func_val,yyvsp[0].func_val); ;
    break;}
case 30:
#line 201 "nnet/neural.y"
{ yyval.func_val = add_infix_bin_func (fu_subtract,yyvsp[-2].func_val,yyvsp[0].func_val); ;
    break;}
case 31:
#line 202 "nnet/neural.y"
{ yyval.func_val = add_infix_bin_func (fu_multiply,yyvsp[-2].func_val,yyvsp[0].func_val); ;
    break;}
case 32:
#line 203 "nnet/neural.y"
{ yyval.func_val = add_infix_bin_func (fu_divide,yyvsp[-2].func_val,yyvsp[0].func_val); ;
    break;}
case 33:
#line 204 "nnet/neural.y"
{ yyval.func_val = add_prefix_func (fu_neural,0,yyvsp[-3].func_val,yyvsp[-1].func_val); ;
    break;}
case 35:
#line 206 "nnet/neural.y"
{ yyval.func_val = yyvsp[-1].func_val; ;
    break;}
case 36:
#line 207 "nnet/neural.y"
{ yyval.func_val = make_tree (double (yyvsp[0].float_val));	;
    break;}
case 37:
#line 210 "nnet/neural.y"
{ yyval.func_val = add_prefix_func (fu_count,0,yyvsp[-3].func_val,yyvsp[-1].func_val); ;
    break;}
case 38:
#line 211 "nnet/neural.y"
{ yyval.func_val = add_prefix_func (fu_index,0,yyvsp[-3].func_val,yyvsp[-1].func_val); ;
    break;}
case 39:
#line 212 "nnet/neural.y"
{ yyval.func_val = add_prefix_func (fu_f0,0,yyvsp[-1].func_val); ;
    break;}
case 40:
#line 213 "nnet/neural.y"
{ yyval.func_val = add_prefix_func (fu_cont,0,yyvsp[-1].func_val); ;
    break;}
case 41:
#line 214 "nnet/neural.y"
{ yyval.func_val = yyvsp[-1].func_val; ;
    break;}
case 42:
#line 215 "nnet/neural.y"
{ yyval.func_val = make_tree (int (yyvsp[0].int_val)); ;
    break;}
case 43:
#line 217 "nnet/neural.y"
{ yyval.func_val = yyvsp[-1].func_val;;
    break;}
case 44:
#line 218 "nnet/neural.y"
{ yyval.func_val = make_tree (yyvsp[0].string_val); ;
    break;}
case 45:
#line 221 "nnet/neural.y"
{ 
				yyval.func_val = yyvsp[0].func_val;
				if (get_level(yyvsp[0].func_val->v()) == U_DEFAULT) shriek (812, fmt("Unknown level: %s", static_cast<const char *>(yyvsp[0].func_val->v()))); 
				yyvsp[0].func_val->v() = static_cast<char>(get_level (yyvsp[0].func_val->v()));
			;
    break;}
}

#line 705 "/usr/share/bison/bison.simple"


  yyvsp -= yylen;
  yyssp -= yylen;
#if YYLSP_NEEDED
  yylsp -= yylen;
#endif

#if YYDEBUG
  if (yydebug)
    {
      short *yyssp1 = yyss - 1;
      YYFPRINTF (stderr, "state stack now");
      while (yyssp1 != yyssp)
	YYFPRINTF (stderr, " %d", *++yyssp1);
      YYFPRINTF (stderr, "\n");
    }
#endif

  *++yyvsp = yyval;
#if YYLSP_NEEDED
  *++yylsp = yyloc;
#endif

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;

#ifdef YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
	{
	  YYSIZE_T yysize = 0;
	  char *yymsg;
	  int yyx, yycount;

	  yycount = 0;
	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  for (yyx = yyn < 0 ? -yyn : 0;
	       yyx < (int) (sizeof (yytname) / sizeof (char *)); yyx++)
	    if (yycheck[yyx + yyn] == yyx)
	      yysize += yystrlen (yytname[yyx]) + 15, yycount++;
	  yysize += yystrlen ("parse error, unexpected ") + 1;
	  yysize += yystrlen (yytname[YYTRANSLATE (yychar)]);
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "parse error, unexpected ");
	      yyp = yystpcpy (yyp, yytname[YYTRANSLATE (yychar)]);

	      if (yycount < 5)
		{
		  yycount = 0;
		  for (yyx = yyn < 0 ? -yyn : 0;
		       yyx < (int) (sizeof (yytname) / sizeof (char *));
		       yyx++)
		    if (yycheck[yyx + yyn] == yyx)
		      {
			const char *yyq = ! yycount ? ", expecting " : " or ";
			yyp = yystpcpy (yyp, yyq);
			yyp = yystpcpy (yyp, yytname[yyx]);
			yycount++;
		      }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    yyerror ("parse error; also virtual memory exhausted");
	}
      else
#endif /* defined (YYERROR_VERBOSE) */
	yyerror ("parse error");
    }
  goto yyerrlab1;


/*--------------------------------------------------.
| yyerrlab1 -- error raised explicitly by an action |
`--------------------------------------------------*/
yyerrlab1:
  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      /* return failure if at end of input */
      if (yychar == YYEOF)
	YYABORT;
      YYDPRINTF ((stderr, "Discarding token %d (%s).\n",
		  yychar, yytname[yychar1]));
      yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */

  yyerrstatus = 3;		/* Each real token shifted decrements this */

  goto yyerrhandle;


/*-------------------------------------------------------------------.
| yyerrdefault -- current state does not do anything special for the |
| error token.                                                       |
`-------------------------------------------------------------------*/
yyerrdefault:
#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */

  /* If its default is to accept any token, ok.  Otherwise pop it.  */
  yyn = yydefact[yystate];
  if (yyn)
    goto yydefault;
#endif


/*---------------------------------------------------------------.
| yyerrpop -- pop the current state because it cannot handle the |
| error token                                                    |
`---------------------------------------------------------------*/
yyerrpop:
  if (yyssp == yyss)
    YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#if YYLSP_NEEDED
  yylsp--;
#endif

#if YYDEBUG
  if (yydebug)
    {
      short *yyssp1 = yyss - 1;
      YYFPRINTF (stderr, "Error: state stack now");
      while (yyssp1 != yyssp)
	YYFPRINTF (stderr, " %d", *++yyssp1);
      YYFPRINTF (stderr, "\n");
    }
#endif

/*--------------.
| yyerrhandle.  |
`--------------*/
yyerrhandle:
  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yyerrdefault;

  yyn += YYTERROR;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != YYTERROR)
    goto yyerrdefault;

  yyn = yytable[yyn];
  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrpop;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrpop;

  if (yyn == YYFINAL)
    YYACCEPT;

  YYDPRINTF ((stderr, "Shifting error token, "));

  *++yyvsp = yylval;
#if YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

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

/*---------------------------------------------.
| yyoverflowab -- parser overflow comes here.  |
`---------------------------------------------*/
yyoverflowlab:
  yyerror ("parser stack overflow");
  yyresult = 2;
  /* Fall through.  */

yyreturn:
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}
#line 228 "nnet/neural.y"

     

#include <ctype.h>


/* * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                     YYERROR                       */
/* * * * * * * * * * * * * * * * * * * * * * * * * * */

int yyerror (char *s)
{
	shriek (812, fmt ("BISON:yyerror:bison_nnet parser: bison_row %i (not counting empty rows) '%s' is erroneous. %s\n", row_num, bison_row_buf, s));
	return -1;
}
 

/* * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                      YYLEX                        */
/* * * * * * * * * * * * * * * * * * * * * * * * * * */

int yylex ()
{
	char lex [MSG_LENGTH];

	D_PRINT(0, "yylex source: %s\n", bison_row);

	if (*bison_row == '#') 
		return 0; // comment line - EOF

	char *cross = const_cast<char *>(bison_row);
	while (cross = strchr (cross, '#')) {
		cross --;
		if (strchr (" \t", *cross))
			*cross = '\x0';
		else cross += 2;
	}

	//jump over whitespace
	while (*bison_row && strchr (" \t\n",*bison_row)) ++bison_row;

	//simple symbol
	if (strchr ("[]{}(),;+-*/$",*bison_row)) {
		return *bison_row++;
	}

	const char *end;

	//quoted string - return without quotas
	if (*bison_row == '\"') {
	   end = strchr (bison_row+1,'\"');
	   if (!*end) shriek (812, fmt ("BISON:yylex:Unterminated quotas! %s\n", bison_row));
		strcpy (yylval.string_val, bison_row+1);
		yylval.string_val [end-bison_row-1]=0;
		bison_row = end + 1;
		return QUOTED_STRING;
   }

	//two-chars-symbols (like ==,<= etc.) and function names
	
	if (strchr ("!=<>&", *bison_row)) { //these chars may form operators like != or <=
		for (end = bison_row; strchr ("!=<>&", *end); ++end);
	  	if (end-bison_row == 1) 
	  	 	return *bison_row++;
	}
	else {
	  end = strpbrk (bison_row, "![]={}(),& \t\n;+-*/><$");
	  if (!end) end = bison_row + strlen (bison_row);
	}


	if (end-bison_row > MSG_LENGTH) shriek (812,"BISON:yylex:Buffer overflow");
	strncpy (lex, bison_row, end-bison_row);
	lex [end-bison_row] = 0;
	bison_row = end;

	//try to find the grammar symbol 
	for (int i = 0; i < YYNTOKENS; i++) {
		if (yytname[i] != 0
		    && yytname[i][0] == '"'
			 && !strncmp (yytname[i] + 1, lex,
							 strlen (lex))
			 && yytname[i][strlen (lex) + 1] == '"'
			 && yytname[i][strlen (lex) + 2] == 0)
		  return yytoknum [i]; //this is an undocumented feature: you must use yytoknum
	}


	//not a grammar symbol - a number or chartofloat name or error
	yylval.int_val = strtol (lex, const_cast<char **>(&end), 10);
	if (*end == 0 && errno != ERANGE) 
		return INT_NUM;
	
	yylval.float_val = strtod (lex, const_cast<char **>(&end));
	if (*end == 0 && errno != ERANGE)
		return FLOAT_NUM;

	strcpy (yylval.string_val, lex);
	return STRING;
}
	
 
