





































#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "plstr.h"
#include "nsDebug.h"

#include "ParseFTPList.h"



static inline int ParseFTPListDetermineRetval(struct list_state *state)
{
  if (state->parsed_one || state->lstyle) 
    return '?';      
  return '"';        
}

int ParseFTPList(const char *line, struct list_state *state,
                 struct list_result *result )
{
  unsigned int carry_buf_len; 
  unsigned int linelen, pos;
  const char *p;

  if (!line || !state || !result)
    return 0;

  memset( result, 0, sizeof(*result) );
  if (state->magic != ((void *)ParseFTPList))
  {
    memset( state, 0, sizeof(*state) );
    state->magic = ((void *)ParseFTPList);
  }
  state->numlines++;

  
  carry_buf_len = state->carry_buf_len;
  state->carry_buf_len = 0;

  linelen = 0;

  
  while (*line == ' ' || *line == '\t')
    line++;
    
  
  p = line;
  while (*p && *p != '\n')
    p++;
  linelen = p - line;

  if (linelen > 0 && *p == '\n' && *(p-1) == '\r')
    linelen--;

  

  if (linelen > 0)
  {
    static const char *month_names = "JanFebMarAprMayJunJulAugSepOctNovDec";
    const char *tokens[16]; 
    unsigned int toklen[(sizeof(tokens)/sizeof(tokens[0]))];
    unsigned int linelen_sans_wsp;  
    unsigned int numtoks = 0;
    unsigned int tokmarker = 0; 
    unsigned int month_num = 0;
    char tbuf[4];
    int lstyle = 0;

    if (carry_buf_len) 
    {
      tokens[0] = state->carry_buf;
      toklen[0] = carry_buf_len;
      numtoks++;
    }

    pos = 0;
    while (pos < linelen && numtoks < (sizeof(tokens)/sizeof(tokens[0])) )
    {
      while (pos < linelen && 
            (line[pos] == ' ' || line[pos] == '\t' || line[pos] == '\r'))
        pos++;
      if (pos < linelen)
      {
        tokens[numtoks] = &line[pos];
        while (pos < linelen && 
           (line[pos] != ' ' && line[pos] != '\t' && line[pos] != '\r'))
          pos++;
        if (tokens[numtoks] != &line[pos])
        {
          toklen[numtoks] = (&line[pos] - tokens[numtoks]);
          numtoks++;  
        }
      }
    }    

    if (!numtoks)
      return ParseFTPListDetermineRetval(state);

    linelen_sans_wsp = &(tokens[numtoks-1][toklen[numtoks-1]]) - tokens[0];
    if (numtoks == (sizeof(tokens)/sizeof(tokens[0])) )
    {
      pos = linelen;
      while (pos > 0 && (line[pos-1] == ' ' || line[pos-1] == '\t'))
        pos--;
      linelen_sans_wsp = pos;
    }

    

#if defined(SUPPORT_EPLF)
    
    if (!lstyle && (!state->lstyle || state->lstyle == 'E'))
    {
      if (*line == '+' && linelen > 4 && numtoks >= 2)
      {
        pos = 1;
        while (pos < (linelen-1))
        {
          p = &line[pos++];
          if (*p == '/') 
            result->fe_type = 'd'; 
          else if (*p == 'r')
            result->fe_type = 'f'; 
          else if (*p == 'm')
          {
            if (isdigit(line[pos]))
            {
              while (pos < linelen && isdigit(line[pos]))
                pos++;
              if (pos < linelen && line[pos] == ',')
              {
                PRTime t;
                PRTime seconds;
                PR_sscanf(p+1, "%llu", &seconds);
                LL_MUL(t, seconds, PR_USEC_PER_SEC);
                PR_ExplodeTime(t, PR_LocalTimeParameters, &(result->fe_time) );
              }
            }
          }
          else if (*p == 's')
          {
            if (isdigit(line[pos]))
            {
              while (pos < linelen && isdigit(line[pos]))
                pos++;
              if (pos < linelen && line[pos] == ',' &&
                 ((&line[pos]) - (p+1)) < int(sizeof(result->fe_size)-1) )
              {
                memcpy( result->fe_size, p+1, (unsigned)(&line[pos] - (p+1)) );
                result->fe_size[(&line[pos] - (p+1))] = '\0';
              }
            }
          }
          else if (isalpha(*p)) 
          {
            while (pos < linelen && *++p != ',')
              pos++;
          }
          else if (*p != '\t' || (p+1) != tokens[1])
          {
            break; 
          }
          else
          {
            state->parsed_one = 1;
            state->lstyle = lstyle = 'E';

            p = &(line[linelen_sans_wsp]);
            result->fe_fname = tokens[1];
            result->fe_fnlen = p - tokens[1];

            if (!result->fe_type) 
            {
              result->fe_type = 'f'; 
              return '?';            
            }
            return result->fe_type;
          }
          if (pos >= (linelen-1) || line[pos] != ',')
            break;
          pos++;
        } 
        memset( result, 0, sizeof(*result) );
      } 
    } 
#endif 

    

#if defined(SUPPORT_VMS)
    if (!lstyle && (!state->lstyle || state->lstyle == 'V'))
    {                          
      










      if (!state->parsed_one &&
          (numtoks == 1 || (numtoks == 2 && toklen[0] == 9 &&
                            memcmp(tokens[0], "Directory", 9)==0 )))
      {
        



        p = tokens[0];
        pos = toklen[0];
        if (numtoks == 2)
        {
          p = tokens[1];
          pos = toklen[1];
        }
        pos--;
        if (pos >= 3)
        {
          while (pos > 0 && p[pos] != '[')
          {
            pos--;
            if (p[pos] == '-' || p[pos] == '$')
            {
              if (pos == 0 || p[pos-1] == '[' || p[pos-1] == '.' ||
                  (p[pos] == '-' && (p[pos+1] == ']' || p[pos+1] == '.')))
                break;
            }
            else if (p[pos] != '.' && p[pos] != '~' && 
                     !isdigit(p[pos]) && !isalpha(p[pos]))
              break;
            else if (isalpha(p[pos]) && p[pos] != toupper(p[pos]))
              break;
          }
          if (pos > 0)
          {
            pos--;
            if (p[pos] != ':' || p[pos+1] != '[')
              pos = 0;
          }
        }
        if (pos > 0 && p[pos] == ':')
        {
          while (pos > 0)
          {
            pos--;
            if (p[pos] != '$' && p[pos] != '_' && p[pos] != '-' &&
                p[pos] != '~' && !isdigit(p[pos]) && !isalpha(p[pos]))
              break;
            else if (isalpha(p[pos]) && p[pos] != toupper(p[pos]))
              break;
          }
          if (pos == 0)
          {  
            state->lstyle = 'V';
            return '?'; 
          }
        }
         
      }
      else if ((tokens[0][toklen[0]-1]) != ';')
      {
        if (numtoks == 1 && (state->lstyle == 'V' && !carry_buf_len))
          lstyle = 'V';
        else if (numtoks < 4)
          ;
        else if (toklen[1] >= 10 && memcmp(tokens[1], "%RMS-E-PRV", 10) == 0)
          lstyle = 'V';
        else if ((&line[linelen] - tokens[1]) >= 22 &&
                  memcmp(tokens[1], "insufficient privilege", 22) == 0)
          lstyle = 'V';
        else if (numtoks != 4 && numtoks != 6)
          ;
        else if (numtoks == 6 && (
                 toklen[5] < 4 || *tokens[5] != '(' ||        
                           (tokens[5][toklen[5]-1]) != ')'  ))
          ;
        else if (  (toklen[2] == 10 || toklen[2] == 11) &&      
                        (tokens[2][toklen[2]-5]) == '-' &&
                        (tokens[2][toklen[2]-9]) == '-' &&
        (((toklen[3]==4 || toklen[3]==5 || toklen[3]==7 || toklen[3]==8) &&
                        (tokens[3][toklen[3]-3]) == ':' ) ||
         ((toklen[3]==10 || toklen[3]==11 ) &&
                        (tokens[3][toklen[3]-3]) == '.' )
        ) &&  
                                    isdigit(*tokens[1]) && 
                                    isdigit(*tokens[2]) && 
                                    isdigit(*tokens[3])    
                )
        {
          lstyle = 'V';
        }
        if (lstyle == 'V')
        {
          
















          tokmarker = 0;
          p = tokens[0];
          pos = 0;
          if (*p == '[' && toklen[0] >= 4) 
          {
            if (p[1] != ']') 
            {
              p++;
              pos++;
            }
            while (lstyle && pos < toklen[0] && *p != ']')
            {
              if (*p != '$' && *p != '.' && *p != '_' && *p != '-' &&
                  *p != '~' && !isdigit(*p) && !isalpha(*p))              
                lstyle = 0;
              pos++;
              p++;
            }
            if (lstyle && pos < (toklen[0]-1))
            {
              
              NS_ASSERTION(*p == ']', "unexpected state");
              pos++;
              p++;
              tokmarker = pos; 
            } else {
              
              lstyle = 0;
            }
          }
          while (lstyle && pos < toklen[0] && *p != ';')
          {
            if (*p != '$' && *p != '.' && *p != '_' && *p != '-' &&
                *p != '~' && !isdigit(*p) && !isalpha(*p))
              lstyle = 0;
            else if (isalpha(*p) && *p != toupper(*p))
              lstyle = 0;
            p++;
            pos++;
          }
          if (lstyle && *p == ';')
          {
            if (pos == 0 || pos == (toklen[0]-1))
              lstyle = 0;
            for (pos++;lstyle && pos < toklen[0];pos++)
            {
              if (!isdigit(tokens[0][pos]))
                lstyle = 0;
            }
          }
          pos = (p - tokens[0]); 
          pos -= tokmarker;      
          p = &(tokens[0][tokmarker]); 

          if (!lstyle || pos == 0 || pos > 80) 
          {
            lstyle = 0;
          }
          else if (numtoks == 1)
          { 
            


            if (pos >= (sizeof(state->carry_buf)-1))
              pos = (sizeof(state->carry_buf)-1); 
            memcpy( state->carry_buf, p, pos );
            state->carry_buf_len = pos;
            return '?'; 
          }
          else if (isdigit(*tokens[1])) 
          {
            for (pos = 0; lstyle && pos < (toklen[1]); pos++)
            {
              if (!isdigit((tokens[1][pos])) && (tokens[1][pos]) != '/')
                lstyle = 0;
            }
            if (lstyle && numtoks > 4) 
            {
              for (pos = 1; lstyle && pos < (toklen[5]-1); pos++)
              {
                p = &(tokens[5][pos]);
                if (*p!='R' && *p!='W' && *p!='E' && *p!='D' && *p!=',')
                  lstyle = 0;
              }
            }
          }
        } 
      }     

      if (lstyle == 'V')
      {
        state->parsed_one = 1;
        state->lstyle = lstyle;

        if (isdigit(*tokens[1]))  
        {
          
          if (*tokens[0] == '[') 
          {
            pos = toklen[0]-1;
            p = tokens[0]+1;
            while (*p != ']')
            {
              p++;
              pos--;
            }
            toklen[0] = --pos;
            tokens[0] = ++p;
          }
          pos = 0;
          while (pos < toklen[0] && (tokens[0][pos]) != ';')
            pos++;
       
          result->fe_cinfs = 1;
          result->fe_type = 'f';
          result->fe_fname = tokens[0];
          result->fe_fnlen = pos;

          if (pos > 4)
          {
            p = &(tokens[0][pos-4]);
            if (p[0] == '.' && p[1] == 'D' && p[2] == 'I' && p[3] == 'R')
            {
              result->fe_fnlen -= 4;
              result->fe_type = 'd';
            }
          }

          if (result->fe_type != 'd')
          {
            




            pos = 0;
            while (pos < toklen[1] && (tokens[1][pos]) != '/')
              pos++;
            






#if 0
            if (pos < toklen[1] && ( (pos<<1) > (toklen[1]-1) ||
                 (strtoul(tokens[1], (char **)0, 10) > 
                  strtoul(tokens[1]+pos+1, (char **)0, 10))        ))
            {                                   
              if (pos > (sizeof(result->fe_size)-1))
                pos = sizeof(result->fe_size)-1;
              memcpy( result->fe_size, tokens[1], pos );
              result->fe_size[pos] = '\0';
            }
            else 
#endif
            {
              
















              PRUint64 fsz, factor;
              LL_UI2L(fsz, strtoul(tokens[1], (char **)0, 10));
              LL_UI2L(factor, 512);
              LL_MUL(fsz, fsz, factor);
              PR_snprintf(result->fe_size, sizeof(result->fe_size), 
                          "%lld", fsz);
            } 

          } 

          p = tokens[2] + 2;
          if (*p == '-')
            p++;
          tbuf[0] = p[0];
          tbuf[1] = tolower(p[1]);
          tbuf[2] = tolower(p[2]);
          month_num = 0;
          for (pos = 0; pos < (12*3); pos+=3)
          {
            if (tbuf[0] == month_names[pos+0] && 
                tbuf[1] == month_names[pos+1] && 
                tbuf[2] == month_names[pos+2])
              break;
            month_num++;
          }
          if (month_num >= 12)
            month_num = 0;
          result->fe_time.tm_month = month_num;
          result->fe_time.tm_mday = atoi(tokens[2]);
          result->fe_time.tm_year = atoi(p+4); 

          p = tokens[3] + 2;
          if (*p == ':')
            p++;
          if (p[2] == ':')
            result->fe_time.tm_sec = atoi(p+3);
          result->fe_time.tm_hour = atoi(tokens[3]);
          result->fe_time.tm_min  = atoi(p);
      
          return result->fe_type;

        } 

        return '?'; 

      } 
    } 
#endif

    

#if defined(SUPPORT_CMS)
    
    if (!lstyle && (!state->lstyle || state->lstyle == 'C'))  
    {
      





















      if (numtoks >= 7 && (toklen[0]+toklen[1]) <= 16)
      {
        for (pos = 1; !lstyle && (pos+5) < numtoks; pos++)
        {
          p = tokens[pos];
          if ((toklen[pos] == 1 && (*p == 'F' || *p == 'V')) ||
              (toklen[pos] == 3 && *p == 'D' && p[1] == 'I' && p[2] == 'R'))
          {
            if (toklen[pos+5] == 8 && (tokens[pos+5][2]) == ':' &&
                                      (tokens[pos+5][5]) == ':'   )
            {
              p = tokens[pos+4];
              if ((toklen[pos+4] == 10 && p[4] == '-' && p[7] == '-') ||
                  (toklen[pos+4] >= 7 && toklen[pos+4] <= 9 && 
                            p[((p[1]!='/')?(2):(1))] == '/' && 
                            p[((p[1]!='/')?(5):(4))] == '/'))
               
              {
                if ( (*tokens[pos+1] == '-' &&
                      *tokens[pos+2] == '-' &&
                      *tokens[pos+3] == '-')  ||
                      (isdigit(*tokens[pos+1]) &&
                       isdigit(*tokens[pos+2]) &&
                       isdigit(*tokens[pos+3])) )
                {
                  lstyle = 'C';
                  tokmarker = pos;
                }
              }
            }
          }
        } 
      } 

      
      if (lstyle && !state->lstyle) 
      {
        for (pos = 0, p = tokens[0]; lstyle && pos < toklen[0]; pos++, p++)
        {  
          if (isalpha(*p) && toupper(*p) != *p)
            lstyle = 0;
        } 
        for (pos = tokmarker+1; pos <= tokmarker+3; pos++)
        {
          if (!(toklen[pos] == 1 && *tokens[pos] == '-'))
          {
            for (p = tokens[pos]; lstyle && p<(tokens[pos]+toklen[pos]); p++)
            {
              if (!isdigit(*p))
                lstyle = 0;
            }
          }
        }
        for (pos = 0, p = tokens[tokmarker+4]; 
             lstyle && pos < toklen[tokmarker+4]; pos++, p++)
        {
          if (*p == '/')
          { 
            

             
            if ((tokens[tokmarker+4][1]) == '/')
            {
              if (pos != 1 && pos != 4)
                lstyle = 0;
            }
            else if (pos != 2 && pos != 5)
              lstyle = 0;
          }
          else if (*p != '-' && !isdigit(*p))
            lstyle = 0;
          else if (*p == '-' && pos != 4 && pos != 7)
            lstyle = 0;
        }
        for (pos = 0, p = tokens[tokmarker+5]; 
             lstyle && pos < toklen[tokmarker+5]; pos++, p++)
        {
          if (*p != ':' && !isdigit(*p))
            lstyle = 0;
          else if (*p == ':' && pos != (toklen[tokmarker+5]-3)
                             && pos != (toklen[tokmarker+5]-6))
            lstyle = 0;
        }
      } 

      if (lstyle == 'C')
      {
        state->parsed_one = 1;
        state->lstyle = lstyle;

        p = tokens[tokmarker+4];
        if (toklen[tokmarker+4] == 10) 
        {
          result->fe_time.tm_year = atoi(p+0) - 1900;
          result->fe_time.tm_month  = atoi(p+5) - 1;
          result->fe_time.tm_mday = atoi(p+8);
        }
        else 
        {
          pos = toklen[tokmarker+4];
          result->fe_time.tm_month  = atoi(p) - 1;
          result->fe_time.tm_mday = atoi((p+pos)-5);
          result->fe_time.tm_year = atoi((p+pos)-2);
          if (result->fe_time.tm_year < 70)
            result->fe_time.tm_year += 100;
        }

        p = tokens[tokmarker+5];
        pos = toklen[tokmarker+5];
        result->fe_time.tm_hour  = atoi(p);
        result->fe_time.tm_min = atoi((p+pos)-5);
        result->fe_time.tm_sec = atoi((p+pos)-2);

        result->fe_cinfs = 1;
        result->fe_fname = tokens[0];
        result->fe_fnlen = toklen[0];
        result->fe_type  = 'f';

        p = tokens[tokmarker];
        if (toklen[tokmarker] == 3 && *p=='D' && p[1]=='I' && p[2]=='R')
          result->fe_type  = 'd';

        if (( toklen[tokmarker+4] == 10 && tokmarker > 1) ||
            ( toklen[tokmarker+4] != 10 && tokmarker > 2))
        {                            
          char *dot;
          p = &(tokens[0][toklen[0]]);
          memcpy( &dot, &p, sizeof(dot) ); 
          *dot++ = '.';
          p = tokens[1];
          for (pos = 0; pos < toklen[1]; pos++)
            *dot++ = *p++;
          result->fe_fnlen += 1 + toklen[1];
        }

        




        
        


        return result->fe_type;

      } 
    } 
#endif

    

#if defined(SUPPORT_DOS) 
    if (!lstyle && (!state->lstyle || state->lstyle == 'W'))
    {
      







      if ((numtoks >= 4) && toklen[0] == 8 && toklen[1] == 7 && 
          (*tokens[2] == '<' || isdigit(*tokens[2])) )
      {
        p = tokens[0];
        if ( isdigit(p[0]) && isdigit(p[1]) && p[2]=='-' && 
             isdigit(p[3]) && isdigit(p[4]) && p[5]=='-' &&
             isdigit(p[6]) && isdigit(p[7]) )
        {
          p = tokens[1];
          if ( isdigit(p[0]) && isdigit(p[1]) && p[2]==':' && 
               isdigit(p[3]) && isdigit(p[4]) && 
               (p[5]=='A' || p[5]=='P') && p[6]=='M')
          {
            lstyle = 'W';
            if (!state->lstyle)
            {            
              p = tokens[2];
              
              if (*p != '<' || p[toklen[2]-1] != '>')
              {
                for (pos = 1; (lstyle && pos < toklen[2]); pos++)
                {
                  if (!isdigit(*++p))
                    lstyle = 0;
                }
              }
            }
          }
        }
      }

      if (lstyle == 'W')
      {
        state->parsed_one = 1;
        state->lstyle = lstyle;

        p = &(line[linelen]); 
        result->fe_cinfs = 1;
        result->fe_fname = tokens[3];
        result->fe_fnlen = p - tokens[3];
        result->fe_type = 'd';

        if (*tokens[2] != '<') 
        {
          
          
          if (tokens[2] + toklen[2] - line == 38) {
            result->fe_fname = &(line[39]);
            result->fe_fnlen = p - result->fe_fname;
          }
          result->fe_type = 'f';
          pos = toklen[2];
          while (pos > (sizeof(result->fe_size)-1))
            pos = (sizeof(result->fe_size)-1);
          memcpy( result->fe_size, tokens[2], pos );
          result->fe_size[pos] = '\0';
        }
        else {
          
          
          
          if (tokens[2] - line == 24 && (toklen[2] == 5 || toklen[2] == 10) &&
              tokens[3] - line >= 39) {
            result->fe_fname = &(line[39]);
            result->fe_fnlen = p - result->fe_fname;
          }

          if ((tokens[2][1]) != 'D') 
          {
            result->fe_type = '?'; 
            if (result->fe_fnlen > 4)
            {
              p = result->fe_fname;
              for (pos = result->fe_fnlen - 4; pos > 0; pos--)
              {
                if (p[0] == ' ' && p[3] == ' ' && p[2] == '>' &&
                    (p[1] == '=' || p[1] == '-'))
                {
                  result->fe_type = 'l';
                  result->fe_fnlen = p - result->fe_fname;
                  result->fe_lname = p + 4;
                  result->fe_lnlen = &(line[linelen]) 
                                     - result->fe_lname;
                  break;
                }
                p++;
              }
            }
          }
        }

        result->fe_time.tm_month = atoi(tokens[0]+0);
        if (result->fe_time.tm_month != 0)
        {
          result->fe_time.tm_month--;
          result->fe_time.tm_mday = atoi(tokens[0]+3);
          result->fe_time.tm_year = atoi(tokens[0]+6);
          


          if (result->fe_time.tm_year < 80)
            result->fe_time.tm_year += 2000;
          else if (result->fe_time.tm_year < 100)
            result->fe_time.tm_year += 1900;
        }

        result->fe_time.tm_hour = atoi(tokens[1]+0);
        result->fe_time.tm_min = atoi(tokens[1]+3);
        if ((tokens[1][5]) == 'P' && result->fe_time.tm_hour < 12)
          result->fe_time.tm_hour += 12;

        






        return result->fe_type;  
      } 
    } 
#endif

    

#if defined(SUPPORT_OS2)
    if (!lstyle && (!state->lstyle || state->lstyle == 'O')) 
    {
      




















      p = &(line[toklen[0]]);
      
      if (numtoks >= 4 && toklen[0] <= 18 && isdigit(*tokens[0]) &&
         (linelen - toklen[0]) >= (53-18)                        &&
         p[18-18] == ' ' && p[34-18] == ' '                      &&
         p[37-18] == '-' && p[40-18] == '-' && p[43-18] == ' '   &&
         p[45-18] == ' ' && p[48-18] == ':' && p[51-18] == ' '   &&
         isdigit(p[35-18]) && isdigit(p[36-18])                  &&
         isdigit(p[38-18]) && isdigit(p[39-18])                  &&
         isdigit(p[41-18]) && isdigit(p[42-18])                  &&
         isdigit(p[46-18]) && isdigit(p[47-18])                  &&
         isdigit(p[49-18]) && isdigit(p[50-18])
      )
      {
        lstyle = 'O'; 
        if (!state->lstyle)
        {            
          for (pos = 1; lstyle && pos < toklen[0]; pos++)
          {
            if (!isdigit(tokens[0][pos]))
              lstyle = 0;
          }
        }
      }

      if (lstyle == 'O')
      {
        state->parsed_one = 1;
        state->lstyle = lstyle;

        p = &(line[toklen[0]]);

        result->fe_cinfs = 1;
        result->fe_fname = &p[53-18];
        result->fe_fnlen = (&(line[linelen_sans_wsp]))
                           - (result->fe_fname);
        result->fe_type = 'f';

        
        for (pos = (18-18); pos < ((35-18)-4); pos++)
        {
          if (p[pos+0] == ' ' && p[pos+1] == 'D' && 
              p[pos+2] == 'I' && p[pos+3] == 'R')
          {
            result->fe_type = 'd';
            break;
          }
        }
    
        if (result->fe_type != 'd')
        {
          pos = toklen[0];
          if (pos > (sizeof(result->fe_size)-1))
            pos = (sizeof(result->fe_size)-1);
          memcpy( result->fe_size, tokens[0], pos );
          result->fe_size[pos] = '\0';
        }  
    
        result->fe_time.tm_month = atoi(&p[35-18]) - 1;
        result->fe_time.tm_mday = atoi(&p[38-18]);
        result->fe_time.tm_year = atoi(&p[41-18]);
        if (result->fe_time.tm_year < 80)
          result->fe_time.tm_year += 100;
        result->fe_time.tm_hour = atoi(&p[46-18]);
        result->fe_time.tm_min = atoi(&p[49-18]);
   
        






        return result->fe_type;
      } 

    } 
#endif

    
    
#if defined(SUPPORT_LSL)
    if (!lstyle && (!state->lstyle || state->lstyle == 'U')) 
    {
      























    
      PRBool is_old_Hellsoft = PR_FALSE;
    
      if (numtoks >= 6)
      {
        


        if (toklen[0] == 1 || (tokens[0][1]) == '[')
        {
          if (*tokens[0] == 'd' || *tokens[0] == '-')
          {
            pos = toklen[0]-1;
            p = tokens[0] + 1;
            if (pos == 0)
            {
              p = tokens[1];
              pos = toklen[1];
            }
            if ((pos == 9 || pos == 10)        && 
                (*p == '[' && p[pos-1] == ']') &&
                (p[1] == 'R' || p[1] == '-')   &&
                (p[2] == 'W' || p[2] == '-')   &&
                (p[3] == 'C' || p[3] == '-')   &&
                (p[4] == 'E' || p[4] == '-'))
            {
              
              lstyle = 'U'; 
              if (toklen[0] == 10)
                is_old_Hellsoft = PR_TRUE;
            }
          }
        }
        else if ((toklen[0] == 10 || toklen[0] == 11) 
                   && strchr("-bcdlpsw?DFam", *tokens[0]))
        {
          p = &(tokens[0][1]);
          if ((p[0] == 'r' || p[0] == '-') &&
              (p[1] == 'w' || p[1] == '-') &&
              (p[3] == 'r' || p[3] == '-') &&
              (p[4] == 'w' || p[4] == '-') &&
              (p[6] == 'r' || p[6] == '-') &&
              (p[7] == 'w' || p[7] == '-'))
            
          {
            lstyle = 'U'; 
          }
        }
      }
      if (lstyle == 'U') 
      {
        lstyle = 0;
        for (pos = (numtoks-5); !lstyle && pos > 1; pos--)
        {
          



          if (isdigit(*tokens[pos]) 
              
           && toklen[pos+1] == 3 && isalpha(*tokens[pos+1]) &&
              isalpha(tokens[pos+1][1]) && isalpha(tokens[pos+1][2])
              
           && isdigit(*tokens[pos+2]) &&
                (toklen[pos+2] == 1 || 
                  (toklen[pos+2] == 2 && isdigit(tokens[pos+2][1])))
           && toklen[pos+3] >= 4 && isdigit(*tokens[pos+3]) 
              
           && (toklen[pos+3] <= 5 || (
               (toklen[pos+3] == 7 || toklen[pos+3] == 8) &&
               (tokens[pos+3][toklen[pos+3]-3]) == ':'))
           && isdigit(tokens[pos+3][toklen[pos+3]-2])
           && isdigit(tokens[pos+3][toklen[pos+3]-1])
           && (
              
                 ((toklen[pos+3] == 4 || toklen[pos+3] == 5) &&
                  isdigit(tokens[pos+3][1]) &&
                  isdigit(tokens[pos+3][2])  )
              
              || ((toklen[pos+3] == 4 || toklen[pos+3] == 7) && 
                  (tokens[pos+3][1]) == ':' &&
                  isdigit(tokens[pos+3][2]) && isdigit(tokens[pos+3][3]))
              
              || ((toklen[pos+3] == 5 || toklen[pos+3] == 8) && 
                  isdigit(tokens[pos+3][1]) && (tokens[pos+3][2]) == ':' &&
                  isdigit(tokens[pos+3][3]) && isdigit(tokens[pos+3][4])) 
              )
           )
          {
            lstyle = 'U'; 
            tokmarker = pos;

            
            p = tokens[tokmarker];
            unsigned int i;
            for (i = 0; i < toklen[tokmarker]; i++)
            {
              if (!isdigit(*p++))
              {
                lstyle = 0;
                break;
              }
            }
            if (lstyle)
            {
              month_num = 0;
              p = tokens[tokmarker+1];
              for (i = 0; i < (12*3); i+=3)
              {
                if (p[0] == month_names[i+0] && 
                    p[1] == month_names[i+1] && 
                    p[2] == month_names[i+2])
                  break;
                month_num++;
              }
              if (month_num >= 12)
                lstyle = 0;
            }
          } 
        } 
      } 

      if (lstyle == 'U')
      {
        state->parsed_one = 1;
        state->lstyle = lstyle;
    
        result->fe_cinfs = 0;
        result->fe_type = '?';
        if (*tokens[0] == 'd' || *tokens[0] == 'l')
          result->fe_type = *tokens[0];
        else if (*tokens[0] == 'D')
          result->fe_type = 'd';
        else if (*tokens[0] == '-' || *tokens[0] == 'F')
          result->fe_type = 'f'; 

        if (result->fe_type != 'd')
        {
          pos = toklen[tokmarker];
          if (pos > (sizeof(result->fe_size)-1))
            pos = (sizeof(result->fe_size)-1);
          memcpy( result->fe_size, tokens[tokmarker], pos );
          result->fe_size[pos] = '\0';
        }

        result->fe_time.tm_month  = month_num;
        result->fe_time.tm_mday = atoi(tokens[tokmarker+2]);
        if (result->fe_time.tm_mday == 0)
          result->fe_time.tm_mday++;

        p = tokens[tokmarker+3];
        pos = (unsigned int)atoi(p);
        if (p[1] == ':') 
          p--;
        if (p[2] != ':') 
        {
          result->fe_time.tm_year = pos;
        }
        else
        {
          result->fe_time.tm_hour = pos;
          result->fe_time.tm_min  = atoi(p+3);
          if (p[5] == ':')
            result->fe_time.tm_sec = atoi(p+6);
       
          if (!state->now_time)
          {
            state->now_time = PR_Now();
            PR_ExplodeTime((state->now_time), PR_LocalTimeParameters, &(state->now_tm) );
          }

          result->fe_time.tm_year = state->now_tm.tm_year;
          if ( (( state->now_tm.tm_month << 5) + state->now_tm.tm_mday) <
               ((result->fe_time.tm_month << 5) + result->fe_time.tm_mday) )
            result->fe_time.tm_year--;
       
        } 
        
        
        
        if (!is_old_Hellsoft)
          result->fe_fname = tokens[tokmarker+3] + toklen[tokmarker+3] + 1;
        else
          result->fe_fname = tokens[tokmarker+4];

        result->fe_fnlen = (&(line[linelen]))
                           - (result->fe_fname);

        if (result->fe_type == 'l' && result->fe_fnlen > 4)
        {
          

          PRUint32 fe_size = atoi(result->fe_size);

          if (result->fe_fnlen > (fe_size + 4) &&
              PL_strncmp(result->fe_fname + result->fe_fnlen - fe_size - 4 , " -> ", 4) == 0)
          {
            result->fe_lname = result->fe_fname + (result->fe_fnlen - fe_size);
            result->fe_lnlen = (&(line[linelen])) - (result->fe_lname);
            result->fe_fnlen -= fe_size + 4;
          }
          else
          {
            




            p = result->fe_fname + (result->fe_fnlen - 5);
            for (pos = (result->fe_fnlen - 5); pos > 0; pos--)
            {
              if (PL_strncmp(p, " -> ", 4) == 0)
              {
                result->fe_lname = p + 4;
                result->fe_lnlen = (&(line[linelen]))
                                 - (result->fe_lname);
                result->fe_fnlen = pos;
                break;
              }
              p--;
            }
          }
        }

#if defined(SUPPORT_LSLF) 
        if (result->fe_fnlen > 1)
        {
          p = result->fe_fname[result->fe_fnlen-1];
          pos = result->fe_type;
          if (pos == 'd') { 
             if (*p == '/') result->fe_fnlen--; 
          } else if (pos == 'l') { 
             if (*p == '@') result->fe_fnlen--; 
          } else if (pos == 'f') { 
             if (*p == '*') result->fe_fnlen--; 
          } else if (*p == '=' || *p == '%' || *p == '|') {
            result->fe_fnlen--; 
          }
        }
#endif
     
        






        return result->fe_type;  

      } 

    } 
#endif

    

#if defined(SUPPORT_W16) 
    if (!lstyle && (!state->lstyle || state->lstyle == 'w'))
    {       
            
      





















      if (numtoks >= 4 && toklen[0] < 13 && 
          ((toklen[1] == 5 && *tokens[1] == '<') || isdigit(*tokens[1])) )
      {
        if (numtoks == 4
         && (toklen[2] == 8 || toklen[2] == 9)
         && (((tokens[2][2]) == '/' && (tokens[2][5]) == '/') ||
             ((tokens[2][2]) == '-' && (tokens[2][5]) == '-'))
         && (toklen[3] == 4 || toklen[3] == 5)
         && (tokens[3][toklen[3]-3]) == ':'
         && isdigit(tokens[2][0]) && isdigit(tokens[2][1])
         && isdigit(tokens[2][3]) && isdigit(tokens[2][4])
         && isdigit(tokens[2][6]) && isdigit(tokens[2][7])
         && (toklen[2] < 9 || isdigit(tokens[2][8]))
         && isdigit(tokens[3][toklen[3]-1]) && isdigit(tokens[3][toklen[3]-2])
         && isdigit(tokens[3][toklen[3]-4]) && isdigit(*tokens[3]) 
         )
        {
          lstyle = 'w';
        }
        else if ((numtoks == 6 || numtoks == 7)
         && toklen[2] == 3 && toklen[3] == 2
         && toklen[4] == 4 && toklen[5] == 5
         && (tokens[5][2]) == ':'
         && isalpha(tokens[2][0]) && isalpha(tokens[2][1])
         &&                          isalpha(tokens[2][2])
         && isdigit(tokens[3][0]) && isdigit(tokens[3][1])
         && isdigit(tokens[4][0]) && isdigit(tokens[4][1])
         && isdigit(tokens[4][2]) && isdigit(tokens[4][3])
         && isdigit(tokens[5][0]) && isdigit(tokens[5][1])
         && isdigit(tokens[5][3]) && isdigit(tokens[5][4])
         
        )
        {
          lstyle = 'w';
        }
        if (lstyle && state->lstyle != lstyle) 
        {
          p = tokens[1];   
          if (toklen[1] != 5 || p[0] != '<' || p[1] != 'D' || 
                 p[2] != 'I' || p[3] != 'R' || p[4] != '>')
          {
            for (pos = 0; lstyle && pos < toklen[1]; pos++)
            {
              if (!isdigit(*p++))
                lstyle = 0;
            }
          } 
        } 
      } 

      if (lstyle == 'w')
      {
        state->parsed_one = 1;
        state->lstyle = lstyle;

        result->fe_cinfs = 1;
        result->fe_fname = tokens[0];
        result->fe_fnlen = toklen[0];
        result->fe_type = 'd';

        p = tokens[1];
        if (isdigit(*p))
        {
          result->fe_type = 'f';
          pos = toklen[1];
          if (pos > (sizeof(result->fe_size)-1))
            pos = sizeof(result->fe_size)-1;
          memcpy( result->fe_size, p, pos );
          result->fe_size[pos] = '\0';
        }

        p = tokens[2];
        if (toklen[2] == 3) 
        {
          tbuf[0] = toupper(p[0]);
          tbuf[1] = tolower(p[1]);
          tbuf[2] = tolower(p[2]);
          for (pos = 0; pos < (12*3); pos+=3)
          {
            if (tbuf[0] == month_names[pos+0] &&
                tbuf[1] == month_names[pos+1] && 
                tbuf[2] == month_names[pos+2])
            {
              result->fe_time.tm_month = pos/3;
              result->fe_time.tm_mday = atoi(tokens[3]);
              result->fe_time.tm_year = atoi(tokens[4]) - 1900;
              break;
            }
          }          
          pos = 5; 
        }
        else
        {
          result->fe_time.tm_month = atoi(p+0)-1;
          result->fe_time.tm_mday = atoi(p+3);
          result->fe_time.tm_year = atoi(p+6);
          if (result->fe_time.tm_year < 80) 
            result->fe_time.tm_year += 100;

          pos = 3; 
        }

        result->fe_time.tm_hour = atoi(tokens[pos]);
        result->fe_time.tm_min = atoi(&(tokens[pos][toklen[pos]-2]));

        






        return result->fe_type;
      } 

    } 
#endif

    

#if defined(SUPPORT_DLS) 
    if (!lstyle && 
       (state->lstyle == 'D' || (!state->lstyle && state->numlines == 1)))
       
    {
      

































      if (!state->lstyle && line[linelen-1] == ':' && 
          linelen >= 2 && toklen[numtoks-1] != 1)
      { 
        



        pos = 0;
        p = line;
        while (pos < (linelen-1))
        {
          
          if (*p == '<' || *p == '|' || *p == '>' ||
              *p == '?' || *p == '*' || *p == '\\')
            break;
          if (*p == '/' && pos < (linelen-2) && p[1] == '/')
            break;
          pos++;
          p++;
        }
        if (pos == (linelen-1))
        {
          state->lstyle = 'D';
          return '?';
        }
      }

      if (!lstyle && numtoks >= 2)
      {
        pos = 22; 
        if (state->lstyle && carry_buf_len) 
          pos = toklen[1]-1; 

        if (linelen > pos)
        {
          p = &line[pos];
          if ((*p == '-' || *p == '=' || isdigit(*p)) &&
              ((linelen == (pos+1)) || 
               (linelen >= (pos+3) && p[1] == ' ' && p[2] == ' ')) )
          {
            tokmarker = 1;
            if (!carry_buf_len)
            {
              pos = 1;
              while (pos < numtoks && (tokens[pos]+toklen[pos]) < (&line[23]))
                pos++;
              tokmarker = 0;
              if ((tokens[pos]+toklen[pos]) == (&line[23]))
                tokmarker = pos;
            }
            if (tokmarker)  
            {
              lstyle = 'D';
              if (*tokens[tokmarker] == '-' || *tokens[tokmarker] == '=')
              {
                if (toklen[tokmarker] != 1 ||
                   (tokens[tokmarker-1][toklen[tokmarker-1]-1]) != '/')
                  lstyle = 0;
              }              
              else
              {
                for (pos = 0; lstyle && pos < toklen[tokmarker]; pos++) 
                {
                  if (!isdigit(tokens[tokmarker][pos]))
                    lstyle = 0; 
                }
              }
              if (lstyle && !state->lstyle) 
              {
                
                for (p = tokens[0]; lstyle &&
                     p < &(tokens[tokmarker-1][toklen[tokmarker-1]]); p++)
                {
                  if (*p == '<' || *p == '|' || *p == '>' || 
                      *p == '?' || *p == '*' || *p == '/' || *p == '\\')
                    lstyle = 0;
                }
              }

            } 
          } 
        } 
      } 

      if (!lstyle && state->lstyle == 'D' && !carry_buf_len)
      {
        





        pos = linelen;
        if (pos > (sizeof(state->carry_buf)-1))
          pos = sizeof(state->carry_buf)-1;
        memcpy( state->carry_buf, line, pos );
        state->carry_buf_len = pos;
        return '?';
      }

      if (lstyle == 'D')
      {
        state->parsed_one = 1;
        state->lstyle = lstyle;

        p = &(tokens[tokmarker-1][toklen[tokmarker-1]]);
        result->fe_fname = tokens[0];
        result->fe_fnlen = p - tokens[0];
        result->fe_type  = 'f';

        if (result->fe_fname[result->fe_fnlen-1] == '/')
        {
          if (result->fe_lnlen == 1)
            result->fe_type = '?';
          else
          {
            result->fe_fnlen--;
            result->fe_type  = 'd';
          }
        }
        else if (isdigit(*tokens[tokmarker]))
        {
          pos = toklen[tokmarker];
          if (pos > (sizeof(result->fe_size)-1))
            pos = sizeof(result->fe_size)-1;
          memcpy( result->fe_size, tokens[tokmarker], pos );
          result->fe_size[pos] = '\0';
        }

        if ((tokmarker+3) < numtoks && 
              (&(tokens[numtoks-1][toklen[numtoks-1]]) - 
               tokens[tokmarker+1]) >= (1+1+3+1+4) )
        {
          pos = (tokmarker+3);
          p = tokens[pos];
          pos = toklen[pos];

          if ((pos == 4 || pos == 5)
          &&  isdigit(*p) && isdigit(p[pos-1]) && isdigit(p[pos-2])
          &&  ((pos == 5 && p[2] == ':') ||  
               (pos == 4 && (isdigit(p[1]) || p[1] == ':')))
             )
          {
            month_num = tokmarker+1; 
            pos = tokmarker+2;       
            if (isdigit(*tokens[month_num])) 
            {
              month_num++;
              pos--;
            }
            p = tokens[month_num];
            if (isdigit(*tokens[pos]) 
            && (toklen[pos] == 1 || 
                  (toklen[pos] == 2 && isdigit(tokens[pos][1])))
            && toklen[month_num] == 3
            && isalpha(*p) && isalpha(p[1]) && isalpha(p[2])  )
            {
              pos = atoi(tokens[pos]);
              if (pos > 0 && pos <= 31)
              {
                result->fe_time.tm_mday = pos;
                month_num = 1;
                for (pos = 0; pos < (12*3); pos+=3)
                {
                  if (p[0] == month_names[pos+0] &&
                      p[1] == month_names[pos+1] &&
                      p[2] == month_names[pos+2])
                    break;
                  month_num++;
                }
                if (month_num > 12)
                  result->fe_time.tm_mday = 0;
                else
                  result->fe_time.tm_month = month_num - 1;
              }
            }
            if (result->fe_time.tm_mday)
            {
              tokmarker += 3; 
              p = tokens[tokmarker];

              pos = atoi(p);
              if (pos > 24)
                result->fe_time.tm_year = pos-1900;
              else
              {
                if (p[1] == ':')
                  p--;
                result->fe_time.tm_hour = pos;
                result->fe_time.tm_min = atoi(p+3);
                if (!state->now_time)
                {
                  state->now_time = PR_Now();
                  PR_ExplodeTime((state->now_time), PR_LocalTimeParameters, &(state->now_tm) );
                }
                result->fe_time.tm_year = state->now_tm.tm_year;
                if ( (( state->now_tm.tm_month  << 4) + state->now_tm.tm_mday) <
                     ((result->fe_time.tm_month << 4) + result->fe_time.tm_mday) )
                  result->fe_time.tm_year--;
              } 
            } 
          } 
        } 

        if (numtoks > (tokmarker+2))
        {
          pos = tokmarker+1;
          p = tokens[pos];
          if (toklen[pos] == 2 && *p == '-' && p[1] == '>')
          {
            p = &(tokens[numtoks-1][toklen[numtoks-1]]);
            result->fe_type  = 'l';
            result->fe_lname = tokens[pos+1];
            result->fe_lnlen = p - result->fe_lname;
            if (result->fe_lnlen > 1 &&
                result->fe_lname[result->fe_lnlen-1] == '/')
              result->fe_lnlen--;
          }
        } 

        






        return result->fe_type;

      } 
    } 
#endif

    

  } 

  return ParseFTPListDetermineRetval(state);
}




#if 0

#include <stdio.h>

static int do_it(FILE *outfile, 
                 char *line, size_t linelen, struct list_state *state,
                 char **cmnt_buf, unsigned int *cmnt_buf_sz,
                 char **list_buf, unsigned int *list_buf_sz )
{
  struct list_result result;
  char *p;
  int rc;

  rc = ParseFTPList( line, state, &result );

  if (!outfile)
  {
    outfile = stdout;
    if (rc == '?')
      fprintf(outfile, "junk: %.*s\n", (int)linelen, line );
    else if (rc == '"')
      fprintf(outfile, "cmnt: %.*s\n", (int)linelen, line );
    else
      fprintf(outfile, 
              "list: %02u-%02u-%02u  %02u:%02u%cM %20s %.*s%s%.*s\n",
              (result.fe_time.tm_mday ? (result.fe_time.tm_month + 1) : 0),
              result.fe_time.tm_mday,
              (result.fe_time.tm_mday ? (result.fe_time.tm_year % 100) : 0),
              result.fe_time.tm_hour - 
                ((result.fe_time.tm_hour > 12)?(12):(0)),
              result.fe_time.tm_min,
              ((result.fe_time.tm_hour >= 12) ? 'P' : 'A'),
              (rc == 'd' ? "<DIR>         " : 
              (rc == 'l' ? "<JUNCTION>    " : result.fe_size)),
              (int)result.fe_fnlen, result.fe_fname,
              ((rc == 'l' && result.fe_lnlen) ? " -> " : ""),
              (int)((rc == 'l' && result.fe_lnlen) ? result.fe_lnlen : 0),
              ((rc == 'l' && result.fe_lnlen) ? result.fe_lname : "") );
  }
  else if (rc != '?') 
  { 
    char **bufp = list_buf;
    unsigned int *bufz = list_buf_sz;
    
    if (rc == '"') 
    {
      memset( &result, 0, sizeof(result));
      result.fe_fname = line;
      result.fe_fnlen = linelen;
      result.fe_type = 'f';
      if (line[linelen-1] == '/')
      {
        result.fe_type = 'd';
        result.fe_fnlen--; 
      }
      bufp = cmnt_buf;
      bufz = cmnt_buf_sz;
      rc = result.fe_type;
    }  

    linelen = 80 + result.fe_fnlen + result.fe_lnlen;
    p = (char *)realloc( *bufp, *bufz + linelen );
    if (!p)
      return -1;
    sprintf( &p[*bufz], 
             "%02u-%02u-%04u  %02u:%02u:%02u %20s %.*s%s%.*s\n",
              (result.fe_time.tm_mday ? (result.fe_time.tm_month + 1) : 0),
              result.fe_time.tm_mday,
              (result.fe_time.tm_mday ? (result.fe_time.tm_year + 1900) : 0),
              result.fe_time.tm_hour,
              result.fe_time.tm_min,
              result.fe_time.tm_sec,
              (rc == 'd' ? "<DIR>         " : 
              (rc == 'l' ? "<JUNCTION>    " : result.fe_size)),
              (int)result.fe_fnlen, result.fe_fname,
              ((rc == 'l' && result.fe_lnlen) ? " -> " : ""),
              (int)((rc == 'l' && result.fe_lnlen) ? result.fe_lnlen : 0),
              ((rc == 'l' && result.fe_lnlen) ? result.fe_lname : "") );
    linelen = strlen(&p[*bufz]);
    *bufp = p;
    *bufz = *bufz + linelen;
  }
  return 0;
}

int main(int argc, char *argv[])
{
  FILE *infile = (FILE *)0;
  FILE *outfile = (FILE *)0;
  int need_close_in = 0;
  int need_close_out = 0;

  if (argc > 1)
  {
    infile = stdin;
    if (strcmp(argv[1], "-") == 0)
      need_close_in = 0;
    else if ((infile = fopen(argv[1], "r")) != ((FILE *)0))
      need_close_in = 1;
    else
      fprintf(stderr, "Unable to open input file '%s'\n", argv[1]);
  }
  if (infile && argc > 2)
  {
    outfile = stdout;
    if (strcmp(argv[2], "-") == 0)
      need_close_out = 0;
    else if ((outfile = fopen(argv[2], "w")) != ((FILE *)0))
      need_close_out = 1;
    else
    {
      fprintf(stderr, "Unable to open output file '%s'\n", argv[2]);
      fclose(infile);
      infile = (FILE *)0;
    }
  }

  if (!infile)
  {
    char *appname = &(argv[0][strlen(argv[0])]);
    while (appname > argv[0])
    {
      appname--;
      if (*appname == '/' || *appname == '\\' || *appname == ':')
      {
        appname++;
        break;
      } 
    }
    fprintf(stderr, 
        "Usage: %s <inputfilename> [<outputfilename>]\n"
        "\nIf an outout file is specified the results will be"
        "\nbe post-processed, and only the file entries will appear"
        "\n(or all comments if there are no file entries)."
        "\nNot specifying an output file causes %s to run in \"debug\""
        "\nmode, ie results are printed as lines are parsed."
        "\nIf a filename is a single dash ('-'), stdin/stdout is used."
        "\n", appname, appname );
  }
  else
  {
    char *cmnt_buf = (char *)0;
    unsigned int cmnt_buf_sz = 0;
    char *list_buf = (char *)0;
    unsigned int list_buf_sz = 0;

    struct list_state state;
    char line[512];

    memset( &state, 0, sizeof(state) );
    while (fgets(line, sizeof(line), infile))
    {
      size_t linelen = strlen(line);
      if (linelen < (sizeof(line)-1))
      {
        if (linelen > 0 && line[linelen-1] == '\n')
          linelen--;
        if (do_it( outfile, line, linelen, &state, 
                   &cmnt_buf, &cmnt_buf_sz, &list_buf, &list_buf_sz) != 0)
        {
          fprintf(stderr, "Insufficient memory. Listing may be incomplete.\n"); 
          break;
        }
      }
      else
      {
        
        fprintf(stderr, "drop: %.*s", (int)linelen, line );
        while (linelen == sizeof(line))
        {
          if (!fgets(line, sizeof(line), infile))
            break;
          linelen = 0;
          while (linelen < sizeof(line) && line[linelen] != '\n')
            linelen++;
          fprintf(stderr, "%.*s", (int)linelen, line );
        }  
        fprintf(stderr, "\n");
      }
    }
    if (outfile)
    {
      if (list_buf)
        fwrite( list_buf, 1, list_buf_sz, outfile );
      else if (cmnt_buf)
        fwrite( cmnt_buf, 1, cmnt_buf_sz, outfile );
    }
    if (list_buf) 
      free(list_buf);
    if (cmnt_buf)
      free(cmnt_buf);

    if (need_close_in)
      fclose(infile);
    if (outfile && need_close_out)
      fclose(outfile);
  }

  return 0;
}
#endif
