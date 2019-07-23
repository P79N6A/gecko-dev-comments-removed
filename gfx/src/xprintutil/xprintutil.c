




































#include "xprintutil.h"
 
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <limits.h>
#include <errno.h>
#include <locale.h>

#ifdef XPU_USE_NSPR
#include "plstr.h"
#undef strtok_r
#define strtok_r(s1, s2, x) PL_strtok_r((s1), (s2), (x))
#endif 



static const char XPServerListSeparators[] = " \t\v\n\r\f";





#define MAKE_STRING_WRITABLE(str) (((str)?((str) = strdup(str)):0))
#define FREE_WRITABLE_STRING(str) free((void *)(str))
#define STRING_AS_WRITABLE(str) ((char *)(str))


static const char *XpuGetDefaultXpPrintername(void);
static const char *XpuGetXpServerList( void );
static const char *XpuEnumerateXpAttributeValue( const char *value, void **vcptr );
static const char *XpuGetCurrentAttributeGroup( void **vcptr );
static void XpuDisposeEnumerateXpAttributeValue( void **vc );
static Bool XpuEnumerateMediumSourceSizes( Display *pdpy, XPContext pcontext,
                                           const char **tray_name,
                                           const char **medium_name, int *mbool, 
                                           float *ma1, float *ma2, float *ma3, float *ma4,
                                           void **vcptr );
static void XpuDisposeEnumerateMediumSourceSizes( void **vc );






int XpuCheckExtension( Display *pdpy )
{
  char *display = XDisplayString(pdpy);
  short major = 0,
        minor = 0;

  if( XpQueryVersion(pdpy, &major, &minor) != 0 )
  {
    XPU_DEBUG_ONLY(printf("XpuCheckExtension: XpQueryVersion '%s' %d %d\n", XPU_NULLXSTR(display), (int)major, (int)minor));
    return(1);
  }
  else
  {
    XPU_DEBUG_ONLY(printf("XpuCheckExtension: XpQueryVersion '%s' returned 0(=Xprint not supported)\n", XPU_NULLXSTR(display)));
  }
  
  return(0);
}






static
const char *XpuGetDefaultXpPrintername(void)
{
  const char *s;
  
  s = getenv("XPRINTER");
  if( !s )
  {
    s = getenv("PDPRINTER");
    if( !s )
    {
      s = getenv("LPDEST");
      if( !s )
      {
        s = getenv("PRINTER");
      }
    }
  }  
  return s;
}

static
const char *XpuGetXpServerList( void )
{
  const char *s;
  


  s = getenv("XPSERVERLIST"); 
  if( s == NULL )
    s = "";
    
  return(s);
}


Bool XpuXprintServersAvailable( void )
{
  const char *s;
  int         c = 0;
  








  s = getenv("XPSERVERLIST");
  
  if (s)
  {
    while( *s++ )
    {
      if( !isspace(*s) )
        c++;
    }
  }
  

  return( c >= 2 );
}


static 
int XpuGetPrinter2( char *printer, char *display, Display **pdpyptr, XPContext *pcontextptr )
{
  Display   *pdpy;
  XPContext  pcontext;
  
  XPU_DEBUG_ONLY(printf("XpuGetPrinter2: probing display '%s' for '%s'\n", XPU_NULLXSTR(display), XPU_NULLXSTR(printer)));
  
  if( (pdpy = XOpenDisplay(display)) != NULL )
  {
    if( XpuCheckExtension(pdpy) )
    {
      XPPrinterList list;
      int           list_count;
      
      
      list = XpGetPrinterList(pdpy, printer, &list_count);        
      if( list != NULL ) XpFreePrinterList(list);
      
      
      if( (list != NULL) && (list_count > 0) )
      {
        if( (pcontext = XpCreateContext(pdpy, printer)) != None )
        {
          *pdpyptr     = pdpy;
          *pcontextptr = pcontext;
          return(1);
        }
        
        XPU_DEBUG_ONLY(printf("XpuGetPrinter2: could not create print context for '%s'\n", XPU_NULLXSTR(printer)));
      }
    }
    else
    {
      XPU_DEBUG_ONLY(printf("display '%s' does not support the Xprint extension\n", XPU_NULLXSTR(display)));
    }  

    XCloseDisplay(pdpy);
    return(0);
  }
  else
  {
    XPU_DEBUG_ONLY(printf("could not open display '%s'\n", XPU_NULLXSTR(display)));
    return(0);
  }
}



int XpuGetPrinter( const char *arg_printername, Display **pdpyptr, XPContext *pcontextptr )
{
  Display       *pdpy;
  XPContext      pcontext;
  char          *printername;
  char          *s;
  char          *tok_lasts;
  
  *pdpyptr     = NULL;
  *pcontextptr = None;
  
  XPU_DEBUG_ONLY(printf("XpuGetPrinter: looking for '%s'\n", XPU_NULLXSTR(arg_printername)));
  
  
  printername = strdup(arg_printername);
  if( printername == NULL )
    return(0);
  
  if( (s = strtok_r(printername, "@", &tok_lasts)) != NULL )
  {
    char *name = s;
    char *display = strtok_r(NULL, "@", &tok_lasts);
    
    
    if( display != NULL )
    {
      if( XpuGetPrinter2(name, display, pdpyptr, pcontextptr) )
      {
        free(printername);
        return(1);
      }
    }
    
    else
    {
      char *sl = strdup(XpuGetXpServerList());
      
      if( sl != NULL )
      {
        for( display = strtok_r(sl, XPServerListSeparators, &tok_lasts) ; 
             display != NULL ; 
             display = strtok_r(NULL, XPServerListSeparators, &tok_lasts) )
        {       
          if( XpuGetPrinter2(name, display, pdpyptr, pcontextptr) )
          {
            free(sl);
            free(printername);
            return(1);
          } 
        }
        
        free(sl);
      }
    }
  }
  
  free(printername);
  XPU_DEBUG_ONLY(printf("XpuGetPrinter: failure\n"));
  
  return(0);
}


void XpuClosePrinterDisplay(Display *pdpy, XPContext pcontext)
{
  if( pdpy )
  {
    if( pcontext != None )
      XpDestroyContext(pdpy, pcontext);
      
    XCloseDisplay(pdpy);
  }
}

void XpuSetOneAttribute( Display *pdpy, XPContext pcontext, 
                         XPAttributes type, const char *attribute_name, const char *value, XPAttrReplacement replacement_rule )
{
  
  char *buffer = (char *)malloc(strlen(attribute_name)+strlen(value)+4);
  
  if( buffer != NULL )
  {
    sprintf(buffer, "%s: %s", attribute_name, value);      
    XpSetAttributes(pdpy, pcontext, type, buffer, replacement_rule);
    free(buffer);
  }  
}

void XpuSetOneLongAttribute( Display *pdpy, XPContext pcontext, 
                             XPAttributes type, const char *attribute_name, long value, XPAttrReplacement replacement_rule )
{
  
  char *buffer = (char *)malloc(strlen(attribute_name)+32+4);
  
  if( buffer != NULL )
  {
    sprintf(buffer, "%s: %ld", attribute_name, value);      
    XpSetAttributes(pdpy, pcontext, type, buffer, replacement_rule);
    free(buffer);
  }  
}





int XpuCheckSupported( Display *pdpy, XPContext pcontext, XPAttributes type, const char *attribute_name, const char *query )
{
  char *value;
  void *tok_lasts;
  
  MAKE_STRING_WRITABLE(attribute_name);
  if( attribute_name == NULL )
    return(0);
    
  value = XpGetOneAttribute(pdpy, pcontext, type, STRING_AS_WRITABLE(attribute_name));   
  
  XPU_DEBUG_ONLY(printf("XpuCheckSupported: XpGetOneAttribute(%s) returned '%s'\n", XPU_NULLXSTR(attribute_name), XPU_NULLXSTR(value)));

  FREE_WRITABLE_STRING(attribute_name);
  
  if( value != NULL )
  {
    const char *s;
    
    for( s = XpuEnumerateXpAttributeValue(value, &tok_lasts) ; s != NULL ; s = XpuEnumerateXpAttributeValue(NULL, &tok_lasts) )
    {
      XPU_DEBUG_ONLY(printf("XpuCheckSupported: probing '%s'=='%s'\n", XPU_NULLXSTR(s), XPU_NULLXSTR(query)));
      if( !strcmp(s, query) )
      {
        XFree(value);
        XpuDisposeEnumerateXpAttributeValue(&tok_lasts);
        return(1);
      }  
    }
    
    XpuDisposeEnumerateXpAttributeValue(&tok_lasts);
    XFree(value);
  }  
  
  return(0);
}


int XpuSetJobTitle( Display *pdpy, XPContext pcontext, const char *title )
{
  if( XpuGetSupportedJobAttributes(pdpy, pcontext) & XPUATTRIBUTESUPPORTED_JOB_NAME )
  {
    char *encoded_title;
    
    encoded_title = XpuResourceEncode(title);
    if (!encoded_title)
      return(0);
    XpuSetOneAttribute(pdpy, pcontext, XPJobAttr, "*job-name", encoded_title, XPAttrMerge);
    XpuResourceFreeString(encoded_title);
    return(1);
  }
  else
  {
    XPU_DEBUG_ONLY(printf("XpuSetJobTitle: XPUATTRIBUTESUPPORTED_JOB_NAME not supported ('%s')\n", XPU_NULLXSTR(title)));
    return(0); 
  }  
}
    
int XpuGetOneLongAttribute( Display *pdpy, XPContext pcontext, XPAttributes type, const char *attribute_name, long *result )
{
  char *s;
  
  MAKE_STRING_WRITABLE(attribute_name);
  if( attribute_name == NULL )
    return(0);
  s = XpGetOneAttribute(pdpy, pcontext, type, STRING_AS_WRITABLE(attribute_name));
  
  if(s && *s) 
  {
    long tmp;
    
    XPU_DEBUG_ONLY(printf("XpuGetOneLongAttribute: '%s' got '%s'\n", XPU_NULLXSTR(attribute_name), XPU_NULLXSTR(s)));
    
    tmp = strtol(s, (char **)NULL, 10);
    
    if( !(((tmp == 0L) || (tmp == LONG_MIN) || (tmp == LONG_MAX)) && 
          ((errno == ERANGE) || (errno == EINVAL))) )
    {
      *result = tmp;
      XFree(s);
      XPU_DEBUG_ONLY(printf("XpuGetOneLongAttribute: result %ld\n", *result));
      FREE_WRITABLE_STRING(attribute_name);
      return(1);
    }            
  }
  
  if( s != NULL ) 
    XFree(s);
  
  FREE_WRITABLE_STRING(attribute_name);
  
  return(0);
}


#ifdef DEBUG

void dumpXpAttributes( Display *pdpy, XPContext pcontext )
{
  char *s;
  printf("------------------------------------------------\n");
  printf("--> Job\n%s\n",         s=XpuGetJobAttributes(pdpy, pcontext));     XFree(s);
  printf("--> Doc\n%s\n",         s=XpuGetDocAttributes(pdpy, pcontext));     XFree(s);
  printf("--> Page\n%s\n",        s=XpuGetPageAttributes(pdpy, pcontext));    XFree(s);
  printf("--> Printer\n%s\n",     s=XpuGetPrinterAttributes(pdpy, pcontext)); XFree(s);
  printf("--> Server\n%s\n",      s=XpuGetServerAttributes(pdpy, pcontext));  XFree(s);
  printf("image resolution %d\n", (int)XpGetImageResolution(pdpy, pcontext));
  printf("------------------------------------------------\n");
}
#endif     


typedef struct XpuIsNotifyEventContext_
{
  int event_base;
  int detail;
} XpuIsNotifyEventContext;

static
Bool IsXpNotifyEvent( Display *pdpy, XEvent *ev, XPointer arg )
{
  Bool match;
  XpuIsNotifyEventContext *context = (XpuIsNotifyEventContext *)arg;
  XPPrintEvent *pev = (XPPrintEvent *)ev;
  
  match = pev->type == (context->event_base+XPPrintNotify) && 
          pev->detail == context->detail;

  XPU_DEBUG_ONLY(printf("XpuWaitForPrintNotify: %d=IsXpNotifyEvent(%d,%d)\n",
                 (int)match,
                 (int)pev->type,
                 (int)pev->detail));
  return match;
}

void XpuWaitForPrintNotify( Display *pdpy, int xp_event_base, int detail )
{
  XEvent                  dummy;
  XpuIsNotifyEventContext matchcontext;

  matchcontext.event_base = xp_event_base;
  matchcontext.detail     = detail;
  XIfEvent(pdpy, &dummy, IsXpNotifyEvent, (XPointer)&matchcontext);
}      

static
const char *skip_matching_brackets(const char *start)
{
  const char *s     = start;
  int         level = 0;
  
  if( !start )
      return(NULL);
  
  do
  {
    switch(*s++)
    {
      case '\0': return(NULL);
      case '{': level++; break;
      case '}': level--; break;
    }
  } while(level > 0);

  return(s);
}


static
const char *search_next_space(const char *start)
{
  const char *s     = start;
  int         level = 0;
  
  if( !start )
    return(NULL);
  
  for(;;)
  {   
    if( isspace(*s) )
      return(s);

    if( *s=='\0' )
      return(NULL);      

    s++;
  }
}


typedef struct _XpuAttributeValueEnumeration
{
  char   *value;
  size_t  original_value_len; 
  char   *group;
  char   *start;
  char   *s;
} XpuAttributeValueEnumeration;



static
const char *XpuEnumerateXpAttributeValue( const char *value, void **vcptr )
{
  XpuAttributeValueEnumeration **cptr = (XpuAttributeValueEnumeration **)vcptr;
  XpuAttributeValueEnumeration  *context;
  const char                    *tmp;
  
  if( !cptr )
    return(NULL);
    
  if( value )
  {
    XpuAttributeValueEnumeration *e;
    const char *s = value;
    Bool        isGroup = FALSE;
  
    e = (XpuAttributeValueEnumeration *)malloc(sizeof(XpuAttributeValueEnumeration));
    if( !e )
      return NULL;
  
    
    while(*s=='{' && isGroup==FALSE)
    {
      s++;
      isGroup = TRUE;
    }  
    
    while(isspace(*s))
      s++;
    
    e->group = NULL;
    
    
    if( isGroup )
    { 
      tmp = s;  
      while(!isspace(*s))
        s++;
      if(strncmp(tmp, "''", s-tmp) != 0)
      {
        e->group = strdup(tmp);
        e->group[s-tmp] = '\0';
      }
    }
  
    e->original_value_len = strlen(s);
    e->value = (char *)malloc(e->original_value_len+4); 
    strcpy(e->value, s);
    memset(e->value+e->original_value_len+1, 0, 3); 
    e->start = e->s = e->value;
    
    *cptr = e;
  }
  
  context = *cptr;
  
  if( !context || !context->s )
    return(NULL);
   
  
  while(isspace(*(context->s)) || *(context->s)=='\''  )
    context->s++;

  if( *(context->s) == '\0' )
    return(NULL);

  context->start = context->s;
  if( *(context->start) == '{' )
    context->s = (char *)skip_matching_brackets(context->start);
  else
    context->s = (char *)search_next_space(context->start);
    
  
  if( context->s )
  {   
    *(context->s) = '\0';
    context->s++;
  }
  
  
  tmp = context->start;
  while(isspace(*tmp))
    tmp++;   
  if( *tmp=='}' )
  {
    void *prev_cptr = *vcptr;
    
    tmp+=2; 
    if( *tmp!='\0' )
    {
      const char *ret;
   
      
      *vcptr = NULL;
      ret = XpuEnumerateXpAttributeValue(tmp, vcptr);
    
      
      XpuDisposeEnumerateXpAttributeValue(&prev_cptr);
    
      return(ret);
    }
    else
    {
      return(NULL);
    }
  }
  
  return(context->start);   
}


static
const char *XpuGetCurrentAttributeGroup( void **vcptr )
{
  XpuAttributeValueEnumeration **cptr = (XpuAttributeValueEnumeration **)vcptr;
  if( !cptr )
    return(NULL);
  if( !*cptr )
    return(NULL);
    
  return((*cptr)->group);
}


static
void XpuDisposeEnumerateXpAttributeValue( void **vc )
{ 
  if( vc )
  {
    XpuAttributeValueEnumeration *context = *((XpuAttributeValueEnumeration **)vc);
    free(context->value);
    if(context->group)
      free(context->group);
    free(context);   
  }
}



static
Bool XpuParseMediumSourceSize( const char *value, 
                               const char **medium_name, int *mbool, 
                               float *ma1, float *ma2, float *ma3, float *ma4 )
{
  const char *s;
  char       *d, 
             *name;
  char       *boolbuf;
  size_t      value_len;
  int         num_input_items;
  const char *cur_locale;
  
  if( value && value[0]!='{' && value[0]!='\0' )
    return(False);
    
  value_len = strlen(value);
  
  
  
  name = (char *)malloc(value_len*2 + 4);
  boolbuf = name + value_len+2; 
  
  
  s = value;
  d = name;
  do
  {
    *d = tolower(*s);
    
    if( *s!='{' && *s!='}' )
      d++;
    
    s++;  
  }
  while(*s);
  *d = '\0';
    
  
  d = (char *)search_next_space(name);
  if( !d )
  {
    free(name);
    return(False);
  }  
  *d = '\0';
  *medium_name = name;
  
  
  d++;
  

  




  {
#define CUR_LOCALE_SIZE 256
    char cur_locale[CUR_LOCALE_SIZE+1];
    strncpy(cur_locale, setlocale(LC_NUMERIC, NULL), CUR_LOCALE_SIZE);
    cur_locale[CUR_LOCALE_SIZE]='\0';
    setlocale(LC_NUMERIC, "C"); 
    num_input_items = sscanf(d, "%s %f %f %f %f", boolbuf, ma1, ma2, ma3, ma4);
    setlocale(LC_NUMERIC, cur_locale);
#undef CUR_LOCALE_SIZE
  }

  if( num_input_items != 5 )
  {
    free(name);
    return(False);
  }

  if( !strcmp(boolbuf, "true") )
    *mbool = True;
  else if( !strcmp(boolbuf, "false") )
    *mbool = False;
  else
  {
    free(name);
    return(False);    
  }
  return(True);
}




static
Bool XpuEnumerateMediumSourceSizes( Display *pdpy, XPContext pcontext,
                                    const char **tray_name,
                                    const char **medium_name, int *mbool, 
                                    float *ma1, float *ma2, float *ma3, float *ma4,
                                    void **vcptr )
{
  const char *medium_spec;
  const char *value = NULL;
  
  if( pdpy && pcontext )
  {
    value = XpGetOneAttribute(pdpy, pcontext, XPPrinterAttr, "medium-source-sizes-supported");
    if( !value )
      return(False);
  }

  while(1)
  {  
    medium_spec = XpuEnumerateXpAttributeValue(value, vcptr);
    
    if( value )
    {
      XFree((void *)value);
      value = NULL;
    }

    
    if( !medium_spec )
      return(False);

    if (XpuParseMediumSourceSize(medium_spec, 
                                 medium_name, mbool, 
                                 ma1, ma2, ma3, ma4))
    {
      *tray_name = XpuGetCurrentAttributeGroup(vcptr);
      return(True);
    }
    else
    {
      
      fprintf(stderr, "XpuEnumerateMediumSourceSize: error parsing '%s'\n", medium_spec);
    }
  }
     
}

static
void XpuDisposeEnumerateMediumSourceSizes( void **vc )
{
  XpuDisposeEnumerateXpAttributeValue(vc);
}  





XPPrinterList XpuGetPrinterList( const char *printer, int *res_list_count )
{
  XPPrinterRec *rec = NULL;
  int           rec_count = 1; 

  char         *sl;
  const char   *default_printer_name = XpuGetDefaultXpPrintername();
  int           default_printer_rec_index = -1;

  if( !res_list_count )
    return(NULL); 
  
  sl = strdup(XpuGetXpServerList());
  MAKE_STRING_WRITABLE(printer);
    
  if( sl != NULL )
  {
    char *display;
    char *tok_lasts;
    
    for( display = strtok_r(sl, XPServerListSeparators, &tok_lasts) ; 
         display != NULL ; 
         display = strtok_r(NULL, XPServerListSeparators, &tok_lasts) )
    {
      Display *pdpy;
      
      if( (pdpy = XOpenDisplay(display)) != NULL )
      {
        XPPrinterList list;
        int           list_count;
        size_t        display_len = strlen(display);

        
        list = XpGetPrinterList(pdpy, STRING_AS_WRITABLE(printer), &list_count);        
      
        if( list && list_count )
        {
          int i;
          
          for( i = 0 ; i < list_count ; i++ )
          {
            char *s;
            
            




            if( !list[i].name )
              continue;
            
            rec_count++;
            rec = (XPPrinterRec *)realloc(rec, sizeof(XPPrinterRec)*rec_count);
            if( !rec ) 
              break;
              
            s = (char *)malloc(strlen(list[i].name)+display_len+4);
            sprintf(s, "%s@%s", list[i].name, display);
            rec[rec_count-2].name = s;
            rec[rec_count-2].desc = (list[i].desc)?(strdup(list[i].desc)):(NULL);
            
            
            if( default_printer_name )
            {
              


              if( (!strcmp(list[i].name, default_printer_name)) ||
                  (!strcmp(s,            default_printer_name)) )
              {
                

                default_printer_rec_index = rec_count-2;
              }
            }  
          }
          
          XpFreePrinterList(list);
        }
                 
        XCloseDisplay(pdpy);
      }
    }
    
    free(sl);
  }
  
  if( rec )
  {
    




    rec[rec_count-1].name = NULL;
    rec[rec_count-1].desc = NULL;
    rec_count--;
  }
  else
  {
    rec_count = 0;
  }
  
  
  if( (default_printer_rec_index != -1) && rec )
  {
    XPPrinterRec tmp;
    tmp = rec[0];
    rec[0] = rec[default_printer_rec_index];
    rec[default_printer_rec_index] = tmp;
  }
    
  *res_list_count = rec_count;
  FREE_WRITABLE_STRING(printer);
  return(rec);
}      


void XpuFreePrinterList( XPPrinterList list )
{
  if( list )
  {
    XPPrinterRec *curr = list;
  
    

    while( curr->name != NULL )
    {
      free(curr->name);
      if(curr->desc)
        free(curr->desc);
      curr++;
    }
  
    free(list);
  }
}


int XpuSetDocumentCopies( Display *pdpy, XPContext pcontext, long num_copies )
{
  if( XpuGetSupportedDocAttributes(pdpy, pcontext) & XPUATTRIBUTESUPPORTED_COPY_COUNT)
  {
    XpuSetOneLongAttribute(pdpy, pcontext, XPDocAttr, "*copy-count", num_copies, XPAttrMerge);
    return(1);
  }
  else
  {
    XPU_DEBUG_ONLY(printf("XpuSetContentOrientation: XPUATTRIBUTESUPPORTED_COPY_COUNT not supported\n"));
       
    
    return(0);
  }  
}

XpuMediumSourceSizeList XpuGetMediumSourceSizeList( Display *pdpy, XPContext pcontext, int *numEntriesPtr )
{
  XpuMediumSourceSizeList list = NULL;
  int                     rec_count = 1; 

  Bool                    status;
  float                   ma1,
                          ma2,
                          ma3,
                          ma4;
  char                   *value;
  void                   *tok_lasts;
  const char             *tray_name,
                         *medium_name;
  int                     mbool;
  const char             *default_tray,
                         *default_medium;
  int                     default_medium_rec_index = -1;
  
  default_tray   = XpGetOneAttribute(pdpy, pcontext, XPDocAttr, "default-input-tray");
  if(!default_tray)
  {
    fprintf(stderr, "XpuGetMediumSourceSizeList: Internal error, no 'default-input-tray' found.\n");
    return(NULL);
  }
  default_medium = XpGetOneAttribute(pdpy, pcontext, XPDocAttr, "default-medium");
  if(!default_medium)
  {
    fprintf(stderr, "XpuGetMediumSourceSizeList: Internal error, no 'default-medium' found.\n");
    XFree((void *)default_tray);
    return(NULL);
  }
  
  for( status = XpuEnumerateMediumSourceSizes(pdpy, pcontext, &tray_name, &medium_name, &mbool,
                                              &ma1, &ma2, &ma3, &ma4, &tok_lasts) ;
       status != False ;
       status = XpuEnumerateMediumSourceSizes(NULL, None,     &tray_name, &medium_name, &mbool, 
                                              &ma1, &ma2, &ma3, &ma4, &tok_lasts) )
  {
    rec_count++;
    list = (XpuMediumSourceSizeRec *)realloc(list, sizeof(XpuMediumSourceSizeRec)*rec_count);
    if( !list )
      return(NULL);
    
    list[rec_count-2].tray_name   = (tray_name)?(strdup(tray_name)):(NULL);
    list[rec_count-2].medium_name = strdup(medium_name);
    list[rec_count-2].mbool       = mbool;
    list[rec_count-2].ma1         = ma1;
    list[rec_count-2].ma2         = ma2;
    list[rec_count-2].ma3         = ma3;
    list[rec_count-2].ma4         = ma4;
    
    
    if( (!strcmp(medium_name, default_medium)) && 
        ((tray_name && (*default_tray))?(!strcmp(tray_name, default_tray)):(True)) )
    {
      default_medium_rec_index = rec_count-2;
    }
  }  

  XpuDisposeEnumerateMediumSourceSizes(&tok_lasts);

  if( list )
  {
    



    list[rec_count-1].tray_name  = NULL;
    list[rec_count-1].medium_name = NULL;
    rec_count--;
  }
  else
  {
    rec_count = 0;
  }

  
  if( (default_medium_rec_index != -1) && list )
  {
    XpuMediumSourceSizeRec tmp;
    tmp = list[0];
    list[0] = list[default_medium_rec_index];
    list[default_medium_rec_index] = tmp;
  }

  *numEntriesPtr = rec_count; 
  return(list);
}

void XpuFreeMediumSourceSizeList( XpuMediumSourceSizeList list )
{
  if( list )
  {
    XpuMediumSourceSizeRec *curr = list;
  
    

    while( curr->medium_name != NULL )
    {
      if( curr->tray_name)
        free((void *)curr->tray_name);
      free((void *)curr->medium_name);
      curr++;
    }
  
    free(list);
  }
}

static
int XpuSetMediumSourceSize( Display *pdpy, XPContext pcontext, XPAttributes type, XpuMediumSourceSizeRec *medium_spec )
{
  


  if (medium_spec->tray_name)
  {
    XpuSetOneAttribute(pdpy, pcontext, type, "*default-input-tray", medium_spec->tray_name, XPAttrMerge);
  }
  XpuSetOneAttribute(pdpy, pcontext, type, "*default-medium", medium_spec->medium_name, XPAttrMerge);
  
  return( 1 );
}


int XpuSetDocMediumSourceSize( Display *pdpy, XPContext pcontext, XpuMediumSourceSizeRec *medium_spec )
{
  XpuSupportedFlags doc_supported_flags;
  
  doc_supported_flags = XpuGetSupportedDocAttributes(pdpy, pcontext);

  if( (doc_supported_flags & XPUATTRIBUTESUPPORTED_DEFAULT_MEDIUM) == 0 )
    return( 0 );
    
  if (medium_spec->tray_name)
  {
    if( (doc_supported_flags & XPUATTRIBUTESUPPORTED_DEFAULT_INPUT_TRAY) == 0 )
      return( 0 );  
  }

  return XpuSetMediumSourceSize(pdpy, pcontext, XPDocAttr, medium_spec);
}


int XpuSetPageMediumSourceSize( Display *pdpy, XPContext pcontext, XpuMediumSourceSizeRec *medium_spec )
{
  XpuSupportedFlags page_supported_flags;
  
  page_supported_flags = XpuGetSupportedPageAttributes(pdpy, pcontext);

  if( (page_supported_flags & XPUATTRIBUTESUPPORTED_DEFAULT_MEDIUM) == 0 )
    return( 0 );
    
  if (medium_spec->tray_name)
  {
    if( (page_supported_flags & XPUATTRIBUTESUPPORTED_DEFAULT_INPUT_TRAY) == 0 )
      return( 0 );  
  }

  return XpuSetMediumSourceSize(pdpy, pcontext, XPPageAttr, medium_spec);
}

#ifndef ABS
#define ABS(x) ((x)<0?-(x):(x))
#endif 
#define MORE_OR_LESS_EQUAL(a, b, tolerance) (ABS((a) - (b)) <= (tolerance))

XpuMediumSourceSizeRec *
XpuFindMediumSourceSizeBySize( XpuMediumSourceSizeList mlist, int mlist_count, 
                               float page_width_mm, float page_height_mm, float tolerance )
{
  int i;
  for( i = 0 ; i < mlist_count ; i++ )
  {
    XpuMediumSourceSizeRec *curr = &mlist[i];
    float total_width  = curr->ma1 + curr->ma2,
          total_height = curr->ma3 + curr->ma4;

    
    if( ((page_width_mm !=-1.f)?(MORE_OR_LESS_EQUAL(total_width,  page_width_mm,  tolerance)):(True)) &&
        ((page_height_mm!=-1.f)?(MORE_OR_LESS_EQUAL(total_height, page_height_mm, tolerance)):(True)) )
    {
      return(curr);
    }
  }

  return(NULL);
}

XpuMediumSourceSizeRec *
XpuFindMediumSourceSizeByBounds( XpuMediumSourceSizeList mlist, int mlist_count, 
                                 float m1, float m2, float m3, float m4, float tolerance )
{
  int i;
  for( i = 0 ; i < mlist_count ; i++ )
  {
    XpuMediumSourceSizeRec *curr = &mlist[i];

    
    if( ((m1!=-1.f)?(MORE_OR_LESS_EQUAL(curr->ma1, m1, tolerance)):(True)) &&
        ((m2!=-1.f)?(MORE_OR_LESS_EQUAL(curr->ma2, m2, tolerance)):(True)) &&
        ((m3!=-1.f)?(MORE_OR_LESS_EQUAL(curr->ma3, m3, tolerance)):(True)) &&
        ((m4!=-1.f)?(MORE_OR_LESS_EQUAL(curr->ma4, m4, tolerance)):(True)) )
    {
      return(curr);
    }
  }

  return(NULL);
}

XpuMediumSourceSizeRec *
XpuFindMediumSourceSizeByName( XpuMediumSourceSizeList mlist, int mlist_count, 
                               const char *tray_name, const char *medium_name )
{
  int i;
  for( i = 0 ; i < mlist_count ; i++ )
  {
    XpuMediumSourceSizeRec *curr = &mlist[i];

    
    if( ((tray_name && curr->tray_name)?(!strcasecmp(curr->tray_name, tray_name)):(tray_name==NULL)) &&
        ((medium_name)?(!strcasecmp(curr->medium_name, medium_name)):(True)) )
    {
      return(curr);
    }
  }

  return(NULL);
}

XpuResolutionList XpuGetResolutionList( Display *pdpy, XPContext pcontext, int *numEntriesPtr )
{
  XpuResolutionList list = NULL;
  int               rec_count = 1; 

  char             *value;
  char             *tok_lasts;
  const char       *s;
  long              default_resolution = -1;
  int               default_resolution_rec_index = -1;
  char              namebuf[64];

  
  if( XpuGetOneLongAttribute(pdpy, pcontext, XPDocAttr, "default-printer-resolution", &default_resolution) != 1 )
  {
    default_resolution = -1;
  }
  
  value = XpGetOneAttribute(pdpy, pcontext, XPPrinterAttr, "printer-resolutions-supported");
  if (!value)
  {
    fprintf(stderr, "XpuGetResolutionList: Internal error, no 'printer-resolutions-supported' XPPrinterAttr found.\n");
    return(NULL);
  }
  
  for( s = strtok_r(value, " ", &tok_lasts) ;
       s != NULL ;
       s = strtok_r(NULL, " ", &tok_lasts) )
  {
    long tmp;
    
    tmp = strtol(s, (char **)NULL, 10);
    
    if( ((tmp == 0L) || (tmp == LONG_MIN) || (tmp == LONG_MAX)) && 
        ((errno == ERANGE) || (errno == EINVAL)) )
    {
      fprintf(stderr, "XpuGetResolutionList: Internal parser errror for '%s'.\n", s);
      continue;
    }    
  
    rec_count++;
    list = (XpuResolutionRec *)realloc(list, sizeof(XpuResolutionRec)*rec_count);
    if( !list )
      return(NULL);
    
    sprintf(namebuf, "%lddpi", tmp);
    list[rec_count-2].name   = strdup(namebuf);
    list[rec_count-2].x_dpi  = tmp;
    list[rec_count-2].y_dpi  = tmp;

    if( default_resolution != -1 )
    {
      
      if( (list[rec_count-2].x_dpi == default_resolution) &&
          (list[rec_count-2].y_dpi == default_resolution) )
      {
        default_resolution_rec_index = rec_count-2;
      }
    }  
  }  

  XFree(value);

  if( list )
  {
    



    list[rec_count-1].name   = NULL;
    list[rec_count-1].x_dpi  = -1;
    list[rec_count-1].y_dpi  = -1;
    rec_count--;
  }
  else
  {
    rec_count = 0;
  }

  
  if( (default_resolution_rec_index != -1) && list )
  {
    XpuResolutionRec tmp;
    tmp = list[0];
    list[0] = list[default_resolution_rec_index];
    list[default_resolution_rec_index] = tmp;
  }

  *numEntriesPtr = rec_count; 
  return(list);
}

void XpuFreeResolutionList( XpuResolutionList list )
{
  if( list )
  { 
    XpuResolutionRec *curr = list;
  
    

    while( curr->name != NULL )
    {
      free((void *)curr->name);
      curr++;
    }  

    free(list);
  }
}



XpuResolutionRec *XpuFindResolutionByName( XpuResolutionList list, int list_count, const char *name)
{
  int i;
  
  for( i = 0 ; i < list_count ; i++ )
  {
    XpuResolutionRec *curr = &list[i];
    if (!strcasecmp(curr->name, name))
      return curr;
  }

  return NULL;
}






Bool XpuGetResolution( Display *pdpy, XPContext pcontext, long *x_dpi_ptr, long *y_dpi_ptr )
{
  long dpi;

  
  if( XpuGetOneLongAttribute(pdpy, pcontext, XPPageAttr, "default-printer-resolution", &dpi) == 1 )
  {
    *x_dpi_ptr = dpi;
    *y_dpi_ptr = dpi;
    return True;
  }

  
  if( XpuGetOneLongAttribute(pdpy, pcontext, XPDocAttr, "default-printer-resolution", &dpi) == 1 )
  {
    *x_dpi_ptr = dpi;
    *y_dpi_ptr = dpi;
    return True;
  }

  return False;
}

static
int XpuSetResolution( Display *pdpy, XPContext pcontext, XPAttributes type, XpuResolutionRec *rec )
{
  if( rec->x_dpi != rec->y_dpi )
  {
    fprintf(stderr, "XpuSetResolution: internal error: x_dpi != y_dpi not supported yet.\n");
    return 0;
  }

  XpuSetOneLongAttribute(pdpy, pcontext, type, "*default-printer-resolution", rec->x_dpi, XPAttrMerge); 
  return( 1 );
}




int XpuSetDocResolution( Display *pdpy, XPContext pcontext, XpuResolutionRec *rec )
{
  if( (XpuGetSupportedDocAttributes(pdpy, pcontext) & XPUATTRIBUTESUPPORTED_DEFAULT_PRINTER_RESOLUTION) == 0 )
    return( 0 );
    
  return XpuSetResolution(pdpy, pcontext, XPDocAttr, rec);
}





int XpuSetPageResolution( Display *pdpy, XPContext pcontext, XpuResolutionRec *rec )
{
  if( (XpuGetSupportedPageAttributes(pdpy, pcontext) & XPUATTRIBUTESUPPORTED_DEFAULT_PRINTER_RESOLUTION) == 0 )
    return( 0 );
    
  return XpuSetResolution(pdpy, pcontext, XPPageAttr, rec);
}

XpuOrientationList XpuGetOrientationList( Display *pdpy, XPContext pcontext, int *numEntriesPtr )
{
  XpuOrientationList list = NULL;
  int                rec_count = 1; 

  char              *value;
  char              *tok_lasts;
  const char        *s;
  const char        *default_orientation = NULL;
  int                default_orientation_rec_index = -1;

  
  default_orientation = XpGetOneAttribute(pdpy, pcontext, XPDocAttr, "content-orientation"); 
  if( !default_orientation )
  {
    fprintf(stderr, "XpuGetOrientationList: Internal error, no 'content-orientation' XPDocAttr found.\n");
    return(NULL);
  }
  
  value = XpGetOneAttribute(pdpy, pcontext, XPPrinterAttr, "content-orientations-supported");
  if (!value)
  {
    fprintf(stderr, "XpuGetOrientationList: Internal error, no 'content-orientations-supported' XPPrinterAttr found.\n");
    return(NULL);
  }
  
  for( s = strtok_r(value, " ", &tok_lasts) ;
       s != NULL ;
       s = strtok_r(NULL, " ", &tok_lasts) )
  { 
    rec_count++;
    list = (XpuOrientationRec *)realloc(list, sizeof(XpuOrientationRec)*rec_count);
    if( !list )
      return(NULL);
    
    list[rec_count-2].orientation = strdup(s);

    
    if( !strcmp(list[rec_count-2].orientation, default_orientation) )
    {
      default_orientation_rec_index = rec_count-2;
    }
  }  

  XFree(value);
  XFree((void *)default_orientation);

  if( list )
  {
    



    list[rec_count-1].orientation = NULL;
    rec_count--;
  }
  else
  {
    rec_count = 0;
  }

  
  if( (default_orientation_rec_index != -1) && list )
  {
    XpuOrientationRec tmp;
    tmp = list[0];
    list[0] = list[default_orientation_rec_index];
    list[default_orientation_rec_index] = tmp;
  }

  *numEntriesPtr = rec_count; 
  return(list);
}

void XpuFreeOrientationList( XpuOrientationList list )
{
  if( list )
  {
    XpuOrientationRec *curr = list;
  
    

    while( curr->orientation != NULL )
    {
      free((void *)curr->orientation);
      curr++;
    }   
    free(list);
  }
}

XpuOrientationRec *
XpuFindOrientationByName( XpuOrientationList list, int list_count, const char *orientation )
{
  int i;
  
  for( i = 0 ; i < list_count ; i++ )
  {
    XpuOrientationRec *curr = &list[i];
    if (!strcasecmp(curr->orientation, orientation))
      return curr;
  }

  return(NULL);
}

static
int XpuSetOrientation( Display *pdpy, XPContext pcontext, XPAttributes type, XpuOrientationRec *rec )
{
  XpuSetOneAttribute(pdpy, pcontext, type, "*content-orientation", rec->orientation, XPAttrMerge);
  return(1);
}




int XpuSetDocOrientation( Display *pdpy, XPContext pcontext, XpuOrientationRec *rec )
{
  if( (XpuGetSupportedDocAttributes(pdpy, pcontext) & XPUATTRIBUTESUPPORTED_CONTENT_ORIENTATION) == 0 )
    return( 0 );
    
  return XpuSetOrientation(pdpy, pcontext, XPDocAttr, rec);
}





int XpuSetPageOrientation( Display *pdpy, XPContext pcontext, XpuOrientationRec *rec )
{
  if( (XpuGetSupportedPageAttributes(pdpy, pcontext) & XPUATTRIBUTESUPPORTED_CONTENT_ORIENTATION) == 0 )
    return( 0 );
    
  return XpuSetOrientation(pdpy, pcontext, XPPageAttr, rec);
}

XpuPlexList XpuGetPlexList( Display *pdpy, XPContext pcontext, int *numEntriesPtr )
{
  XpuPlexList  list = NULL;
  int          rec_count = 1; 

  char        *value;
  char        *tok_lasts;
  const char  *s;
  const char  *default_plex = NULL;
  int          default_plex_rec_index = -1;

  
  default_plex = XpGetOneAttribute(pdpy, pcontext, XPDocAttr, "plex"); 
  if( !default_plex )
  {
    fprintf(stderr, "XpuGetPlexList: Internal error, no 'plex' XPDocAttr found.\n");
    return(NULL);
  }
   
  value = XpGetOneAttribute(pdpy, pcontext, XPPrinterAttr, "plexes-supported");
  if (!value)
  {
    fprintf(stderr, "XpuGetPlexList: Internal error, no 'plexes-supported' XPPrinterAttr found.\n");
    return(NULL);
  }
  
  for( s = strtok_r(value, " ", &tok_lasts) ;
       s != NULL ;
       s = strtok_r(NULL, " ", &tok_lasts) )
  { 
    rec_count++;
    list = (XpuPlexRec *)realloc(list, sizeof(XpuPlexRec)*rec_count);
    if( !list )
      return(NULL);
    
    list[rec_count-2].plex = strdup(s);

    
    if( !strcmp(list[rec_count-2].plex, default_plex) )
    {
      default_plex_rec_index = rec_count-2;
    }
  }  

  XFree(value);
  XFree((void *)default_plex);

  if( list )
  {
    



    list[rec_count-1].plex = NULL;
    rec_count--;
  }
  else
  {
    rec_count = 0;
  }

  
  if( (default_plex_rec_index != -1) && list )
  {
    XpuPlexRec tmp;
    tmp = list[0];
    list[0] = list[default_plex_rec_index];
    list[default_plex_rec_index] = tmp;
  }

  *numEntriesPtr = rec_count; 
  return(list);
}

void XpuFreePlexList( XpuPlexList list )
{
  if( list )
  {
    XpuPlexRec *curr = list;
  
    

    while( curr->plex != NULL )
    {
      free((void *)curr->plex);
      curr++;
    }   
    free(list);
  }
}

XpuPlexRec *
XpuFindPlexByName( XpuPlexList list, int list_count, const char *plex )
{
  int i;
  
  for( i = 0 ; i < list_count ; i++ )
  {
    XpuPlexRec *curr = &list[i];
    if (!strcasecmp(curr->plex, plex))
      return curr;
  }

  return(NULL);
}

static
int XpuSetContentPlex( Display *pdpy, XPContext pcontext, XPAttributes type, XpuPlexRec *rec )
{
  XpuSetOneAttribute(pdpy, pcontext, type, "*plex", rec->plex, XPAttrMerge);
  return(1);
}




int XpuSetDocPlex( Display *pdpy, XPContext pcontext, XpuPlexRec *rec )
{
  if( (XpuGetSupportedDocAttributes(pdpy, pcontext) & XPUATTRIBUTESUPPORTED_PLEX) == 0 )
    return( 0 );
    
  return XpuSetContentPlex(pdpy, pcontext, XPDocAttr, rec);
}





int XpuSetPagePlex( Display *pdpy, XPContext pcontext, XpuPlexRec *rec )
{
  if( (XpuGetSupportedPageAttributes(pdpy, pcontext) & XPUATTRIBUTESUPPORTED_PLEX) == 0 )
    return( 0 );
    
  return XpuSetContentPlex(pdpy, pcontext, XPPageAttr, rec);
}


XpuColorspaceList XpuGetColorspaceList( Display *pdpy, XPContext pcontext, int *numEntriesPtr )
{
  XpuColorspaceList list = NULL;
  int               rec_count = 1; 

  char              namebuf[256];  
  int               i;             
  int               nvi;           
  Screen           *pscreen;       
  XVisualInfo       viproto;       
  XVisualInfo      *vip;           

  pscreen = XpGetScreenOfContext(pdpy, pcontext);

  nvi = 0;
  viproto.screen = XScreenNumberOfScreen(pscreen);
  vip = XGetVisualInfo(pdpy, VisualScreenMask, &viproto, &nvi);
  if (!vip)
  {
    fprintf(stderr, "XpuGetColorspaceList: Internal error: vip == NULL\n");
    return NULL;
  }
  
  for( i = 0 ; i < nvi ; i++ )
  {
    XVisualInfo *vcurr = vip+i;
    char         cbuff[64];
    const char  *class = NULL;

#ifdef USE_MOZILLA_TYPES
    

    if( vcurr->depth > 24 )
      continue;
#endif 
 
    rec_count++;
    list = (XpuColorspaceRec *)realloc(list, sizeof(XpuColorspaceRec)*rec_count);
    if( !list )
      return NULL;

    

    switch (vcurr->class) {
      case StaticGray:   class = "StaticGray";  break;
      case GrayScale:    class = "GrayScale";   break;
      case StaticColor:  class = "StaticColor"; break;
      case PseudoColor:  class = "PseudoColor"; break;
      case TrueColor:    class = "TrueColor";   break;
      case DirectColor:  class = "DirectColor"; break;
      default: 
        sprintf (cbuff, "unknown_class_%x", vcurr->class);
        class = cbuff;
        break;
    }

    if (vcurr->bits_per_rgb == 8)
    {
      sprintf(namebuf, "%s/%dbit", class, vcurr->depth);
    }
    else
    {
      sprintf(namebuf, "%s/%dbit/%dbpg", class, vcurr->depth, vcurr->bits_per_rgb);
    }
    list[rec_count-2].name       = strdup(namebuf);
    list[rec_count-2].visualinfo = *vcurr;
  }  
 
  XFree((char *)vip);

  if( list )
  {
    



    list[rec_count-1].name = NULL;
    rec_count--;
  }
  else
  {
    rec_count = 0;
  }

  *numEntriesPtr = rec_count; 
  return(list);
}

void XpuFreeColorspaceList( XpuColorspaceList list )
{
  if( list )
  { 
    XpuColorspaceRec *curr = list;
  
    

    while( curr->name != NULL )
    {
      free((void *)curr->name);
      curr++;
    }  

    free(list);
  }
}

XpuColorspaceRec *
XpuFindColorspaceByName( XpuColorspaceList list, int list_count, const char *name )
{
  int i;
  
  for( i = 0 ; i < list_count ; i++ )
  {
    XpuColorspaceRec *curr = &list[i];
    if (!strcmp(curr->name, name))
      return curr;
  }

  return(NULL);
}

Bool XpuGetEnableFontDownload( Display *pdpy, XPContext pcontext )
{
  Bool  enableFontDownload;
  char *value;
  
  value = XpGetOneAttribute(pdpy, pcontext, XPPrinterAttr, "xp-listfonts-modes-supported"); 
  if( !value )
  {
    fprintf(stderr, "XpuGetEnableFontDownload: xp-listfonts-modes-supported printer attribute not found.\n");
    return False;
  }
  
  enableFontDownload = (strstr(value, "xp-list-glyph-fonts") != NULL);
  XFree(value);
  return enableFontDownload;
}

int XpuSetEnableFontDownload( Display *pdpy, XPContext pcontext, Bool enableFontDownload )
{
  char *value,
       *newvalue;
  
  value = XpGetOneAttribute(pdpy, pcontext, XPPrinterAttr, "xp-listfonts-modes-supported"); 
  if( !value )
  {
    fprintf(stderr, "XpuSetEnableFontDownload: xp-listfonts-modes-supported printer attribute not found.\n");
    return 0; 
  }
  
  
  if( enableFontDownload )
  {
    
    if( strstr(value, "xp-list-glyph-fonts") != NULL )
    {
      XFree(value);
      return 1; 
    }

    newvalue = malloc(strlen(value) + 33);
    if( !newvalue )
    {
      XFree(value);
      return 0; 
    }

    sprintf(newvalue, "%s xp-list-glyph-fonts", value);
    XpuSetOneAttribute(pdpy, pcontext, XPDocAttr, "*xp-listfonts-modes", newvalue, XPAttrMerge);

    free(newvalue);
    XFree(value);
    return 1; 
  }
  else
  {
    char *s, 
         *d; 
    
    
    d = strstr(value, "xp-list-glyph-fonts");
    if( d == NULL )
    {
      XFree(value);
      return 1; 
    }

    
    s = d+19;
    while( (*d++ = *s++) != '\0' )
      ;

    XpuSetOneAttribute(pdpy, pcontext, XPDocAttr, "*xp-listfonts-modes", value, XPAttrMerge);

    XFree(value);
    return 1; 
  } 
}


static
XpuSupportedFlags XpuGetSupportedAttributes( Display *pdpy, XPContext pcontext, XPAttributes type, const char *attribute_name )
{
  char              *value;
  void              *tok_lasts;
  XpuSupportedFlags  flags = 0;
  
  MAKE_STRING_WRITABLE(attribute_name);
  if( attribute_name == NULL )
    return(0);
    
  value = XpGetOneAttribute(pdpy, pcontext, type, STRING_AS_WRITABLE(attribute_name));   
  
  FREE_WRITABLE_STRING(attribute_name);
  
  if( value != NULL )
  {
    const char *s;
    
    for( s = XpuEnumerateXpAttributeValue(value, &tok_lasts) ; s != NULL ; s = XpuEnumerateXpAttributeValue(NULL, &tok_lasts) )
    {
           if( !strcmp(s, "job-name") )                   flags |= XPUATTRIBUTESUPPORTED_JOB_NAME;
      else if( !strcmp(s, "job-owner") )                  flags |= XPUATTRIBUTESUPPORTED_JOB_OWNER;
      else if( !strcmp(s, "notification-profile") )       flags |= XPUATTRIBUTESUPPORTED_NOTIFICATION_PROFILE;
      else if( !strcmp(s, "copy-count") )                 flags |= XPUATTRIBUTESUPPORTED_COPY_COUNT;
      else if( !strcmp(s, "document-format") )            flags |= XPUATTRIBUTESUPPORTED_DOCUMENT_FORMAT;
      else if( !strcmp(s, "content-orientation") )        flags |= XPUATTRIBUTESUPPORTED_CONTENT_ORIENTATION;
      else if( !strcmp(s, "default-printer-resolution") ) flags |= XPUATTRIBUTESUPPORTED_DEFAULT_PRINTER_RESOLUTION;
      else if( !strcmp(s, "default-input-tray") )         flags |= XPUATTRIBUTESUPPORTED_DEFAULT_INPUT_TRAY;
      else if( !strcmp(s, "default-medium") )             flags |= XPUATTRIBUTESUPPORTED_DEFAULT_MEDIUM;
      else if( !strcmp(s, "plex") )                       flags |= XPUATTRIBUTESUPPORTED_PLEX;
      else if( !strcmp(s, "xp-listfonts-modes") )         flags |= XPUATTRIBUTESUPPORTED_LISTFONTS_MODES;
    }
    
    XpuDisposeEnumerateXpAttributeValue(&tok_lasts);
    XFree(value);
  }  
  
  return(flags);
}

XpuSupportedFlags XpuGetSupportedJobAttributes(Display *pdpy, XPContext pcontext)
{
  return XpuGetSupportedAttributes(pdpy, pcontext, XPPrinterAttr, "job-attributes-supported");
}

XpuSupportedFlags XpuGetSupportedDocAttributes(Display *pdpy, XPContext pcontext)
{
  return XpuGetSupportedAttributes(pdpy, pcontext, XPPrinterAttr, "document-attributes-supported");
}

XpuSupportedFlags XpuGetSupportedPageAttributes(Display *pdpy, XPContext pcontext)
{
  return XpuGetSupportedAttributes(pdpy, pcontext, XPPrinterAttr, "xp-page-attributes-supported");
}





















char *XpuResourceEncode( const char *s )
{
  size_t  slen;
  char   *res;
  char   *d;
  int     i,
          c;

  slen = strlen(s);
  res  = malloc(slen*4+1);
  if (!res)
    return NULL;
  
  d = res;
  i = slen;
  while (i--) {
    c = *s++;
    if (c == '\n') {
      if (i) {
        *d++ = '\\';
        *d++ = 'n';
        *d++ = '\\';
        *d++ = '\n';
      }
      else {
        *d++ = '\\';
        *d++ = 'n';
      }
    } else if (c == '\\') {
        *d++ = '\\';
        *d++ = '\\';
    }
    else if ((c < ' ' && c != '\t') ||
            ((unsigned char)c >= 0x7F && (unsigned char)c < 0xA0)) {
        sprintf(d, "\\%03o", (unsigned char)c);
        d += 4;
    }
    else {
        *d++ = c;
    }
  }

  *d = '\0';
  
  return res;
}

#ifdef XXXJULIEN_NOTNOW
char *XpuResourceDecode( const char *str )
{
}
#endif 

void XpuResourceFreeString( char *s )
{
  free(s);
}

const char *XpuXmbToCompoundText(Display *dpy, const char *xmbtext)
{
  XTextProperty   xtp;
  int             xcr;
  char           *xtl[2];
  char           *ct;

  if (strlen(xmbtext) == 0)
    return strdup(xmbtext);
  
  memset(&xtp, 0, sizeof(xtp));
  xtl[0] = (char *)xmbtext;
  xtl[1] = NULL;
  
  xcr = XmbTextListToTextProperty(dpy, xtl, 1, XCompoundTextStyle, &xtp);
  
  if (xcr == XNoMemory || xcr == XLocaleNotSupported)
  {
    fprintf(stderr, "XpuXmbToCompoundText: XmbTextListToTextProperty failure.\n");
    return strdup(xmbtext);
  }

  

  if ( !((xcr == Success) || (xcr > 0)) ||
       (xtp.value == NULL))
  {
    fprintf(stderr, "XpuXmbToCompoundText: XmbTextListToTextProperty failure 2.\n");
    return strdup(xmbtext);
  }
  
  ct = malloc(xtp.nitems+1);
  if (!ct)
  {
    XFree(xtp.value);
    return NULL;
  }
  memcpy(ct, xtp.value, xtp.nitems);
  ct[xtp.nitems] = '\0';  

  XFree(xtp.value);
  
  return ct;
}

void XpuFreeCompundTextString( const char *s )
{
  free((void *)s);
}

const char *XpuCompoundTextToXmb(Display *dpy, const char *ct)
{
  XTextProperty   xtp;
  int             xcr;
  char          **xtl = NULL;
  int             xtl_count = 0;
  char           *xmb;
  int             xmb_len = 0;
  int             i;

  if (strlen(ct) == 0)
    return strdup(ct);
    
  xtp.value    = (unsigned char *)ct;
  xtp.nitems   = strlen(ct); 
  xtp.encoding = XInternAtom(dpy, "COMPOUND_TEXT", False);
  xtp.format   = 8;
  
  xcr = XmbTextPropertyToTextList(dpy, &xtp, &xtl, &xtl_count);
  
  if (xcr == XNoMemory || xcr == XLocaleNotSupported)
  {
    fprintf(stderr, "XpuCompoundTextToXmb: XmbTextPropertyToTextList failure 1.\n");
    return strdup(ct);
  }

  

  if ( !((xcr == Success) || (xcr > 0)) ||
       (xtl == NULL))
  {
    fprintf(stderr, "XpuCompoundTextToXmb: XmbTextPropertyToTextList failure 2.\n");
    return strdup(ct);
  }
   
  for (i = 0; i < xtl_count; i++)
  {
    xmb_len += strlen(xtl[i]);
  }
  xmb = malloc (xmb_len + 1);
  if (!xmb)
  {
    XFreeStringList(xtl);
    return NULL;
  }
  xmb[0] = '\0'; 
  for (i = 0; i < xtl_count; i++)
  {
    strcat(xmb, xtl[i]);
  }
  
  XFreeStringList(xtl); 
  
  return xmb;
}

void XpuFreeXmbString( const char *s )
{
  free((void *)s);
}


