#ifndef BISON_Y_TAB_H
# define BISON_Y_TAB_H

#ifndef YYSTYPE
typedef union {
	int int_val;
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
# define	COUNT	268
# define	INDEX	269
# define	THIS	270
# define	ANCESTOR	271
# define	PREV	272
# define	NEXT	273
# define	NEURAL	274
# define	BASAL_F	275
# define	CONT	276


extern YYSTYPE yylval;

#endif /* not BISON_Y_TAB_H */
