




































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
  l->outputfd = stdout;

  for (int i = 0; i < l->numLogFiles; i++) {
    if (l->output_dir || l->numLogFiles > 1) {
      char name[2048]; 
      if (l->output_dir)
        snprintf(name,sizeof(name),"%s/%s.html",l->output_dir,argv[l->logFileIndex + i]);
      else
        snprintf(name,sizeof(name),"%s.html",argv[l->logFileIndex + i]);

      fprintf(stderr,"opening %s\n",name);
      l->outputfd = fopen(name,"w");
      
    }
    if (l->outputfd) { 
      l->open(argv[l->logFileIndex + i]);

      if (l->outputfd != stderr) {
        fclose(l->outputfd);
        l->outputfd = NULL;
      }
    }
  }

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
  progFile = NULL;

  quiet = true;
  showAddress = false;
  showThreads = false;
  stackDepth = 100000;
  onlyThread = 0;

  mappedLogFile = -1;
  firstLogEntry = lastLogEntry = 0;

  sfd = -1;
  externalSymbols = 0;
  usefulSymbols = 0;
  numExternalSymbols = 0;
  lowestSymbolAddr = 0;
  highestSymbolAddr = 0;

  loadMap = NULL;

  collect_last  = false;
  collect_start = -1;
  collect_end   = -1;
}

leaky::~leaky()
{
}

void leaky::usageError()
{
  fprintf(stderr, "Usage: %s [-v] [-t] [-e exclude] [-i include] [-s stackdepth] [--last] [--all] [--start n [--end m]] [--output-dir dir] prog log [log2 ...]\n", (char*) applicationName);
  fprintf(stderr, 
          "\t-v: verbose\n"
          "\t-t | --threads: split threads\n"
          "\t--only-thread n: only profile thread N\n"
          "\t-i include-id: stack must include specified id\n"
          "\t-e exclude-id: stack must NOT include specified id\n"
          "\t-s stackdepth: Limit depth looked at from captured stack frames\n"
          "\t--last: only profile the last capture section\n"
          "\t--start n [--end m]: profile n to m (or end) capture sections\n"
          "\t--output-dir dir: write output files to dir\n"
          "\tIf there's one log, output goes to stdout unless --output-dir is set\n"
          "\tIf there are more than one log, output files will be named with .html added\n"
          );
  exit(-1);
}

static struct option longopts[] = {
    { "threads", 0, NULL, 't' },
    { "only-thread", 1, NULL, 'T' },
    { "last", 0, NULL, 'l' },
    { "start", 1, NULL, 'x' },
    { "end", 1, NULL, 'n' },
    { "output-dir", 1, NULL, 'd' },
    { NULL, 0, NULL, 0 },
};

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
  int longindex = 0;

  onlyThread = 0;
  output_dir = NULL;

  
  
  while (((arg = getopt_long(argc, argv, "adEe:gh:i:r:Rs:tT:qvx:ln:",longopts,&longindex)) != -1)) {
    switch (arg) {
      case '?':
      default:
        fprintf(stderr,"error: unknown option %c\n",optopt);
	errflg++;
	break;
      case 'a':
	break;
      case 'A': 
	showAddress = true;
	break;
      case 'd':
        output_dir = optarg; 
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
        
        collect_start = atoi(optarg);
	break;
      case 'n':
        
        collect_end = atoi(optarg);
        break;
      case 'l':
        
        collect_last = true;
        break;
      case 'q':
        break;
      case 'v':
        quiet = !quiet;
        break;
      case 't':
        showThreads = true;
	break;
      case 'T':
        showThreads = true;
        onlyThread = atoi(optarg);
	break;
    }
  }
  if (errflg || ((argc - optind) < 2)) {
    usageError();
  }
  progFile = argv[optind++];
  logFileIndex = optind;
  numLogFiles  = argc - optind;
  if (!quiet)
    fprintf(stderr,"numlogfiles = %d\n",numLogFiles);
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

  if (!loadMap) {
    
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
        fprintf(stderr,"%s @ %lx\n", name, mme.address);
      }

      LoadMapEntry* lme = new LoadMapEntry;
      lme->address = mme.address;
      lme->name = strdup(name);
      lme->next = loadMap;
      loadMap = lme;
    }
    close(fd);
  }
}

void leaky::open(char *logFile)
{
  int threadArray[100]; 
  int last_thread = -1;
  int numThreads = 0;
  int section = -1;
  bool collecting = false;

  LoadMap();

  setupSymbols(progFile);

  
  if (mappedLogFile)
    ::close(mappedLogFile);

  mappedLogFile = ::open(logFile, O_RDONLY);
  if (mappedLogFile < 0) {
    perror("open");
    exit(-1);
  }
  off_t size;
  firstLogEntry = (malloc_log_entry*) mapFile(mappedLogFile, PROT_READ, &size);
  lastLogEntry = (malloc_log_entry*)((char*)firstLogEntry + size);

  if (!collect_last || collect_start < 0) {
    collecting = true;
  }

  
  
  for (malloc_log_entry* lep=firstLogEntry;
       lep < lastLogEntry;
       lep = reinterpret_cast<malloc_log_entry*>(&lep->pcs[lep->numpcs])) {

    if (lep->flags & JP_FIRST_AFTER_PAUSE) {
      section++;
      if (collect_last) {
        firstLogEntry = lep;
        numThreads = 0;
        collecting = true;
      }
      if (collect_start == section) {
        collecting = true;
        firstLogEntry = lep;
      }
      if (collect_end == section) {
        collecting = false;
        lastLogEntry = lep;
      }
      if (!quiet)
        fprintf(stderr,"New section %d: first=%p, last=%p, collecting=%d\n",
                section,(void*)firstLogEntry,(void*)lastLogEntry,collecting);
    }

    

    

    
    
    
    
    
    if (showThreads && collecting) {
      if (lep->thread != last_thread)
      {
        int i;
        for (i=0; i<numThreads; i++)
        {
          if (lep->thread == threadArray[i])
            break;
        }
        if (i == numThreads &&
            i < (int) (sizeof(threadArray)/sizeof(threadArray[0])))
        {
          threadArray[i] = lep->thread;
          numThreads++;
          if (!quiet)
            fprintf(stderr,"new thread %d\n",lep->thread);
        }
      }
    }
  }  
  if (!quiet)
    fprintf(stderr,"Done collecting: sections %d: first=%p, last=%p, numThreads=%d\n",
            section,(void*)firstLogEntry,(void*)lastLogEntry,numThreads);

  fprintf(outputfd,"<html><head><title>Jprof Profile Report</title></head><body>\n");
  fprintf(outputfd,"<h1><center>Jprof Profile Report</center></h1>\n");

  if (showThreads)
  {
    fprintf(stderr,"Num threads %d\n",numThreads);

    fprintf(outputfd,"<hr>Threads:<p><pre>\n");
    for (int i=0; i<numThreads; i++)
    {
      fprintf(outputfd,"   <a href=\"#thread_%d\">%d</a>  ",
              threadArray[i],threadArray[i]);
    }
    fprintf(outputfd,"</pre>");

    for (int i=0; i<numThreads; i++)
    {
      if (!onlyThread || onlyThread == threadArray[i])
        analyze(threadArray[i]);
    }
  }
  else
  {
    analyze(0);
  }

  fprintf(outputfd,"</pre></body></html>\n");
}




static int symbolOrder(void const* a, void const* b)
{
  Symbol const* ap = (Symbol const *)a;
  Symbol const* bp = (Symbol const *)b;
  return ap->address == bp->address ? 0 :
    (ap->address > bp->address ? 1 : -1);
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
  if (usefulSymbols == 0) {
    

    
    ReadSymbols(fileName, 0);

    
    ReadSharedLibrarySymbols();

    if (!quiet) {
      fprintf(stderr,"A total of %d symbols were loaded\n", usefulSymbols);
    }

    
    qsort(externalSymbols, usefulSymbols, sizeof(Symbol), symbolOrder);
    lowestSymbolAddr = externalSymbols[0].address;
    highestSymbolAddr = externalSymbols[usefulSymbols-1].address;
  }
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
  displayStackTrace(outputfd, lep);
}

void leaky::generateReportHTML(FILE *fp, int *countArray, int count, int thread)
{
  fprintf(fp,"<center>");
  if (showThreads)
  {
    fprintf(fp,"<hr><A NAME=thread_%d><b>Thread: %d</b></A><p>",
            thread,thread);
  }
  fprintf(fp,"<A href=#flat_%d>flat</A><b> | </b><A href=#hier_%d>hierarchical</A>",
          thread,thread);
  fprintf(fp,"</center><P><P><P>\n");

  int totalTimerHits = count;
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
          "<h2><A NAME=hier_%d></A><center><a href=\"http://mxr.mozilla.org/mozilla-central/source/tools/jprof/README.html#hier\">Hierarchical Profile</a></center></h2><hr>\n",
          thread);
  fprintf(fp, "<pre>\n");
  fprintf(fp, "%6s %6s         %4s      %s\n",
          "index", "Count", "Hits", "Function Name");

  for(i=0; i<usefulSymbols && countArray[rankingTable[i]]>0; i++) {
    Symbol *sp=&externalSymbols[rankingTable[i]];
    
    sp->cntP.printReport(fp, this, rankingTable[i], totalTimerHits);

    char *symname = htmlify(sp->name);
    fprintf(fp, "%6d %6d (%3.1f%%)%s <a name=%d>%8d (%3.1f%%)</a>%s <b>%s</b>\n", 
            rankingTable[i],
            sp->timerHit, (sp->timerHit*1000/totalTimerHits)/10.0,
            (sp->timerHit*1000/totalTimerHits)/10.0 >= 10.0 ? "" : " ",
            rankingTable[i], countArray[rankingTable[i]],
            (countArray[rankingTable[i]]*1000/totalTimerHits)/10.0,
            (countArray[rankingTable[i]]*1000/totalTimerHits)/10.0 >= 10.0 ? "" : " ",
            symname);
    delete [] symname;

    sp->cntC.printReport(fp, this, rankingTable[i], totalTimerHits);

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

  
  
  
  
  totalTimerHits = 0;
  for(i=0;
    i<usefulSymbols && externalSymbols[rankingTable[i]].timerHit>0; i++) {
    Symbol *sp=&externalSymbols[rankingTable[i]];
    totalTimerHits += sp->timerHit;
  }
  if (totalTimerHits == 0)
    totalTimerHits = 1;

  if (totalTimerHits != count)
    fprintf(stderr,"Hit count mismatch: count=%d; totalTimerHits=%d",
            count,totalTimerHits);

  fprintf(fp,"<h2><A NAME=flat_%d></A><center><a href=\"http://mxr.mozilla.org/mozilla-central/source/tools/jprof/README.html#flat\">Flat Profile</a></center></h2><br>\n",
          thread);
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
}

void leaky::analyze(int thread)
{
  int *countArray = new int[usefulSymbols];
  int *flagArray  = new int[usefulSymbols];

  
  memset(countArray, 0, sizeof(countArray[0])*usefulSymbols);

  
  for(int i=0; i<usefulSymbols; i++) {
    externalSymbols[i].timerHit = 0;
    externalSymbols[i].regClear();
  }

  
  
  
  
  
  
  memset(flagArray, -1, sizeof(flagArray[0])*usefulSymbols);

  
  
  stacks = 0;
  for(malloc_log_entry* lep=firstLogEntry; 
    lep < lastLogEntry;
    lep = reinterpret_cast<malloc_log_entry*>(&lep->pcs[lep->numpcs])) {

    if ((thread != 0 && lep->thread != thread) ||
        excluded(lep) || !included(lep))
    {
      continue;
    }

    ++stacks; 

    
    
    u_int n = (lep->numpcs < stackDepth) ? lep->numpcs : stackDepth;
    char** pcp = &lep->pcs[n-1];
    int idx=-1, parrentIdx=-1;  
    for (int i=n-1; i>=0; --i, --pcp) {
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
        
        
        parrentIdx=idx;
      }
    }

    
    if(idx>=0) {
      ++externalSymbols[idx].timerHit;
    }
  }

  generateReportHTML(outputfd, countArray, stacks, thread);
}

void FunctionCount::printReport(FILE *fp, leaky *lk, int parent, int total)
{
    const char *fmt = "                      <A href=\"#%d\">%8d (%3.1f%%)%s %s</A>%s\n";

    int nmax, tmax=((~0U)>>1);
    
    do {
	nmax=0;
	for(int j=getSize(); --j>=0;) {
	    int cnt = getCount(j);
	    if(cnt==tmax) {
		int idx = getIndex(j);
		char *symname = htmlify(lk->indexToName(idx));
                fprintf(fp, fmt, idx, getCount(j),
                        getCount(j)*100.0/total,
                        getCount(j)*100.0/total >= 10.0 ? "" : " ",
                        symname,
                        parent == idx ? " (self)" : "");
		delete [] symname;
	    } else if(cnt<tmax && cnt>nmax) {
	        nmax=cnt;
	    }
	}
    } while((tmax=nmax)>0);
}
