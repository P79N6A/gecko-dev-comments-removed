



































#include "leaky.h"

#ifdef USE_BFD
#include <stdio.h>
#include <string.h>
#include <bfd.h>

extern "C" {
  char *cplus_demangle (const char *mangled, int options);
}

void leaky::ReadSymbols(const char *aFileName, u_long aBaseAddress)
{
  static bfd_boolean kDynamic = (bfd_boolean) false;

  static int firstTime = 1;
  if (firstTime) {
    firstTime = 0;
    bfd_init ();
  }

  bfd* lib = bfd_openr(aFileName, NULL);
  if (NULL == lib) {
    return;
  }
  char **matching;
  if (!bfd_check_format_matches(lib, bfd_object, &matching)) {
    bfd_close(lib);
    return;
  }

  asymbol* store;
  store = bfd_make_empty_symbol(lib);

  
  PTR minisyms;
  unsigned int size;
  long symcount = bfd_read_minisymbols(lib, kDynamic, &minisyms, &size);

  int initialSymbols = usefulSymbols;
  if (NULL == externalSymbols) {
    externalSymbols = (Symbol*) malloc(sizeof(Symbol) * 10000);
    numExternalSymbols = 10000;
  }
  Symbol* sp = externalSymbols + usefulSymbols;
  Symbol* lastSymbol = externalSymbols + numExternalSymbols;

  
  bfd_byte* from = (bfd_byte *) minisyms;
  bfd_byte* fromend = from + symcount * size;
  for (; from < fromend; from += size) {
    asymbol *sym;
    sym = bfd_minisymbol_to_symbol(lib, kDynamic, (const PTR) from, store);

    symbol_info syminfo;
    bfd_get_symbol_info (lib, sym, &syminfo);


      const char* nm = bfd_asymbol_name(sym);
      if (nm && nm[0]) {
	char* dnm = NULL;
	if (strncmp("__thunk", nm, 7)) {
	  dnm = cplus_demangle(nm, 1);
	}
	sp->Init(dnm ? dnm : nm, syminfo.value + aBaseAddress);
	sp++;
	if (sp >= lastSymbol) {
	  long n = numExternalSymbols + 10000;
	  externalSymbols = (Symbol*)
	    realloc(externalSymbols, (size_t) (sizeof(Symbol) * n));
	  lastSymbol = externalSymbols + n;
	  sp = externalSymbols + numExternalSymbols;
	  numExternalSymbols = n;
	}
      }

  }


  bfd_close(lib);

  int interesting = sp - externalSymbols;
  if (!quiet) {
    printf("%s provided %d symbols\n", aFileName,
	   interesting - initialSymbols);
  }
  usefulSymbols = interesting;
}

#endif 
