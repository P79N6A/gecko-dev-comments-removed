


































#ifndef __leaky_h_
#define __leaky_h_

#include "config.h"
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include "libmalloc.h"
#include "strset.h"
#include "intcnt.h"

typedef unsigned int u_int;

struct Symbol;
struct leaky;

class FunctionCount : public IntCount
{
public:
    void printReport(FILE *fp, leaky *lk);
};

struct Symbol {
  char* name;
  u_long address;
  int    timerHit;
  FunctionCount cntP, cntC;

  int regChild(int id) {return cntC.countAdd(id, 1);}
  int regParrent(int id) {return cntP.countAdd(id, 1);}

  Symbol() : timerHit(0) {}
  void Init(const char* aName, u_long aAddress) {
    name = aName ? strdup(aName) : (char *)"";
    address = aAddress;
  }
};

struct LoadMapEntry {
  char* name;			
  u_long address;		
  LoadMapEntry* next;
};

struct leaky {
  leaky();
  ~leaky();

  void initialize(int argc, char** argv);
  void open();

  char*  applicationName;
  char*  logFile;
  char*  progFile;

  int   quiet;
  int   showAddress;
  u_int  stackDepth;

  int   mappedLogFile;
  malloc_log_entry* firstLogEntry;
  malloc_log_entry* lastLogEntry;

  int    stacks;

  int sfd;
  Symbol* externalSymbols;
  int     usefulSymbols;
  int     numExternalSymbols;
  StrSet exclusions;
  u_long lowestSymbolAddr;
  u_long highestSymbolAddr;

  LoadMapEntry* loadMap;

  StrSet roots;
  StrSet includes;

  void usageError();

  void LoadMap();

  void analyze();

  void dumpEntryToLog(malloc_log_entry* lep);

  void insertAddress(u_long address, malloc_log_entry* lep);
  void removeAddress(u_long address, malloc_log_entry* lep);

  void displayStackTrace(FILE* out, malloc_log_entry* lep);

  void ReadSymbols(const char* fileName, u_long aBaseAddress);
  void ReadSharedLibrarySymbols();
  void setupSymbols(const char* fileName);
  Symbol* findSymbol(u_long address);
  bool excluded(malloc_log_entry* lep);
  bool included(malloc_log_entry* lep);
  const char* indexToName(int idx) {return externalSymbols[idx].name;}

  private:
  void generateReportHTML(FILE *fp, int *countArray, int count);
  int  findSymbolIndex(u_long address);
};

#endif 
