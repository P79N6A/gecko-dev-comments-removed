



#include "BaseElf.h"
#include "Elfxx.h"
#include "Logging.h"

using namespace Elf;

unsigned long
BaseElf::Hash(const char *symbol)
{
  const unsigned char *sym = reinterpret_cast<const unsigned char *>(symbol);
  unsigned long h = 0, g;
  while (*sym) {
    h = (h << 4) + *sym++;
    g = h & 0xf0000000;
    h ^= g;
    h ^= g >> 24;
  }
  return h;
}

void *
BaseElf::GetSymbolPtr(const char *symbol) const
{
  return GetSymbolPtr(symbol, Hash(symbol));
}

void *
BaseElf::GetSymbolPtr(const char *symbol, unsigned long hash) const
{
  const Sym *sym = GetSymbol(symbol, hash);
  void *ptr = nullptr;
  if (sym && sym->st_shndx != SHN_UNDEF)
    ptr = GetPtr(sym->st_value);
  DEBUG_LOG("BaseElf::GetSymbolPtr(%p [\"%s\"], \"%s\") = %p",
            reinterpret_cast<const void *>(this), GetPath(), symbol, ptr);
  return ptr;
}

const Sym *
BaseElf::GetSymbol(const char *symbol, unsigned long hash) const
{
  







  size_t bucket = hash % buckets.numElements();
  for (size_t y = buckets[bucket]; y != STN_UNDEF; y = chains[y]) {
    if (strcmp(symbol, strtab.GetStringAt(symtab[y].st_name)))
      continue;
    return &symtab[y];
  }
  return nullptr;
}

bool
BaseElf::Contains(void *addr) const
{
  return base.Contains(addr);
}

#ifdef __ARM_EABI__
const void *
BaseElf::FindExidx(int *pcount) const
{
  if (arm_exidx) {
    *pcount = arm_exidx.numElements();
    return arm_exidx;
  }
  *pcount = 0;
  return nullptr;
}
#endif
