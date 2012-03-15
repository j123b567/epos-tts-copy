typedef union {
	char *string_val;
	CXml *xml_val;
	CXml::TAttr *attr_val;
} YYSTYPE;
#define	STRING	258
#define	QUOTED_STRING	259
#define	COMMENT_STRING	260
#define	PCDATA_STRING	261
#define	SPACE_STRING	262


extern YYSTYPE xmllval;
