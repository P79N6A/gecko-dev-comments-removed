















#ifndef _SQLITE_HASH_H_
#define _SQLITE_HASH_H_


typedef struct Hash Hash;
typedef struct HashElem HashElem;









struct Hash {
  char keyClass;          
  char copyKey;           
  int count;              
  HashElem *first;        
  void *(*xMalloc)(int);  
  void (*xFree)(void *);  
  int htsize;             
  struct _ht {            
    int count;               
    HashElem *chain;         
  } *ht;
};







struct HashElem {
  HashElem *next, *prev;   
  void *data;              
  void *pKey; int nKey;    
};




















#define SQLITE_HASH_STRING    3
#define SQLITE_HASH_BINARY    4




void sqlite3HashInit(Hash*, int keytype, int copyKey);
void *sqlite3HashInsert(Hash*, const void *pKey, int nKey, void *pData);
void *sqlite3HashFind(const Hash*, const void *pKey, int nKey);
void sqlite3HashClear(Hash*);













#define sqliteHashFirst(H)  ((H)->first)
#define sqliteHashNext(E)   ((E)->next)
#define sqliteHashData(E)   ((E)->data)
#define sqliteHashKey(E)    ((E)->pKey)
#define sqliteHashKeysize(E) ((E)->nKey)




#define sqliteHashCount(H)  ((H)->count)

#endif 
