

















































#include "nsWildCard.h"
#include "nsXPCOM.h"
#include "nsCRTGlue.h"
#include "nsCharTraits.h"



typedef int static_assert_character_code_arrangement['a' > 'A' ? 1 : -1];

template<class T>
static int
alpha(T c)
{
    return ('a' <= c && c <= 'z') ||
           ('A' <= c && c <= 'Z');
}

template<class T>
static int
alphanumeric(T c)
{
    return ('0' <= c && c <= '9') || alpha(c);
}

template<class T>
static int
lower(T c)
{
    return ('A' <= c && c <= 'Z') ? c + ('a' - 'A') : c;
}

template<class T>
static int
upper(T c)
{
    return ('a' <= c && c <= 'z') ? c - ('a' - 'A') : c;
}



template<class T>
static int
_valid_subexp(const T *expr, T stop1, T stop2)
{
    register int x;
    int nsc = 0;     
    int np;          
    int tld = 0;     
 
    for (x = 0; expr[x] && (expr[x] != stop1) && (expr[x] != stop2); ++x) {
        switch(expr[x]) {
        case '~':
            if(tld)                 
                return INVALID_SXP;
            if (stop1)              
                return INVALID_SXP;
            if (!expr[x+1])          
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
            if((!expr[++x]) || (expr[x] == ']'))
                return INVALID_SXP;
            for(; expr[x] && (expr[x] != ']'); ++x) {
                if(expr[x] == '\\' && !expr[++x])
                    return INVALID_SXP;
            }
            if(!expr[x])
                return INVALID_SXP;
            break;
        case '(':
            ++nsc;
            if (stop1)                  
                return INVALID_SXP;
            np = -1;
            do {
                int t = ::_valid_subexp(&expr[++x], T(')'), T('|'));
                if(t == 0 || t == INVALID_SXP)
                    return INVALID_SXP;
                x+=t;
                if(!expr[x])
                    return INVALID_SXP;
                ++np;
            } while (expr[x] == '|' );
            if(np < 1)  
                return INVALID_SXP;
            break;
        case ')':
        case ']':
        case '|':
            return INVALID_SXP;
        case '\\':
            ++nsc;
            if(!expr[++x])
                return INVALID_SXP;
            break;
        default:
            break;
        }
    }
    if((!stop1) && (!nsc)) 
         return NON_SXP;
    return ((expr[x] == stop1 || expr[x] == stop2) ? x : INVALID_SXP);
}


template<class T>
int
NS_WildCardValid_(const T *expr)
{
    int x = ::_valid_subexp(expr, T('\0'), T('\0'));
    return (x < 0 ? x : VALID_SXP);
}

NS_COM int
NS_WildCardValid(const char *expr)
{
    return NS_WildCardValid_(expr);
}

NS_COM int
NS_WildCardValid(const PRUnichar *expr)
{
    return NS_WildCardValid_(expr);
}




#define MATCH 0
#define NOMATCH 1
#define ABORTED -1

template<class T>
static int
_shexp_match(const T *str, const T *expr, PRBool case_insensitive, unsigned int level);










template<class T>
static int
_scan_and_copy(const T *expr, T stop1, T stop2, T *dest)
{
    register int sx;     
    register char cc;

    for (sx = 0; (cc = expr[sx]) && cc != stop1 && cc != stop2; sx++) {
        if (cc == '\\') {
            if (!expr[++sx])
                return ABORTED; 
        }
        else if (cc == '[') {
            while ((cc = expr[++sx]) && cc != ']') {
                if(cc == '\\' && !expr[++sx])
                    return ABORTED;
            }
            if (!cc)
                return ABORTED; 
        }
    }
    if (dest && sx) {
        
        memcpy(dest, expr, sx * sizeof(T));
        dest[sx] = 0;
    }
    return cc ? sx : ABORTED; 
}









template<class T>
static int
_handle_union(const T *str, const T *expr, PRBool case_insensitive,
              unsigned int level)
{
    register int sx;     
    int cp;              
    int count;
    int ret   = NOMATCH;
    T *e2;

    
    cp = ::_scan_and_copy(expr, T(')'), T('\0'), static_cast<T*>(NULL));
    if (cp == ABORTED || cp < 4) 
        return ABORTED;
    ++cp;                
    e2 = (T *) NS_Alloc((1 + nsCharTraits<T>::length(expr)) * sizeof(T));
    if (!e2)
        return ABORTED;
    for (sx = 1; ; ++sx) {
        
        
        count = ::_scan_and_copy(expr + sx, T(')'), T('|'), e2);
        if (count == ABORTED || !count) {
            ret = ABORTED;
            break;
        }
        sx += count;
        
        nsCharTraits<T>::copy(e2 + count, expr + cp, nsCharTraits<T>::length(expr + cp) + 1);
        ret = ::_shexp_match(str, e2, case_insensitive, level + 1);
        if (ret != NOMATCH || !expr[sx] || expr[sx] == ')')
            break;
    }
    NS_Free(e2);
    if (sx < 2)
        ret = ABORTED;
    return ret;
}


static int
_is_char_in_range(unsigned char start, unsigned char end, unsigned char val)
{
    char map[256];
    memset(map, 0, sizeof map);
    while (start <= end)
        map[lower(start++)] = 1;
    return map[lower(val)];
}

template<class T>
static int
_shexp_match(const T *str, const T *expr, PRBool case_insensitive,
             unsigned int level)
{
    register int x;   
    register int y;   
    int ret,neg;

    if (level > 20)      
        return ABORTED;
    for(x = 0, y = 0; expr[y]; ++y, ++x) {
        if((!str[x]) && (expr[y] != '$') && (expr[y] != '*')) {
            return NOMATCH;
        }
        switch(expr[y]) {
        case '$':
            if(str[x])
                return NOMATCH;
            --x;                 
            break;
        case '*':
            while(expr[++y] == '*'){}
            if(!expr[y])
                return MATCH;
            while(str[x]) {
                ret = ::_shexp_match(&str[x++], &expr[y], case_insensitive,
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
            if((expr[y] == '$') && (expr[y+1] == '\0') && (!str[x]))
                return MATCH;
            else
                return NOMATCH;
        case '[': {
            T start, end = 0;
            int i;
            neg = ((expr[++y] == '^') && (expr[y+1] != ']'));
            if (neg)
                ++y;
            i = y;
            start = expr[i++];
            if (start == '\\')
                start = expr[i++];
            if (alphanumeric(start) && expr[i++] == '-') {
                end = expr[i++];
                if (end == '\\')
                    end = expr[i++];
            }
            if (alphanumeric(end) && expr[i] == ']') {
                
                T val = str[x];
                if (end < start) { 
                    T tmp = end;
                    end = start;
                    start = tmp;
                }
                if (case_insensitive && alpha(val)) {
                    val = ::_is_char_in_range((unsigned char) start,
                                              (unsigned char) end,
                                              (unsigned char) val);
                    if (neg == val)
                        return NOMATCH;
                }
                else if (neg != ((val < start) || (val > end))) {
                    return NOMATCH;
                }
                y = i;
            }
            else {
                
                int matched = 0;
                for (; expr[y] != ']'; y++) {
                    if (expr[y] == '\\')
                        ++y;
                    if(case_insensitive)
                        matched |= (upper(str[x]) == upper(expr[y]));
                    else
                        matched |= (str[x] == expr[y]);
                }
                if (neg == matched)
                    return NOMATCH;
            }
        }
        break;
        case '(':
            if (!expr[y+1])
                return ABORTED;
            return ::_handle_union(&str[x], &expr[y], case_insensitive, level + 1);
        case '?':
            break;
        case ')':
        case ']':
        case '|':
            return ABORTED;
        case '\\':
            ++y;
            
        default:
            if(case_insensitive) {
                if(upper(str[x]) != upper(expr[y]))
                    return NOMATCH;
            }
            else {
                if(str[x] != expr[y])
                    return NOMATCH;
            }
            break;
        }
    }
    return (str[x] ? NOMATCH : MATCH);
}


template<class T>
static int
ns_WildCardMatch(const T *str, const T *xp, PRBool case_insensitive)
{
    T *expr = NULL;
    int x, ret = MATCH;

    if (!nsCharTraits<T>::find(xp, nsCharTraits<T>::length(xp), T('~')))
        return _shexp_match(str, xp, case_insensitive, 0);

    expr = (T *) NS_Alloc((nsCharTraits<T>::length(xp) + 1) * sizeof(T));
    memcpy(expr, xp, (nsCharTraits<T>::length(xp) + 1) * sizeof(T));
    if(!expr)
        return NOMATCH;

    x = ::_scan_and_copy(expr, T('~'), T('\0'), static_cast<T*>(NULL));
    if (x != ABORTED && expr[x] == '~') {
        expr[x++] = '\0';
        ret = ::_shexp_match(str, &expr[x], case_insensitive, 0);
        switch (ret) {
        case NOMATCH: ret = MATCH;   break;
        case MATCH:   ret = NOMATCH; break;
        default:                     break;
        }
    }
    if (ret == MATCH)
        ret = ::_shexp_match(str, expr, case_insensitive, 0);

    NS_Free(expr);
    return ret;
}

template<class T>
NS_COM int
NS_WildCardMatch_(const T *str, const T *expr, PRBool case_insensitive)
{
    int is_valid = NS_WildCardValid(expr);
    switch(is_valid) {
        case INVALID_SXP:
            return -1;
        default:
            return ns_WildCardMatch(str, expr, case_insensitive);
    }
}

NS_COM int
NS_WildCardMatch(const char *str, const char *xp,
                 PRBool case_insensitive)
{
    return NS_WildCardMatch_(str, xp, case_insensitive);
}

NS_COM int
NS_WildCardMatch(const PRUnichar *str, const PRUnichar *xp,
                 PRBool case_insensitive)
{
    return NS_WildCardMatch_(str, xp, case_insensitive);
}
