

















#include "nsWildCard.h"
#include "nsXPCOM.h"
#include "nsCRTGlue.h"
#include "nsCharTraits.h"



typedef int static_assert_character_code_arrangement['a' > 'A' ? 1 : -1];

template<class T>
static int
alpha(T aChar)
{
  return ('a' <= aChar && aChar <= 'z') ||
         ('A' <= aChar && aChar <= 'Z');
}

template<class T>
static int
alphanumeric(T aChar)
{
  return ('0' <= aChar && aChar <= '9') || ::alpha(aChar);
}

template<class T>
static int
lower(T aChar)
{
  return ('A' <= aChar && aChar <= 'Z') ? aChar + ('a' - 'A') : aChar;
}

template<class T>
static int
upper(T aChar)
{
  return ('a' <= aChar && aChar <= 'z') ? aChar - ('a' - 'A') : aChar;
}



template<class T>
static int
_valid_subexp(const T* aExpr, T aStop1, T aStop2)
{
  int x;
  int nsc = 0;     
  int np;          
  int tld = 0;     

  for (x = 0; aExpr[x] && (aExpr[x] != aStop1) && (aExpr[x] != aStop2); ++x) {
    switch (aExpr[x]) {
      case '~':
        if (tld) {              
          return INVALID_SXP;
        }
        if (aStop1) {           
          return INVALID_SXP;
        }
        if (!aExpr[x + 1]) {    
          return INVALID_SXP;
        }
        if (!x) {               
          return INVALID_SXP;
        }
        ++tld;
      
      case '*':
      case '?':
      case '$':
        ++nsc;
        break;
      case '[':
        ++nsc;
        if ((!aExpr[++x]) || (aExpr[x] == ']')) {
          return INVALID_SXP;
        }
        for (; aExpr[x] && (aExpr[x] != ']'); ++x) {
          if (aExpr[x] == '\\' && !aExpr[++x]) {
            return INVALID_SXP;
          }
        }
        if (!aExpr[x]) {
          return INVALID_SXP;
        }
        break;
      case '(':
        ++nsc;
        if (aStop1) {           
          return INVALID_SXP;
        }
        np = -1;
        do {
          int t = ::_valid_subexp(&aExpr[++x], T(')'), T('|'));
          if (t == 0 || t == INVALID_SXP) {
            return INVALID_SXP;
          }
          x += t;
          if (!aExpr[x]) {
            return INVALID_SXP;
          }
          ++np;
        } while (aExpr[x] == '|');
        if (np < 1) { 
          return INVALID_SXP;
        }
        break;
      case ')':
      case ']':
      case '|':
        return INVALID_SXP;
      case '\\':
        ++nsc;
        if (!aExpr[++x]) {
          return INVALID_SXP;
        }
        break;
      default:
        break;
    }
  }
  if (!aStop1 && !nsc) { 
    return NON_SXP;
  }
  return ((aExpr[x] == aStop1 || aExpr[x] == aStop2) ? x : INVALID_SXP);
}


template<class T>
int
NS_WildCardValid_(const T* aExpr)
{
  int x = ::_valid_subexp(aExpr, T('\0'), T('\0'));
  return (x < 0 ? x : VALID_SXP);
}

int
NS_WildCardValid(const char* aExpr)
{
  return NS_WildCardValid_(aExpr);
}

int
NS_WildCardValid(const char16_t* aExpr)
{
  return NS_WildCardValid_(aExpr);
}




#define MATCH 0
#define NOMATCH 1
#define ABORTED -1

template<class T>
static int
_shexp_match(const T* aStr, const T* aExpr, bool aCaseInsensitive,
             unsigned int aLevel);










template<class T>
static int
_scan_and_copy(const T* aExpr, T aStop1, T aStop2, T* aDest)
{
  int sx;     
  T cc;

  for (sx = 0; (cc = aExpr[sx]) && cc != aStop1 && cc != aStop2; ++sx) {
    if (cc == '\\') {
      if (!aExpr[++sx]) {
        return ABORTED;  
      }
    } else if (cc == '[') {
      while ((cc = aExpr[++sx]) && cc != ']') {
        if (cc == '\\' && !aExpr[++sx]) {
          return ABORTED;
        }
      }
      if (!cc) {
        return ABORTED;  
      }
    }
  }
  if (aDest && sx) {
    
    memcpy(aDest, aExpr, sx * sizeof(T));
    aDest[sx] = 0;
  }
  return cc ? sx : ABORTED; 
}









template<class T>
static int
_handle_union(const T* aStr, const T* aExpr, bool aCaseInsensitive,
              unsigned int aLevel)
{
  int sx;              
  int cp;              
  int count;
  int ret   = NOMATCH;
  T* e2;

  
  cp = ::_scan_and_copy(aExpr, T(')'), T('\0'), static_cast<T*>(nullptr));
  if (cp == ABORTED || cp < 4) { 
    return ABORTED;
  }
  ++cp;                
  e2 = (T*)moz_xmalloc((1 + nsCharTraits<T>::length(aExpr)) * sizeof(T));
  if (!e2) {
    return ABORTED;
  }
  for (sx = 1; ; ++sx) {
    
    
    count = ::_scan_and_copy(aExpr + sx, T(')'), T('|'), e2);
    if (count == ABORTED || !count) {
      ret = ABORTED;
      break;
    }
    sx += count;
    
    nsCharTraits<T>::copy(e2 + count, aExpr + cp,
                          nsCharTraits<T>::length(aExpr + cp) + 1);
    ret = ::_shexp_match(aStr, e2, aCaseInsensitive, aLevel + 1);
    if (ret != NOMATCH || !aExpr[sx] || aExpr[sx] == ')') {
      break;
    }
  }
  free(e2);
  if (sx < 2) {
    ret = ABORTED;
  }
  return ret;
}


static int
_is_char_in_range(unsigned char aStart, unsigned char aEnd, unsigned char aVal)
{
  char map[256];
  memset(map, 0, sizeof(map));
  while (aStart <= aEnd) {
    map[lower(aStart++)] = 1;
  }
  return map[lower(aVal)];
}

template<class T>
static int
_shexp_match(const T* aStr, const T* aExpr, bool aCaseInsensitive,
             unsigned int aLevel)
{
  int x;   
  int y;   
  int ret, neg;

  if (aLevel > 20) {    
    return ABORTED;
  }
  for (x = 0, y = 0; aExpr[y]; ++y, ++x) {
    if (!aStr[x] && aExpr[y] != '$' && aExpr[y] != '*') {
      return NOMATCH;
    }
    switch (aExpr[y]) {
      case '$':
        if (aStr[x]) {
          return NOMATCH;
        }
        --x;                 
        break;
      case '*':
        while (aExpr[++y] == '*') {
        }
        if (!aExpr[y]) {
          return MATCH;
        }
        while (aStr[x]) {
          ret = ::_shexp_match(&aStr[x++], &aExpr[y], aCaseInsensitive,
                               aLevel + 1);
          switch (ret) {
            case NOMATCH:
              continue;
            case ABORTED:
              return ABORTED;
            default:
              return MATCH;
          }
        }
        if (aExpr[y] == '$' && aExpr[y + 1] == '\0' && !aStr[x]) {
          return MATCH;
        } else {
          return NOMATCH;
        }
      case '[': {
        T start, end = 0;
        int i;
        ++y;
        neg = (aExpr[y] == '^' && aExpr[y + 1] != ']');
        if (neg) {
          ++y;
        }
        i = y;
        start = aExpr[i++];
        if (start == '\\') {
          start = aExpr[i++];
        }
        if (::alphanumeric(start) && aExpr[i++] == '-') {
          end = aExpr[i++];
          if (end == '\\') {
            end = aExpr[i++];
          }
        }
        if (::alphanumeric(end) && aExpr[i] == ']') {
          
          T val = aStr[x];
          if (end < start) { 
            T tmp = end;
            end = start;
            start = tmp;
          }
          if (aCaseInsensitive && ::alpha(val)) {
            val = ::_is_char_in_range((unsigned char)start,
                                      (unsigned char)end,
                                      (unsigned char)val);
            if (neg == val) {
              return NOMATCH;
            }
          } else if (neg != (val < start || val > end)) {
            return NOMATCH;
          }
          y = i;
        } else {
          
          int matched = 0;
          for (; aExpr[y] != ']'; ++y) {
            if (aExpr[y] == '\\') {
              ++y;
            }
            if (aCaseInsensitive) {
              matched |= (::upper(aStr[x]) == ::upper(aExpr[y]));
            } else {
              matched |= (aStr[x] == aExpr[y]);
            }
          }
          if (neg == matched) {
            return NOMATCH;
          }
        }
      }
      break;
      case '(':
        if (!aExpr[y + 1]) {
          return ABORTED;
        }
        return ::_handle_union(&aStr[x], &aExpr[y], aCaseInsensitive,
                               aLevel + 1);
      case '?':
        break;
      case ')':
      case ']':
      case '|':
        return ABORTED;
      case '\\':
        ++y;
      
      default:
        if (aCaseInsensitive) {
          if (::upper(aStr[x]) != ::upper(aExpr[y])) {
            return NOMATCH;
          }
        } else {
          if (aStr[x] != aExpr[y]) {
            return NOMATCH;
          }
        }
        break;
    }
  }
  return (aStr[x] ? NOMATCH : MATCH);
}


template<class T>
static int
ns_WildCardMatch(const T* aStr, const T* aXp, bool aCaseInsensitive)
{
  T* expr = nullptr;
  int ret = MATCH;

  if (!nsCharTraits<T>::find(aXp, nsCharTraits<T>::length(aXp), T('~'))) {
    return ::_shexp_match(aStr, aXp, aCaseInsensitive, 0);
  }

  expr = (T*)moz_xmalloc((nsCharTraits<T>::length(aXp) + 1) * sizeof(T));
  if (!expr) {
    return NOMATCH;
  }
  memcpy(expr, aXp, (nsCharTraits<T>::length(aXp) + 1) * sizeof(T));

  int x = ::_scan_and_copy(expr, T('~'), T('\0'), static_cast<T*>(nullptr));
  if (x != ABORTED && expr[x] == '~') {
    expr[x++] = '\0';
    ret = ::_shexp_match(aStr, &expr[x], aCaseInsensitive, 0);
    switch (ret) {
      case NOMATCH:
        ret = MATCH;
        break;
      case MATCH:
        ret = NOMATCH;
        break;
      default:
        break;
    }
  }
  if (ret == MATCH) {
    ret = ::_shexp_match(aStr, expr, aCaseInsensitive, 0);
  }

  free(expr);
  return ret;
}

template<class T>
int
NS_WildCardMatch_(const T* aStr, const T* aExpr, bool aCaseInsensitive)
{
  int is_valid = NS_WildCardValid(aExpr);
  switch (is_valid) {
    case INVALID_SXP:
      return -1;
    default:
      return ::ns_WildCardMatch(aStr, aExpr, aCaseInsensitive);
  }
}

int
NS_WildCardMatch(const char* aStr, const char* aXp, bool aCaseInsensitive)
{
  return NS_WildCardMatch_(aStr, aXp, aCaseInsensitive);
}

int
NS_WildCardMatch(const char16_t* aStr, const char16_t* aXp,
                 bool aCaseInsensitive)
{
  return NS_WildCardMatch_(aStr, aXp, aCaseInsensitive);
}
