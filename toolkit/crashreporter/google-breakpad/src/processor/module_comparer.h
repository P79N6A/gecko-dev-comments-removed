







































#ifndef PROCESSOR_MODULE_COMPARER_H__
#define PROCESSOR_MODULE_COMPARER_H__

#include <string>

#include "processor/basic_source_line_resolver_types.h"
#include "processor/fast_source_line_resolver_types.h"
#include "processor/module_serializer.h"
#include "processor/windows_frame_info.h"

namespace google_breakpad {

class ModuleComparer {
 public:
  ModuleComparer(): fast_resolver_(new FastSourceLineResolver),
                   basic_resolver_(new BasicSourceLineResolver) { }
  ~ModuleComparer() {
    delete fast_resolver_;
    delete basic_resolver_;
  }

  
  
  
  
  
  bool Compare(const string &symbol_data);

 private:
  typedef BasicSourceLineResolver::Module BasicModule;
  typedef FastSourceLineResolver::Module FastModule;
  typedef BasicSourceLineResolver::Function BasicFunc;
  typedef FastSourceLineResolver::Function FastFunc;
  typedef BasicSourceLineResolver::Line BasicLine;
  typedef FastSourceLineResolver::Line FastLine;
  typedef BasicSourceLineResolver::PublicSymbol BasicPubSymbol;
  typedef FastSourceLineResolver::PublicSymbol FastPubSymbol;
  typedef WindowsFrameInfo WFI;

  bool CompareModule(const BasicModule *oldmodule,
                     const FastModule *newmodule) const;
  bool CompareFunction(const BasicFunc *oldfunc, const FastFunc *newfunc) const;
  bool CompareLine(const BasicLine *oldline, const FastLine *newline) const;
  bool ComparePubSymbol(const BasicPubSymbol*, const FastPubSymbol*) const;
  bool CompareWFI(const WindowsFrameInfo&, const WindowsFrameInfo&) const;

  
  bool CompareCRM(const ContainedRangeMap<MemAddr, linked_ptr<WFI> >*,
                  const StaticContainedRangeMap<MemAddr, char>*) const;

  FastSourceLineResolver *fast_resolver_;
  BasicSourceLineResolver *basic_resolver_;
  ModuleSerializer serializer_;
};

}  

#endif  
