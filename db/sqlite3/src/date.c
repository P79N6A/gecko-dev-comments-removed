
















































#include "sqliteInt.h"
#include "os.h"
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#ifndef SQLITE_OMIT_DATETIME_FUNCS




typedef struct DateTime DateTime;
struct DateTime {
  double rJD;      
  int Y, M, D;     
  int h, m;        
  int tz;          
  double s;        
  char validYMD;   
  char validHMS;   
  char validJD;    
  char validTZ;    
};















static int getDigits(const char *zDate, ...){
  va_list ap;
  int val;
  int N;
  int min;
  int max;
  int nextC;
  int *pVal;
  int cnt = 0;
  va_start(ap, zDate);
  do{
    N = va_arg(ap, int);
    min = va_arg(ap, int);
    max = va_arg(ap, int);
    nextC = va_arg(ap, int);
    pVal = va_arg(ap, int*);
    val = 0;
    while( N-- ){
      if( !isdigit(*(u8*)zDate) ){
        goto end_getDigits;
      }
      val = val*10 + *zDate - '0';
      zDate++;
    }
    if( val<min || val>max || (nextC!=0 && nextC!=*zDate) ){
      goto end_getDigits;
    }
    *pVal = val;
    zDate++;
    cnt++;
  }while( nextC );
end_getDigits:
  va_end(ap);
  return cnt;
}





#define getValue sqlite3AtoF













static int parseTimezone(const char *zDate, DateTime *p){
  int sgn = 0;
  int nHr, nMn;
  while( isspace(*(u8*)zDate) ){ zDate++; }
  p->tz = 0;
  if( *zDate=='-' ){
    sgn = -1;
  }else if( *zDate=='+' ){
    sgn = +1;
  }else{
    return *zDate!=0;
  }
  zDate++;
  if( getDigits(zDate, 2, 0, 14, ':', &nHr, 2, 0, 59, 0, &nMn)!=2 ){
    return 1;
  }
  zDate += 5;
  p->tz = sgn*(nMn + nHr*60);
  while( isspace(*(u8*)zDate) ){ zDate++; }
  return *zDate!=0;
}








static int parseHhMmSs(const char *zDate, DateTime *p){
  int h, m, s;
  double ms = 0.0;
  if( getDigits(zDate, 2, 0, 24, ':', &h, 2, 0, 59, 0, &m)!=2 ){
    return 1;
  }
  zDate += 5;
  if( *zDate==':' ){
    zDate++;
    if( getDigits(zDate, 2, 0, 59, 0, &s)!=1 ){
      return 1;
    }
    zDate += 2;
    if( *zDate=='.' && isdigit((u8)zDate[1]) ){
      double rScale = 1.0;
      zDate++;
      while( isdigit(*(u8*)zDate) ){
        ms = ms*10.0 + *zDate - '0';
        rScale *= 10.0;
        zDate++;
      }
      ms /= rScale;
    }
  }else{
    s = 0;
  }
  p->validJD = 0;
  p->validHMS = 1;
  p->h = h;
  p->m = m;
  p->s = s + ms;
  if( parseTimezone(zDate, p) ) return 1;
  p->validTZ = p->tz!=0;
  return 0;
}







static void computeJD(DateTime *p){
  int Y, M, D, A, B, X1, X2;

  if( p->validJD ) return;
  if( p->validYMD ){
    Y = p->Y;
    M = p->M;
    D = p->D;
  }else{
    Y = 2000;  
    M = 1;
    D = 1;
  }
  if( M<=2 ){
    Y--;
    M += 12;
  }
  A = Y/100;
  B = 2 - A + (A/4);
  X1 = 365.25*(Y+4716);
  X2 = 30.6001*(M+1);
  p->rJD = X1 + X2 + D + B - 1524.5;
  p->validJD = 1;
  p->validYMD = 0;
  if( p->validHMS ){
    p->rJD += (p->h*3600.0 + p->m*60.0 + p->s)/86400.0;
    if( p->validTZ ){
      p->rJD -= p->tz*60/86400.0;
      p->validHMS = 0;
      p->validTZ = 0;
    }
  }
}













static int parseYyyyMmDd(const char *zDate, DateTime *p){
  int Y, M, D, neg;

  if( zDate[0]=='-' ){
    zDate++;
    neg = 1;
  }else{
    neg = 0;
  }
  if( getDigits(zDate,4,0,9999,'-',&Y,2,1,12,'-',&M,2,1,31,0,&D)!=3 ){
    return 1;
  }
  zDate += 10;
  while( isspace(*(u8*)zDate) || 'T'==*(u8*)zDate ){ zDate++; }
  if( parseHhMmSs(zDate, p)==0 ){
    
  }else if( *zDate==0 ){
    p->validHMS = 0;
  }else{
    return 1;
  }
  p->validJD = 0;
  p->validYMD = 1;
  p->Y = neg ? -Y : Y;
  p->M = M;
  p->D = D;
  if( p->validTZ ){
    computeJD(p);
  }
  return 0;
}

















static int parseDateOrTime(const char *zDate, DateTime *p){
  memset(p, 0, sizeof(*p));
  if( parseYyyyMmDd(zDate,p)==0 ){
    return 0;
  }else if( parseHhMmSs(zDate, p)==0 ){
    return 0;
  }else if( sqlite3StrICmp(zDate,"now")==0){
    double r;
    sqlite3OsCurrentTime(&r);
    p->rJD = r;
    p->validJD = 1;
    return 0;
  }else if( sqlite3IsNumber(zDate, 0, SQLITE_UTF8) ){
    getValue(zDate, &p->rJD);
    p->validJD = 1;
    return 0;
  }
  return 1;
}




static void computeYMD(DateTime *p){
  int Z, A, B, C, D, E, X1;
  if( p->validYMD ) return;
  if( !p->validJD ){
    p->Y = 2000;
    p->M = 1;
    p->D = 1;
  }else{
    Z = p->rJD + 0.5;
    A = (Z - 1867216.25)/36524.25;
    A = Z + 1 + A - (A/4);
    B = A + 1524;
    C = (B - 122.1)/365.25;
    D = 365.25*C;
    E = (B-D)/30.6001;
    X1 = 30.6001*E;
    p->D = B - D - X1;
    p->M = E<14 ? E-1 : E-13;
    p->Y = p->M>2 ? C - 4716 : C - 4715;
  }
  p->validYMD = 1;
}




static void computeHMS(DateTime *p){
  int Z, s;
  if( p->validHMS ) return;
  Z = p->rJD + 0.5;
  s = (p->rJD + 0.5 - Z)*86400000.0 + 0.5;
  p->s = 0.001*s;
  s = p->s;
  p->s -= s;
  p->h = s/3600;
  s -= p->h*3600;
  p->m = s/60;
  p->s += s - p->m*60;
  p->validHMS = 1;
}




static void computeYMD_HMS(DateTime *p){
  computeYMD(p);
  computeHMS(p);
}




static void clearYMD_HMS_TZ(DateTime *p){
  p->validYMD = 0;
  p->validHMS = 0;
  p->validTZ = 0;
}





static double localtimeOffset(DateTime *p){
  DateTime x, y;
  time_t t;
  struct tm *pTm;
  x = *p;
  computeYMD_HMS(&x);
  if( x.Y<1971 || x.Y>=2038 ){
    x.Y = 2000;
    x.M = 1;
    x.D = 1;
    x.h = 0;
    x.m = 0;
    x.s = 0.0;
  } else {
    int s = x.s + 0.5;
    x.s = s;
  }
  x.tz = 0;
  x.validJD = 0;
  computeJD(&x);
  t = (x.rJD-2440587.5)*86400.0 + 0.5;
  sqlite3OsEnterMutex();
  pTm = localtime(&t);
  y.Y = pTm->tm_year + 1900;
  y.M = pTm->tm_mon + 1;
  y.D = pTm->tm_mday;
  y.h = pTm->tm_hour;
  y.m = pTm->tm_min;
  y.s = pTm->tm_sec;
  sqlite3OsLeaveMutex();
  y.validYMD = 1;
  y.validHMS = 1;
  y.validJD = 0;
  y.validTZ = 0;
  computeJD(&y);
  return y.rJD - x.rJD;
}






















static int parseModifier(const char *zMod, DateTime *p){
  int rc = 1;
  int n;
  double r;
  char *z, zBuf[30];
  z = zBuf;
  for(n=0; n<sizeof(zBuf)-1 && zMod[n]; n++){
    z[n] = tolower(zMod[n]);
  }
  z[n] = 0;
  switch( z[0] ){
    case 'l': {
      




      if( strcmp(z, "localtime")==0 ){
        computeJD(p);
        p->rJD += localtimeOffset(p);
        clearYMD_HMS_TZ(p);
        rc = 0;
      }
      break;
    }
    case 'u': {
      





      if( strcmp(z, "unixepoch")==0 && p->validJD ){
        p->rJD = p->rJD/86400.0 + 2440587.5;
        clearYMD_HMS_TZ(p);
        rc = 0;
      }else if( strcmp(z, "utc")==0 ){
        double c1;
        computeJD(p);
        c1 = localtimeOffset(p);
        p->rJD -= c1;
        clearYMD_HMS_TZ(p);
        p->rJD += c1 - localtimeOffset(p);
        rc = 0;
      }
      break;
    }
    case 'w': {
      






      if( strncmp(z, "weekday ", 8)==0 && getValue(&z[8],&r)>0
                 && (n=r)==r && n>=0 && r<7 ){
        int Z;
        computeYMD_HMS(p);
        p->validTZ = 0;
        p->validJD = 0;
        computeJD(p);
        Z = p->rJD + 1.5;
        Z %= 7;
        if( Z>n ) Z -= 7;
        p->rJD += n - Z;
        clearYMD_HMS_TZ(p);
        rc = 0;
      }
      break;
    }
    case 's': {
      





      if( strncmp(z, "start of ", 9)!=0 ) break;
      z += 9;
      computeYMD(p);
      p->validHMS = 1;
      p->h = p->m = 0;
      p->s = 0.0;
      p->validTZ = 0;
      p->validJD = 0;
      if( strcmp(z,"month")==0 ){
        p->D = 1;
        rc = 0;
      }else if( strcmp(z,"year")==0 ){
        computeYMD(p);
        p->M = 1;
        p->D = 1;
        rc = 0;
      }else if( strcmp(z,"day")==0 ){
        rc = 0;
      }
      break;
    }
    case '+':
    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9': {
      n = getValue(z, &r);
      if( n<=0 ) break;
      if( z[n]==':' ){
        




        const char *z2 = z;
        DateTime tx;
        int day;
        if( !isdigit(*(u8*)z2) ) z2++;
        memset(&tx, 0, sizeof(tx));
        if( parseHhMmSs(z2, &tx) ) break;
        computeJD(&tx);
        tx.rJD -= 0.5;
        day = (int)tx.rJD;
        tx.rJD -= day;
        if( z[0]=='-' ) tx.rJD = -tx.rJD;
        computeJD(p);
        clearYMD_HMS_TZ(p);
       p->rJD += tx.rJD;
        rc = 0;
        break;
      }
      z += n;
      while( isspace(*(u8*)z) ) z++;
      n = strlen(z);
      if( n>10 || n<3 ) break;
      if( z[n-1]=='s' ){ z[n-1] = 0; n--; }
      computeJD(p);
      rc = 0;
      if( n==3 && strcmp(z,"day")==0 ){
        p->rJD += r;
      }else if( n==4 && strcmp(z,"hour")==0 ){
        p->rJD += r/24.0;
      }else if( n==6 && strcmp(z,"minute")==0 ){
        p->rJD += r/(24.0*60.0);
      }else if( n==6 && strcmp(z,"second")==0 ){
        p->rJD += r/(24.0*60.0*60.0);
      }else if( n==5 && strcmp(z,"month")==0 ){
        int x, y;
        computeYMD_HMS(p);
        p->M += r;
        x = p->M>0 ? (p->M-1)/12 : (p->M-12)/12;
        p->Y += x;
        p->M -= x*12;
        p->validJD = 0;
        computeJD(p);
        y = r;
        if( y!=r ){
          p->rJD += (r - y)*30.0;
        }
      }else if( n==4 && strcmp(z,"year")==0 ){
        computeYMD_HMS(p);
        p->Y += r;
        p->validJD = 0;
        computeJD(p);
      }else{
        rc = 1;
      }
      clearYMD_HMS_TZ(p);
      break;
    }
    default: {
      break;
    }
  }
  return rc;
}







static int isDate(int argc, sqlite3_value **argv, DateTime *p){
  int i;
  if( argc==0 ) return 1;
  if( SQLITE_NULL==sqlite3_value_type(argv[0]) || 
      parseDateOrTime((char*)sqlite3_value_text(argv[0]), p) ) return 1;
  for(i=1; i<argc; i++){
    if( SQLITE_NULL==sqlite3_value_type(argv[i]) || 
        parseModifier((char*)sqlite3_value_text(argv[i]), p) ) return 1;
  }
  return 0;
}












static void juliandayFunc(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  DateTime x;
  if( isDate(argc, argv, &x)==0 ){
    computeJD(&x);
    sqlite3_result_double(context, x.rJD);
  }
}






static void datetimeFunc(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  DateTime x;
  if( isDate(argc, argv, &x)==0 ){
    char zBuf[100];
    computeYMD_HMS(&x);
    sprintf(zBuf, "%04d-%02d-%02d %02d:%02d:%02d",x.Y, x.M, x.D, x.h, x.m,
           (int)(x.s));
    sqlite3_result_text(context, zBuf, -1, SQLITE_TRANSIENT);
  }
}






static void timeFunc(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  DateTime x;
  if( isDate(argc, argv, &x)==0 ){
    char zBuf[100];
    computeHMS(&x);
    sprintf(zBuf, "%02d:%02d:%02d", x.h, x.m, (int)x.s);
    sqlite3_result_text(context, zBuf, -1, SQLITE_TRANSIENT);
  }
}






static void dateFunc(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  DateTime x;
  if( isDate(argc, argv, &x)==0 ){
    char zBuf[100];
    computeYMD(&x);
    sprintf(zBuf, "%04d-%02d-%02d", x.Y, x.M, x.D);
    sqlite3_result_text(context, zBuf, -1, SQLITE_TRANSIENT);
  }
}




















static void strftimeFunc(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  DateTime x;
  int n, i, j;
  char *z;
  const char *zFmt = (const char*)sqlite3_value_text(argv[0]);
  char zBuf[100];
  if( zFmt==0 || isDate(argc-1, argv+1, &x) ) return;
  for(i=0, n=1; zFmt[i]; i++, n++){
    if( zFmt[i]=='%' ){
      switch( zFmt[i+1] ){
        case 'd':
        case 'H':
        case 'm':
        case 'M':
        case 'S':
        case 'W':
          n++;
          
        case 'w':
        case '%':
          break;
        case 'f':
          n += 8;
          break;
        case 'j':
          n += 3;
          break;
        case 'Y':
          n += 8;
          break;
        case 's':
        case 'J':
          n += 50;
          break;
        default:
          return;  
      }
      i++;
    }
  }
  if( n<sizeof(zBuf) ){
    z = zBuf;
  }else{
    z = sqliteMalloc( n );
    if( z==0 ) return;
  }
  computeJD(&x);
  computeYMD_HMS(&x);
  for(i=j=0; zFmt[i]; i++){
    if( zFmt[i]!='%' ){
      z[j++] = zFmt[i];
    }else{
      i++;
      switch( zFmt[i] ){
        case 'd':  sprintf(&z[j],"%02d",x.D); j+=2; break;
        case 'f': {
          int s = x.s;
          int ms = (x.s - s)*1000.0;
          sprintf(&z[j],"%02d.%03d",s,ms);
          j += strlen(&z[j]);
          break;
        }
        case 'H':  sprintf(&z[j],"%02d",x.h); j+=2; break;
        case 'W': 
        case 'j': {
          int nDay;             
          DateTime y = x;
          y.validJD = 0;
          y.M = 1;
          y.D = 1;
          computeJD(&y);
          nDay = x.rJD - y.rJD;
          if( zFmt[i]=='W' ){
            int wd;   
            wd = ((int)(x.rJD+0.5)) % 7;
            sprintf(&z[j],"%02d",(nDay+7-wd)/7);
            j += 2;
          }else{
            sprintf(&z[j],"%03d",nDay+1);
            j += 3;
          }
          break;
        }
        case 'J':  sprintf(&z[j],"%.16g",x.rJD); j+=strlen(&z[j]); break;
        case 'm':  sprintf(&z[j],"%02d",x.M); j+=2; break;
        case 'M':  sprintf(&z[j],"%02d",x.m); j+=2; break;
        case 's': {
          sprintf(&z[j],"%d",(int)((x.rJD-2440587.5)*86400.0 + 0.5));
          j += strlen(&z[j]);
          break;
        }
        case 'S':  sprintf(&z[j],"%02d",(int)(x.s+0.5)); j+=2; break;
        case 'w':  z[j++] = (((int)(x.rJD+1.5)) % 7) + '0'; break;
        case 'Y':  sprintf(&z[j],"%04d",x.Y); j+=strlen(&z[j]); break;
        case '%':  z[j++] = '%'; break;
      }
    }
  }
  z[j] = 0;
  sqlite3_result_text(context, z, -1, SQLITE_TRANSIENT);
  if( z!=zBuf ){
    sqliteFree(z);
  }
}






static void ctimeFunc(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  sqlite3_value *pVal = sqlite3ValueNew();
  if( pVal ){
    sqlite3ValueSetStr(pVal, -1, "now", SQLITE_UTF8, SQLITE_STATIC);
    timeFunc(context, 1, &pVal);
    sqlite3ValueFree(pVal);
  }
}






static void cdateFunc(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  sqlite3_value *pVal = sqlite3ValueNew();
  if( pVal ){
    sqlite3ValueSetStr(pVal, -1, "now", SQLITE_UTF8, SQLITE_STATIC);
    dateFunc(context, 1, &pVal);
    sqlite3ValueFree(pVal);
  }
}






static void ctimestampFunc(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  sqlite3_value *pVal = sqlite3ValueNew();
  if( pVal ){
    sqlite3ValueSetStr(pVal, -1, "now", SQLITE_UTF8, SQLITE_STATIC);
    datetimeFunc(context, 1, &pVal);
    sqlite3ValueFree(pVal);
  }
}
#endif 

#ifdef SQLITE_OMIT_DATETIME_FUNCS











static void currentTimeFunc(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  time_t t;
  char *zFormat = (char *)sqlite3_user_data(context);
  char zBuf[20];

  time(&t);
#ifdef SQLITE_TEST
  {
    extern int sqlite3_current_time;  
    if( sqlite3_current_time ){
      t = sqlite3_current_time;
    }
  }
#endif

  sqlite3OsEnterMutex();
  strftime(zBuf, 20, zFormat, gmtime(&t));
  sqlite3OsLeaveMutex();

  sqlite3_result_text(context, zBuf, -1, SQLITE_TRANSIENT);
}
#endif






void sqlite3RegisterDateTimeFunctions(sqlite3 *db){
#ifndef SQLITE_OMIT_DATETIME_FUNCS
  static const struct {
     char *zName;
     int nArg;
     void (*xFunc)(sqlite3_context*,int,sqlite3_value**);
  } aFuncs[] = {
    { "julianday", -1, juliandayFunc   },
    { "date",      -1, dateFunc        },
    { "time",      -1, timeFunc        },
    { "datetime",  -1, datetimeFunc    },
    { "strftime",  -1, strftimeFunc    },
    { "current_time",       0, ctimeFunc      },
    { "current_timestamp",  0, ctimestampFunc },
    { "current_date",       0, cdateFunc      },
  };
  int i;

  for(i=0; i<sizeof(aFuncs)/sizeof(aFuncs[0]); i++){
    sqlite3CreateFunc(db, aFuncs[i].zName, aFuncs[i].nArg,
        SQLITE_UTF8, 0, aFuncs[i].xFunc, 0, 0);
  }
#else
  static const struct {
     char *zName;
     char *zFormat;
  } aFuncs[] = {
    { "current_time", "%H:%M:%S" },
    { "current_date", "%Y-%m-%d" },
    { "current_timestamp", "%Y-%m-%d %H:%M:%S" }
  };
  int i;

  for(i=0; i<sizeof(aFuncs)/sizeof(aFuncs[0]); i++){
    sqlite3CreateFunc(db, aFuncs[i].zName, 0, SQLITE_UTF8, 
        aFuncs[i].zFormat, currentTimeFunc, 0, 0);
  }
#endif
}
