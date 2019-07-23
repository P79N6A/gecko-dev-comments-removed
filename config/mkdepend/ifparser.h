




























































#include <stdio.h>

typedef int Bool;
#define False 0
#define True 1

typedef struct _if_parser {
    struct {				
	const char *(*handle_error) (struct _if_parser *, const char *,
				     const char *);
	long (*eval_variable) (struct _if_parser *, const char *, int);
	int (*eval_defined) (struct _if_parser *, const char *, int);
    } funcs;
    char *data;
} IfParser;

const char *ParseIfExpression (
    IfParser *, 
    const char *, 
    long *
);

