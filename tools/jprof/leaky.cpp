


































#include "leaky.h"
#include "intcnt.h"

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#ifndef NTO
#include <getopt.h>
#endif
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef NTO
#include <mem.h>
#endif

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

static const u_int DefaultBuckets = 10007;	
static const u_int MaxBuckets = 1000003;	



int main(int argc, char** argv)
{
  leaky* l = new leaky;

  l->initialize(argc, argv);
  l->open();
  return 0;
}

char *
htmlify(const char *in)
{
  const char *p = in;
  char *out, *q;
  int n = 0;
  size_t newlen;

  
  while ((p = strpbrk(p, "<>")))
  {
    ++n;
    ++p;
  }

  
  
  newlen = strlen(in) + n * 3 + 1;
  out = new char[newlen];

  
  p = in;
  q = out;
  do
  {
    if (*p == '<') 
    {
      strcpy(q, "&lt;");
      q += 4;
    }
    else if (*p == '>')
    {
      strcpy(q, "&gt;");
      q += 4;
    }
    else
    {
      *q++ = *p;
    }
    p++;
  } while (*p);
  *q = '\0';

  return out;
}

leaky::leaky()
{
  applicationName = NULL;
  logFile = NULL;
  progFile = NULL;

  quiet = TRUE;
  showAddress = FALSE;
  stackDepth = 100000;

  mappedLogFile = -1;
  firstLogEntry = lastLogEntry = 0;

  sfd = -1;
  externalSymbols = 0;
  usefulSymbols = 0;
  numExternalSymbols = 0;
  lowestSymbolAddr = 0;
  highestSymbolAddr = 0;

  loadMap = NULL;
}

leaky::~leaky()
{
}

void leaky::usageError()
{
  fprintf(stderr, "Usage: %s prog log\n", (char*) applicationName);
  exit(-1);
}

void leaky::initialize(int argc, char** argv)
{
  applicationName = argv[0];
  applicationName = strrchr(applicationName, '/');
  if (!applicationName) {
    applicationName = argv[0];
  } else {
    applicationName++;
  }

  int arg;
  int errflg = 0;
  while ((arg = getopt(argc, argv, "adEe:gh:i:r:Rs:tqx")) != -1) {
    switch (arg) {
      case '?':
	errflg++;
	break;
      case 'a':
	break;
      case 'A':
	showAddress = TRUE;
	break;
      case 'd':
	break;
      case 'R':
	break;
      case 'e':
	exclusions.add(optarg);
	break;
      case 'g':
	break;
      case 'r':
	roots.add(optarg);
	if (!includes.IsEmpty()) {
	  errflg++;
	}
	break;
      case 'i':
	includes.add(optarg);
	if (!roots.IsEmpty()) {
	  errflg++;
	}
	break;
      case 'h':
	break;
      case 's':
	stackDepth = atoi(optarg);
	if (stackDepth < 2) {
	  stackDepth = 2;
	}
	break;
      case 'x':
	break;
      case 'q':
	quiet = TRUE;
	break;
    }
  }
  if (errflg || ((argc - optind) < 2)) {
    usageError();
  }
  progFile = argv[optind++];
  logFile = argv[optind];
}

static void* mapFile(int fd, u_int flags, off_t* sz)
{
  struct stat sb;
  if (fstat(fd, &sb) < 0) {
    perror("fstat");
    exit(-1);
  }
  void* base = mmap(0, (int)sb.st_size, flags, MAP_PRIVATE, fd, 0);
  if (!base) {
    perror("mmap");
    exit(-1);
  }
  *sz = sb.st_size;
  return base;
}

void leaky::LoadMap()
{
  malloc_map_entry mme;
  char name[1000];

  int fd = ::open(M_MAPFILE, O_RDONLY);
  if (fd < 0) {
    perror("open: " M_MAPFILE);
    exit(-1);
  }
  for (;;) {
    int nb = read(fd, &mme, sizeof(mme));
    if (nb != sizeof(mme)) break;
    nb = read(fd, name, mme.nameLen);
    if (nb != (int)mme.nameLen) break;
    name[mme.nameLen] = 0;
    if (!quiet) {
      printf("%s @ %lx\n", name, mme.address);
    }

    LoadMapEntry* lme = new LoadMapEntry;
    lme->address = mme.address;
    lme->name = strdup(name);
    lme->next = loadMap;
    loadMap = lme;
  }
  close(fd);
}

void leaky::open()
{
  LoadMap();

  setupSymbols(progFile);

  
  mappedLogFile = ::open(logFile, O_RDONLY);
  if (mappedLogFile < 0) {
    perror("open");
    exit(-1);
  }
  off_t size;
  firstLogEntry = (malloc_log_entry*) mapFile(mappedLogFile, PROT_READ, &size);
  lastLogEntry = (malloc_log_entry*)((char*)firstLogEntry + size);

  analyze();

  exit(0);
}




static int symbolOrder(void const* a, void const* b)
{
  Symbol const* ap = (Symbol const *)a;
  Symbol const* bp = (Symbol const *)b;
  ptrdiff_t diff = ap->address - bp->address;
  return (diff == 0) ? 0 : ((diff > 0) ? 1 : -1);
}

void leaky::ReadSharedLibrarySymbols()
{
  LoadMapEntry* lme = loadMap;
  while (NULL != lme) {
    ReadSymbols(lme->name, lme->address);
    lme = lme->next;
  }
}

void leaky::setupSymbols(const char *fileName)
{
  
  ReadSymbols(fileName, 0);

  
  ReadSharedLibrarySymbols();

  if (!quiet) {
    printf("A total of %d symbols were loaded\n", usefulSymbols);
  }

  
  qsort(externalSymbols, usefulSymbols, sizeof(Symbol), symbolOrder);
  lowestSymbolAddr = externalSymbols[0].address;
  highestSymbolAddr = externalSymbols[usefulSymbols-1].address;
}



int leaky::findSymbolIndex(u_long addr)
{
  u_int base = 0;
  u_int limit = usefulSymbols - 1;
  Symbol* end = &externalSymbols[limit];
  while (base <= limit) {
    u_int midPoint = (base + limit)>>1;
    Symbol* sp = &externalSymbols[midPoint];
    if (addr < sp->address) {
      if (midPoint == 0) {
	return -1;
      }
      limit = midPoint - 1;
    } else {
      if (sp+1 < end) {
	if (addr < (sp+1)->address) {
	  return midPoint;
	}
      } else {
	return midPoint;
      }
      base = midPoint + 1;
    }
  }
  return -1;
}

Symbol* leaky::findSymbol(u_long addr)
{
  int idx = findSymbolIndex(addr);

  if(idx<0) {
    return NULL;
  } else {
    return &externalSymbols[idx];
  }
}



bool leaky::excluded(malloc_log_entry* lep)
{
  if (exclusions.IsEmpty()) {
    return false;
  }

  char** pcp = &lep->pcs[0];
  u_int n = lep->numpcs;
  for (u_int i = 0; i < n; i++, pcp++) {
    Symbol* sp = findSymbol((u_long) *pcp);
    if (sp && exclusions.contains(sp->name)) {
      return true;
    }
  }
  return false;
}

bool leaky::included(malloc_log_entry* lep)
{
  if (includes.IsEmpty()) {
    return true;
  }

  char** pcp = &lep->pcs[0];
  u_int n = lep->numpcs;
  for (u_int i = 0; i < n; i++, pcp++) {
    Symbol* sp = findSymbol((u_long) *pcp);
    if (sp && includes.contains(sp->name)) {
      return true;
    }
  }
  return false;
}



void leaky::displayStackTrace(FILE* out, malloc_log_entry* lep)
{
  char** pcp = &lep->pcs[0];
  u_int n = (lep->numpcs < stackDepth) ? lep->numpcs : stackDepth;
  for (u_int i = 0; i < n; i++, pcp++) {
    u_long addr = (u_long) *pcp;
    Symbol* sp = findSymbol(addr);
    if (sp) {
      fputs(sp->name, out);
      if (showAddress) {
	fprintf(out, "[%p]", (char*)addr);
      }
    }
    else {
      fprintf(out, "<%p>", (char*)addr);
    }
    fputc(' ', out);
  }
  fputc('\n', out);
}

void leaky::dumpEntryToLog(malloc_log_entry* lep)
{
  printf("%ld\t", lep->delTime);
  printf(" --> ");
  displayStackTrace(stdout, lep);
}

void leaky::generateReportHTML(FILE *fp, int *countArray, int count)
{
  fprintf(fp,"<html><head><title>Jprof Profile Report</title></head><body>\n");
  fprintf(fp,"<h1><center>Jprof Profile Report</center></h1>\n");
  fprintf(fp,"<center>");
  fprintf(fp,"<A href=#flat>flat</A><b> | </b><A href=#hier>hierarchical</A>");
  fprintf(fp,"</center><P><P><P>\n");

  int *rankingTable = new int[usefulSymbols];

  for(int cnt=usefulSymbols; --cnt>=0; rankingTable[cnt]=cnt);

  
  
  
  

  
  int mx, i, h;
  for(mx=usefulSymbols/9, h=581130733; h>0; h/=3) {
    if(h<mx) {
      for(i = h-1; i<usefulSymbols; i++) {
        int j, tmp=rankingTable[i], val = countArray[tmp];
	for(j = i; (j>=h) && (countArray[rankingTable[j-h]]<val); j-=h) {
	  rankingTable[j] = rankingTable[j-h];
	}
	rankingTable[j] = tmp;
      }
    }
  }

  
  
  
  
  fprintf(fp,
  "<h2><A NAME=hier></A><center><a href=\"http://lxr.mozilla.org/mozilla/source/tools/jprof/README.html#hier\">Hierarchical Profile</a></center></h2><hr>\n");
  fprintf(fp, "<pre>\n");
  fprintf(fp, "%5s %5s    %4s %s\n",
  "index", "Count", "Hits", "Function Name");

  for(i=0; i<usefulSymbols && countArray[rankingTable[i]]>0; i++) {
    Symbol *sp=&externalSymbols[rankingTable[i]];
    
    sp->cntP.printReport(fp, this);

    char *symname = htmlify(sp->name);
    fprintf(fp, "%6d %3d <a name=%d>%8d</a> <b>%s</b>\n", rankingTable[i],
            sp->timerHit, rankingTable[i], countArray[rankingTable[i]],
            symname);
    delete [] symname;

    sp->cntC.printReport(fp, this);

    fprintf(fp, "<hr>\n");
  }
  fprintf(fp,"</pre>\n");

  
  

  
  
  for(mx=usefulSymbols/9, h=581130733; h>0; h/=3) {
    if(h<mx) {
      for(i = h-1; i<usefulSymbols; i++) {
	int j, tmp=rankingTable[i], val = externalSymbols[tmp].timerHit;
	for(j = i;
	  (j>=h) && (externalSymbols[rankingTable[j-h]].timerHit<val); j-=h) {
	  rankingTable[j] = rankingTable[j-h];
	}
	rankingTable[j] = tmp;
      }
    }
  }

  
  
  
  
  int totalTimerHits = 0;
  for(i=0;
    i<usefulSymbols && externalSymbols[rankingTable[i]].timerHit>0; i++) {
    Symbol *sp=&externalSymbols[rankingTable[i]];
    totalTimerHits += sp->timerHit;
  }

  fprintf(fp,"<h2><A NAME=flat></A><center><a href=\"http://lxr.mozilla.org/mozilla/source/tools/jprof/README.html#flat\">Flat Profile</a></center></h2><br>\n");
  fprintf(fp, "<pre>\n");

  fprintf(fp, "Total hit count: %d\n", totalTimerHits);
  fprintf(fp, "Count %%Total  Function Name\n");
  
  for(i=0;
    i<usefulSymbols && externalSymbols[rankingTable[i]].timerHit>0; i++) {

    Symbol *sp=&externalSymbols[rankingTable[i]];
    
    char *symname = htmlify(sp->name);
    fprintf(fp, "<a href=\"#%d\">%3d   %-2.1f     %s</a>\n",
            rankingTable[i], sp->timerHit,
            ((float)sp->timerHit/(float)totalTimerHits)*100.0, symname);
    delete [] symname;
  }

  fprintf(fp,"</pre></body></html>\n");
}

void leaky::analyze()
{
  int *countArray = new int[usefulSymbols];
  int *flagArray  = new int[usefulSymbols];

  
  memset(countArray, 0, sizeof(countArray[0])*usefulSymbols);

  
  
  
  
  
  
  memset(flagArray, -1, sizeof(flagArray[0])*usefulSymbols);

  
  stacks = 0;
  for(malloc_log_entry* lep=firstLogEntry; 
    lep < lastLogEntry;
    lep = reinterpret_cast<malloc_log_entry*>(&lep->pcs[lep->numpcs])) {

    if (excluded(lep) || !included(lep))
      continue;

    ++stacks; 

    
    
    u_int n = (lep->numpcs < stackDepth) ? lep->numpcs : stackDepth;
    char** pcp = &lep->pcs[n-1];
    int idx=-1, parrentIdx=-1;  
    for(int i=n-1; i>=0; --i, --pcp, parrentIdx=idx) {
      idx = findSymbolIndex(reinterpret_cast<u_long>(*pcp));

      if(idx>=0) {
	
	
	if (i > 0 && !strcmp(externalSymbols[idx].name, "__restore_rt")) {
	  --pcp;
	  --i;
	  idx = findSymbolIndex(reinterpret_cast<u_long>(*pcp));
	  if (idx < 0) {
	    continue;
	  }
	}
	
	
	if(flagArray[idx]!=stacks && ((flagArray[idx]=stacks) || true)) {
	  ++countArray[idx];
	}

	
	if(parrentIdx>=0) {
	  externalSymbols[parrentIdx].regChild(idx);
	  externalSymbols[idx].regParrent(parrentIdx);
	}
      }
    }

    
    if(idx>=0) {
      ++externalSymbols[idx].timerHit;
    }
  }

  generateReportHTML(stdout, countArray, stacks);
}

void FunctionCount::printReport(FILE *fp, leaky *lk)
{
    const char *fmt = "             <A href=\"#%d\">%6d %s</A>\n";

    int nmax, tmax=((~0U)>>1);
    
    do {
	nmax=0;
	for(int j=getSize(); --j>=0;) {
	    int cnt = getCount(j);
	    if(cnt==tmax) {
		int idx = getIndex(j);
		char *symname = htmlify(lk->indexToName(idx));
		fprintf(fp, fmt, idx, getCount(j), symname);
		delete [] symname;
	    } else if(cnt<tmax && cnt>nmax) {
	        nmax=cnt;
	    }
	}
    } while((tmax=nmax)>0);
}
