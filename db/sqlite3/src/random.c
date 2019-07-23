


















#include "sqliteInt.h"
#include "os.h"


















static int randomByte(){
  unsigned char t;

  


  static struct {
    unsigned char isInit;          
    unsigned char i, j;            
    unsigned char s[256];          
  } prng;

  








  if( !prng.isInit ){
    int i;
    char k[256];
    prng.j = 0;
    prng.i = 0;
    sqlite3OsRandomSeed(k);
    for(i=0; i<256; i++){
      prng.s[i] = i;
    }
    for(i=0; i<256; i++){
      prng.j += prng.s[i] + k[i];
      t = prng.s[prng.j];
      prng.s[prng.j] = prng.s[i];
      prng.s[i] = t;
    }
    prng.isInit = 1;
  }

  

  prng.i++;
  t = prng.s[prng.i];
  prng.j += t;
  prng.s[prng.i] = prng.s[prng.j];
  prng.s[prng.j] = t;
  t += prng.s[prng.i];
  return prng.s[t];
}




void sqlite3Randomness(int N, void *pBuf){
  unsigned char *zBuf = pBuf;
  sqlite3OsEnterMutex();
  while( N-- ){
    *(zBuf++) = randomByte();
  }
  sqlite3OsLeaveMutex();
}
