












































#include "seccomon.h"
#include "portreg.h"




static int 
_valid_subexp(const char *exp, char stop1, char stop2) 
{
    register int x;
    int nsc = 0;     
    int np;          
    int tld = 0;     

    for (x = 0; exp[x] && (exp[x] != stop1) && (exp[x] != stop2); ++x) {
        switch(exp[x]) {
	case '~':
            if(tld)                 
                return INVALID_SXP;
            if (stop1)              
                return INVALID_SXP;
            if (!exp[x+1])          
                return INVALID_SXP;
            if (!x)                 
                return INVALID_SXP;
            ++tld;
	    
	case '*':
	case '?':
	case '$':
            ++nsc;
            break;
	case '[':
            ++nsc;
            if((!exp[++x]) || (exp[x] == ']'))
                return INVALID_SXP;
            for(; exp[x] && (exp[x] != ']'); ++x) {
                if(exp[x] == '\\' && !exp[++x])
                    return INVALID_SXP;
            }
            if(!exp[x])
                return INVALID_SXP;
            break;
	case '(':
            ++nsc;
	    if (stop1)			
		return INVALID_SXP;
            np = -1;
            do {
                int t = _valid_subexp(&exp[++x], ')', '|');
                if(t == 0 || t == INVALID_SXP)
                    return INVALID_SXP;
                x+=t;
		if(!exp[x])
		    return INVALID_SXP;
		++np;
            } while (exp[x] == '|' );
	    if(np < 1)  
		return INVALID_SXP;
            break;
	case ')':
	case '|':
	case ']':
            return INVALID_SXP;
	case '\\':
	    ++nsc;
            if(!exp[++x])
                return INVALID_SXP;
            break;
	default:
            break;
        }
    }
    if((!stop1) && (!nsc)) 
        return NON_SXP;
    return ((exp[x] == stop1 || exp[x] == stop2) ? x : INVALID_SXP);
}

int 
PORT_RegExpValid(const char *exp) 
{
    int x;

    x = _valid_subexp(exp, '\0', '\0');
    return (x < 0 ? x : VALID_SXP);
}





#define MATCH 0
#define NOMATCH 1
#define ABORTED -1

static int 
_shexp_match(const char *str, const char *exp, PRBool case_insensitive,
             unsigned int level);









static int 
_scan_and_copy(const char *exp, char stop1, char stop2, char *dest)
{
    register int sx;     
    register char cc;

    for (sx = 0; (cc = exp[sx]) && cc != stop1 && cc != stop2; sx++) {
	if (cc == '\\') {
	    if (!exp[++sx])
		return ABORTED; 
	} else if (cc == '[') {
	    while ((cc = exp[++sx]) && cc != ']') {
		if(cc == '\\' && !exp[++sx])
		    return ABORTED;
	    }
	    if (!cc) 
		return ABORTED; 
	}
    }
    if (dest && sx) {
	
	memcpy(dest, exp, sx);
	dest[sx] = 0;
    }
    return cc ? sx : ABORTED; 
}









static int 
_handle_union(const char *str, const char *exp, PRBool case_insensitive,
              unsigned int level) 
{
    register int sx;     
    int cp;              
    int count;
    int ret   = NOMATCH;
    char *e2;

    
    cp = _scan_and_copy(exp, ')', '\0', NULL);
    if (cp == ABORTED || cp < 4) 
    	return ABORTED;
    ++cp;                
    e2 = (char *) PORT_Alloc(1 + strlen(exp));
    if (!e2)
    	return ABORTED;
    for (sx = 1; ; ++sx) {
	
	
	count = _scan_and_copy(exp + sx, ')', '|', e2);
	if (count == ABORTED || !count) {
	    ret = ABORTED;
	    break;
	}
	sx += count;
	
	strcpy(e2+count, exp+cp);
        ret = _shexp_match(str, e2, case_insensitive, level + 1);
	if (ret != NOMATCH || !exp[sx] || exp[sx] == ')')
            break;
    }
    PORT_Free(e2);
    if (sx < 2)
    	ret = ABORTED;
    return ret;
}


static int
_is_char_in_range(int start, int end, int val)
{
    char map[256];
    memset(map, 0, sizeof map);
    while (start <= end)
	map[tolower(start++)] = 1;
    return map[tolower(val)];
}

static int 
_shexp_match(const char *str, const char *exp, PRBool case_insensitive, 
             unsigned int level) 
{
    register int x;   
    register int y;   
    int ret,neg;

    if (level > 20)      
    	return ABORTED;
    for(x = 0, y = 0; exp[y]; ++y, ++x) {
        if((!str[x]) && (exp[y] != '$') && (exp[y] != '*')) {
            return NOMATCH;
	}
	switch(exp[y]) {
	case '$':
	    if(str[x])
		return NOMATCH;
	    --x;                 
	    break;
	case '*':
	    while(exp[++y] == '*'){}
	    if(!exp[y])
		return MATCH;
	    while(str[x]) {
	        ret = _shexp_match(&str[x++], &exp[y], case_insensitive, 
				   level + 1);
		switch(ret) {
		case NOMATCH:
		    continue;
		case ABORTED:
		    return ABORTED;
		default:
		    return MATCH;
		}
	    }
	    if((exp[y] == '$') && (exp[y+1] == '\0') && (!str[x]))
		return MATCH;
	    else
		return NOMATCH;
	case '[': {
	    int start, end = 0, i;
	    neg = ((exp[++y] == '^') && (exp[y+1] != ']'));
	    if (neg)
		++y;
	    i = y;
	    start = (unsigned char)(exp[i++]);
	    if (start == '\\')
	    	start = (unsigned char)(exp[i++]);
	    if (isalnum(start) && exp[i++] == '-') {
		end = (unsigned char)(exp[i++]); 
		if (end == '\\')
		    end = (unsigned char)(exp[i++]);
	    }
	    if (isalnum(end) && exp[i] == ']') {
		
		int val   = (unsigned char)(str[x]);
		if (end < start) { 
		    start ^= end;
		    end ^= start;
		    start ^= end;
		}
		if (case_insensitive && isalpha(val)) {
		    val = _is_char_in_range(start, end, val);
		    if (neg == val)
			return NOMATCH;
		} else if (neg != ((val < start) || (val > end))) {
		    return NOMATCH;
		}
		y = i;
	    } else {
		
		int matched = 0;
		for (; exp[y] != ']'; y++) {
		    if (exp[y] == '\\')
			++y;
		    if(case_insensitive) {
			matched |= (toupper(str[x]) == toupper(exp[y]));
		    } else {
			matched |= (str[x] == exp[y]);
		    }
		}
		if (neg == matched)
		    return NOMATCH;
	    }
	}
	break;
	case '(':
	    if (!exp[y+1])
	    	return ABORTED;
	    return _handle_union(&str[x], &exp[y], case_insensitive, level);
	case '?':
	    break;
	case '|':
	case ']':
	case ')':
	    return ABORTED;
	case '\\':
	    ++y;
	    
	default:
	    if(case_insensitive) {
		if(toupper(str[x]) != toupper(exp[y]))
		    return NOMATCH;
	    } else {
		if(str[x] != exp[y])
		    return NOMATCH;
	    }
	    break;
	}
    }
    return (str[x] ? NOMATCH : MATCH);
}

static int 
port_RegExpMatch(const char *str, const char *xp, PRBool case_insensitive) 
{
    char *exp = 0;
    int x, ret = MATCH;

    if (!strchr(xp, '~'))
    	return _shexp_match(str, xp, case_insensitive, 0);

    exp = PORT_Strdup(xp);
    if(!exp)
	return NOMATCH;

    x = _scan_and_copy(exp, '~', '\0', NULL);
    if (x != ABORTED && exp[x] == '~') {
	exp[x++] = '\0';
	ret = _shexp_match(str, &exp[x], case_insensitive, 0);
	switch (ret) {
	case NOMATCH: ret = MATCH;   break;
	case MATCH:   ret = NOMATCH; break;
	default:                     break;
        }
    }
    if (ret == MATCH)
	ret = _shexp_match(str, exp, case_insensitive, 0);

    PORT_Free(exp);
    return ret;
}




int 
PORT_RegExpSearch(const char *str, const char *exp)
{
    switch(PORT_RegExpValid(exp)) 
	  {
        case INVALID_SXP:
            return -1;
        case NON_SXP:
            return (strcmp(exp,str) ? 1 : 0);
        default:
            return port_RegExpMatch(str, exp, PR_FALSE);
      }
}

int
PORT_RegExpCaseSearch(const char *str, const char *exp)
{
    switch(PORT_RegExpValid(exp))
      {
        case INVALID_SXP:
            return -1;
        case NON_SXP:
            return (PORT_Strcasecmp(exp,str) ? 1 : 0);
        default:
            return port_RegExpMatch(str, exp, PR_TRUE);
      }
}

