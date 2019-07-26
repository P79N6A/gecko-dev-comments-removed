




























#include <stdlib.h> 
#include <string.h>
#include <stdio.h> 
#include <ctype.h>

#include "csutil.hxx"
#include "phonet.hxx"

void init_phonet_hash(phonetable & parms) 
  {
    int i, k;

    for (i = 0; i < HASHSIZE; i++) {
      parms.hash[i] = -1;
    }

    for (i = 0; parms.rules[i][0] != '\0'; i += 2) {
      
      k = (unsigned char) parms.rules[i][0];

      if (parms.hash[k] < 0) {
	parms.hash[k] = i;
      }
    }
  }



static inline void strmove(char * dest, char * src) {
  while (*src) 
    *dest++ = *src++;
  *dest = '\0';
}

static int myisalpha(char ch) {
  if ((unsigned char) ch < 128) return isalpha(ch);
  return 1;
}




int phonet (const char * inword, char * target,
              int len,
	      phonetable & parms)
  {
    
    

    
    

    int  i,j,k=0,n,p,z;
    int  k0,n0,p0=-333,z0;
    char c, c0;
    const char * s;
    typedef unsigned char uchar;    
    char word[MAXPHONETUTF8LEN + 1];
    if (len == -1) len = strlen(inword);
    if (len > MAXPHONETUTF8LEN) return 0;
    strncpy(word, inword, MAXPHONETUTF8LEN);
    word[MAXPHONETUTF8LEN] = '\0';
  
    
    i = j = z = 0;
    while ((c = word[i]) != '\0') {
      n = parms.hash[(uchar) c];
      z0 = 0;

      if (n >= 0) {
        
        while (parms.rules[n][0] == c) {

          
          k = 1;   
          p = 5;   
          s = parms.rules[n];
          s++;     
          
          while (*s != '\0'  &&  word[i+k] == *s
                 &&  !isdigit ((unsigned char) *s)  &&  strchr ("(-<^$", *s) == NULL) {
            k++;
            s++;
          }
          if (*s == '(') {
            
            if (myisalpha(word[i+k])  
                && strchr(s+1, word[i+k]) != NULL) {
              k++;
              while (*s != ')')
                s++;
              s++;
            }
          }
          p0 = (int) *s;
          k0 = k;
          while (*s == '-'  &&  k > 1) {
            k--;
            s++;
          }
          if (*s == '<')
            s++;
          if (isdigit ((unsigned char) *s)) {
            
            p = *s - '0';
            s++;
          }
          if (*s == '^'  &&  *(s+1) == '^')
            s++;

          if (*s == '\0'
              || (*s == '^'  
                  && (i == 0  ||  ! myisalpha(word[i-1]))
                  && (*(s+1) != '$'
                      || (! myisalpha(word[i+k0]) )))
              || (*s == '$'  &&  i > 0  
                  &&  myisalpha(word[i-1])
                  && (! myisalpha(word[i+k0]) ))) 
          {
            
            
            c0 = word[i+k-1];
            n0 = parms.hash[(uchar) c0];


            if (k > 1  &&  n0 >= 0
                &&  p0 != (int) '-'  &&  word[i+k] != '\0') {
              
              while (parms.rules[n0][0] == c0) {

                
                k0 = k;
                p0 = 5;
                s = parms.rules[n0];
                s++;
                while (*s != '\0'  &&  word[i+k0] == *s
                       && ! isdigit((unsigned char) *s)  &&  strchr("(-<^$",*s) == NULL) {
                  k0++;
                  s++;
                }
                if (*s == '(') {
                  
                  if (myisalpha(word[i+k0])
                      &&  strchr (s+1, word[i+k0]) != NULL) {
                    k0++;
                    while (*s != ')'  &&  *s != '\0')
                      s++;
                    if (*s == ')')
                      s++;
                  }
                }
                while (*s == '-') {
                  
                  
                  s++;
                }
                if (*s == '<')
                  s++;
                if (isdigit ((unsigned char) *s)) {
                  p0 = *s - '0';
                  s++;
                }

                if (*s == '\0'
                    
                    || (*s == '$'  &&  ! myisalpha(word[i+k0]))) 
                {
                  if (k0 == k) {
                    
                    n0 += 2;
                    continue;
                  }

                  if (p0 < p) {
                    
                    n0 += 2;
                    continue;
                  }
                  
                  break;
                }
                n0 += 2;
              } 

              if (p0 >= p  && parms.rules[n0][0] == c0) {
                n += 2;
                continue;
              }
            } 

            
            s = parms.rules[n+1];
            p0 = (parms.rules[n][0] != '\0'
                 &&  strchr (parms.rules[n]+1,'<') != NULL) ? 1:0;
            if (p0 == 1 &&  z == 0) {
              
              if (j > 0  &&  *s != '\0'
                 && (target[j-1] == c  ||  target[j-1] == *s)) {
                j--;
              }
              z0 = 1;
              z = 1;
              k0 = 0;
              while (*s != '\0'  &&  word[i+k0] != '\0') {
                word[i+k0] = *s;
                k0++;
                s++;
              }
              if (k > k0)
                strmove (&word[0]+i+k0, &word[0]+i+k);

              
              c = word[i];
            }
            else { 
              i += k - 1;
              z = 0;
              while (*s != '\0'
                     &&  *(s+1) != '\0'  &&  j < len) {
                if (j == 0  ||  target[j-1] != *s) {
                  target[j] = *s;
                  j++;
                }
                s++;
              }
              
              c = *s;
              if (parms.rules[n][0] != '\0'
                 &&  strstr (parms.rules[n]+1, "^^") != NULL) {
                if (c != '\0') {
                  target[j] = c;
                  j++;
                }
                strmove (&word[0], &word[0]+i+1);
                i = 0;
                z0 = 1;
              }
            }
            break;
          }  
          n += 2;
        } 
      } 
      if (z0 == 0) {


        if (k && !p0 && j < len &&  c != '\0'
           && (1 || j == 0  ||  target[j-1] != c)){
           
          target[j] = c;
	  
          j++;
        }

        i++;
        z = 0;
	k=0;
      }
    }  

    target[j] = '\0';
    return (j);

  }  
