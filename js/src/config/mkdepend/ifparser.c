






























































#include "ifparser.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>





#define DO(val) if (!(val)) return NULL
#define CALLFUNC(ggg,fff) (*((ggg)->funcs.fff))
#define SKIPSPACE(ccc) while (isspace(*ccc)) ccc++
#define isvarfirstletter(ccc) (isalpha(ccc) || (ccc) == '_')


static const char *
parse_variable (IfParser *g, const char *cp, const char **varp)
{
    SKIPSPACE (cp);

    if (!isvarfirstletter (*cp))
	return CALLFUNC(g, handle_error) (g, cp, "variable name");

    *varp = cp;
    
    for (cp++; isalnum(*cp) || *cp == '_'; cp++) ;
    return cp;
}


static const char *
parse_number (IfParser *g, const char *cp, long *valp)
{
    long base = 10;
    SKIPSPACE (cp);

    if (!isdigit(*cp))
	return CALLFUNC(g, handle_error) (g, cp, "number");

    *valp = 0;

    if (*cp == '0') {
	cp++;
	if ((*cp == 'x') || (*cp == 'X')) {
	    base = 16;
	    cp++;
	} else {
	    base = 8;
	}
    }

    
    while (1) {
	int increment = -1;
	if (base == 8) {
	    if ((*cp >= '0') && (*cp <= '7'))
		increment = *cp++ - '0';
	} else if (base == 16) {
	    if ((*cp >= '0') && (*cp <= '9'))
		increment = *cp++ - '0';
	    else if ((*cp >= 'A') &&  (*cp <= 'F'))
		increment = *cp++ - ('A' - 10);
	    else if ((*cp >= 'a') && (*cp <= 'f'))
		increment = *cp++ - ('a' - 10);
	} else {	
	    if ((*cp >= '0') && (*cp <= '9'))
		increment = *cp++ - '0';
	}
	if (increment < 0)
	    break;
	*valp = (*valp * base) + increment;
    }

    
    while (*cp == 'U' || *cp == 'u' || *cp == 'L' || *cp == 'l') cp++;
    return cp;
}

static const char *
parse_character (IfParser *g, const char *cp, long *valp)
{
    char val;

    SKIPSPACE (cp);
    if (*cp == '\\')
	switch (cp[1]) {
	case 'n': val = '\n'; break;
	case 't': val = '\t'; break;
	case 'v': val = '\v'; break;
	case 'b': val = '\b'; break;
	case 'r': val = '\r'; break;
	case 'f': val = '\f'; break;
	case 'a': val = '\a'; break;
	case '\\': val = '\\'; break;
	case '?': val = '\?'; break;
	case '\'': val = '\''; break;
	case '\"': val = '\"'; break;
	case 'x': val = (char) strtol (cp + 2, NULL, 16); break;
	default: val = (char) strtol (cp + 1, NULL, 8); break;
	}
    else
	val = *cp;
    while (*cp != '\'') cp++;
    *valp = (long) val;
    return cp;
}

static const char *
parse_value (IfParser *g, const char *cp, long *valp)
{
    const char *var, *varend;

    *valp = 0;

    SKIPSPACE (cp);
    if (!*cp)
	return cp;

    switch (*cp) {
      case '(':
	DO (cp = ParseIfExpression (g, cp + 1, valp));
	SKIPSPACE (cp);
	if (*cp != ')') 
	    return CALLFUNC(g, handle_error) (g, cp, ")");

	return cp + 1;			

      case '!':
	DO (cp = parse_value (g, cp + 1, valp));
	*valp = !(*valp);
	return cp;

      case '-':
	DO (cp = parse_value (g, cp + 1, valp));
	*valp = -(*valp);
	return cp;

      case '+':
	DO (cp = parse_value (g, cp + 1, valp));
	return cp;

      case '~':
	DO (cp = parse_value (g, cp + 1, valp));
	*valp = ~(*valp);
	return cp;

      case '#':
	DO (cp = parse_variable (g, cp + 1, &var));
	SKIPSPACE (cp);
	if (*cp != '(')
	    return CALLFUNC(g, handle_error) (g, cp, "(");
	do {
	    DO (cp = parse_variable (g, cp + 1, &var));
	    SKIPSPACE (cp);
	} while (*cp && *cp != ')');
	if (*cp != ')')
	    return CALLFUNC(g, handle_error) (g, cp, ")");
	*valp = 1; 
	return cp + 1;

      case '\'':
	DO (cp = parse_character (g, cp + 1, valp));
	if (*cp != '\'')
	    return CALLFUNC(g, handle_error) (g, cp, "'");
	return cp + 1;

      case 'd':
	if (strncmp (cp, "defined", 7) == 0 && !isalnum(cp[7])) {
	    int paren = 0;
	    int len;

	    cp += 7;
	    SKIPSPACE (cp);
	    if (*cp == '(') {
		paren = 1;
		cp++;
	    }
	    DO (cp = parse_variable (g, cp, &var));
	    len = cp - var;
	    SKIPSPACE (cp);
	    if (paren && *cp != ')')
		return CALLFUNC(g, handle_error) (g, cp, ")");
	    *valp = (*(g->funcs.eval_defined)) (g, var, len);
	    return cp + paren;		
	}
	
    }

    if (isdigit(*cp)) {
	DO (cp = parse_number (g, cp, valp));
    } else if (!isvarfirstletter(*cp))
	return CALLFUNC(g, handle_error) (g, cp, "variable or number");
    else {
	DO (cp = parse_variable (g, cp, &var));
	varend = cp;
	SKIPSPACE(cp);
	if (*cp != '(') {
	    *valp = (*(g->funcs.eval_variable)) (g, var, varend - var);
	} else {
	    do {
		long dummy;
		DO (cp = ParseIfExpression (g, cp + 1, &dummy));
		SKIPSPACE(cp);
		if (*cp == ')')
		    break;
		if (*cp != ',')
		    return CALLFUNC(g, handle_error) (g, cp, ",");
	    } while (1);

	    *valp = 1;	
	    cp++;
	}
    }
    
    return cp;
}



static const char *
parse_product (IfParser *g, const char *cp, long *valp)
{
    long rightval;

    DO (cp = parse_value (g, cp, valp));
    SKIPSPACE (cp);

    switch (*cp) {
      case '*':
	DO (cp = parse_product (g, cp + 1, &rightval));
	*valp = (*valp * rightval);
	break;

      case '/':
	DO (cp = parse_product (g, cp + 1, &rightval));
	*valp = (*valp / rightval);
	break;

      case '%':
	DO (cp = parse_product (g, cp + 1, &rightval));
	*valp = (*valp % rightval);
	break;
    }
    return cp;
}


static const char *
parse_sum (IfParser *g, const char *cp, long *valp)
{
    long rightval;

    DO (cp = parse_product (g, cp, valp));
    SKIPSPACE (cp);

    switch (*cp) {
      case '+':
	DO (cp = parse_sum (g, cp + 1, &rightval));
	*valp = (*valp + rightval);
	break;

      case '-':
	DO (cp = parse_sum (g, cp + 1, &rightval));
	*valp = (*valp - rightval);
	break;
    }
    return cp;
}


static const char *
parse_shift (IfParser *g, const char *cp, long *valp)
{
    long rightval;

    DO (cp = parse_sum (g, cp, valp));
    SKIPSPACE (cp);

    switch (*cp) {
      case '<':
	if (cp[1] == '<') {
	    DO (cp = parse_shift (g, cp + 2, &rightval));
	    *valp = (*valp << rightval);
	}
	break;

      case '>':
	if (cp[1] == '>') {
	    DO (cp = parse_shift (g, cp + 2, &rightval));
	    *valp = (*valp >> rightval);
	}
	break;
    }
    return cp;
}


static const char *
parse_inequality (IfParser *g, const char *cp, long *valp)
{
    long rightval;

    DO (cp = parse_shift (g, cp, valp));
    SKIPSPACE (cp);

    switch (*cp) {
      case '<':
	if (cp[1] == '=') {
	    DO (cp = parse_inequality (g, cp + 2, &rightval));
	    *valp = (*valp <= rightval);
	} else {
	    DO (cp = parse_inequality (g, cp + 1, &rightval));
	    *valp = (*valp < rightval);
	}
	break;

      case '>':
	if (cp[1] == '=') {
	    DO (cp = parse_inequality (g, cp + 2, &rightval));
	    *valp = (*valp >= rightval);
	} else {
	    DO (cp = parse_inequality (g, cp + 1, &rightval));
	    *valp = (*valp > rightval);
	}
	break;
    }
    return cp;
}


static const char *
parse_equality (IfParser *g, const char *cp, long *valp)
{
    long rightval;

    DO (cp = parse_inequality (g, cp, valp));
    SKIPSPACE (cp);

    switch (*cp) {
      case '=':
	if (cp[1] == '=')
	    cp++;
	DO (cp = parse_equality (g, cp + 1, &rightval));
	*valp = (*valp == rightval);
	break;

      case '!':
	if (cp[1] != '=')
	    break;
	DO (cp = parse_equality (g, cp + 2, &rightval));
	*valp = (*valp != rightval);
	break;
    }
    return cp;
}


static const char *
parse_band (IfParser *g, const char *cp, long *valp)
{
    long rightval;

    DO (cp = parse_equality (g, cp, valp));
    SKIPSPACE (cp);

    switch (*cp) {
      case '&':
	if (cp[1] != '&') {
	    DO (cp = parse_band (g, cp + 1, &rightval));
	    *valp = (*valp & rightval);
	}
	break;
    }
    return cp;
}


static const char *
parse_bxor (IfParser *g, const char *cp, long *valp)
{
    long rightval;

    DO (cp = parse_band (g, cp, valp));
    SKIPSPACE (cp);

    switch (*cp) {
      case '^':
	DO (cp = parse_bxor (g, cp + 1, &rightval));
	*valp = (*valp ^ rightval);
	break;
    }
    return cp;
}


static const char *
parse_bor (IfParser *g, const char *cp, long *valp)
{
    long rightval;

    DO (cp = parse_bxor (g, cp, valp));
    SKIPSPACE (cp);

    switch (*cp) {
      case '|':
	if (cp[1] != '|') {
	    DO (cp = parse_bor (g, cp + 1, &rightval));
	    *valp = (*valp | rightval);
	}
	break;
    }
    return cp;
}


static const char *
parse_land (IfParser *g, const char *cp, long *valp)
{
    long rightval;

    DO (cp = parse_bor (g, cp, valp));
    SKIPSPACE (cp);

    switch (*cp) {
      case '&':
	if (cp[1] != '&')
	    return CALLFUNC(g, handle_error) (g, cp, "&&");
	DO (cp = parse_land (g, cp + 2, &rightval));
	*valp = (*valp && rightval);
	break;
    }
    return cp;
}


static const char *
parse_lor (IfParser *g, const char *cp, long *valp)
{
    long rightval;

    DO (cp = parse_land (g, cp, valp));
    SKIPSPACE (cp);

    switch (*cp) {
      case '|':
	if (cp[1] != '|')
	    return CALLFUNC(g, handle_error) (g, cp, "||");
	DO (cp = parse_lor (g, cp + 2, &rightval));
	*valp = (*valp || rightval);
	break;
    }
    return cp;
}


static const char *
parse_cond(IfParser *g, const char *cp, long *valp)
{
    long trueval, falseval;

    DO (cp = parse_lor (g, cp, valp));
    SKIPSPACE (cp);

    switch (*cp) {
      case '?':
	DO (cp = parse_cond (g, cp + 1, &trueval));
	SKIPSPACE (cp);
	if (*cp != ':')
	    return CALLFUNC(g, handle_error) (g, cp, ":");
	DO (cp = parse_cond (g, cp + 1, &falseval));
	*valp = (*valp ? trueval : falseval);
	break;
    }
    return cp;
}






const char *
ParseIfExpression (IfParser *g, const char *cp, long *valp)
{
    return parse_cond (g, cp, valp);
}
