


























































































































































#ifndef COMMON_DWARF_DWARF2DIEHANDLER_H__
#define COMMON_DWARF_DWARF2DIEHANDLER_H__

#include <stack>
#include <string>

#include "common/dwarf/types.h"
#include "common/dwarf/dwarf2enums.h"
#include "common/dwarf/dwarf2reader.h"
#include "common/using_std_string.h"

namespace dwarf2reader {













class DIEHandler {
 public:
  DIEHandler() { }
  virtual ~DIEHandler() { }

  
  
  
  
  
  
  
  
  
  
  
  
  virtual void ProcessAttributeUnsigned(enum DwarfAttribute attr,
                                        enum DwarfForm form,
                                        uint64 data) { }
  virtual void ProcessAttributeSigned(enum DwarfAttribute attr,
                                      enum DwarfForm form,
                                      int64 data) { }
  virtual void ProcessAttributeReference(enum DwarfAttribute attr,
                                         enum DwarfForm form,
                                         uint64 data) { }
  virtual void ProcessAttributeBuffer(enum DwarfAttribute attr,
                                      enum DwarfForm form,
                                      const char* data,
                                      uint64 len) { }
  virtual void ProcessAttributeString(enum DwarfAttribute attr,
                                      enum DwarfForm form,
                                      const string& data) { }
  virtual void ProcessAttributeSignature(enum DwarfAttribute attr,
                                         enum DwarfForm form,
                                         uint64 signture) { }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual bool EndAttributes() { return false; }

  
  
  
  
  
  
  
  
  
  
  virtual DIEHandler *FindChildHandler(uint64 offset, enum DwarfTag tag) {
    return NULL;
  }

  
  
  
  
  
  virtual void Finish() { };
};



class RootDIEHandler: public DIEHandler {
 public:
  RootDIEHandler() { }
  virtual ~RootDIEHandler() { }

  
  
  
  
  
  virtual bool StartCompilationUnit(uint64 offset, uint8 address_size,
                                    uint8 offset_size, uint64 cu_length,
                                    uint8 dwarf_version) { return true; }

  
  
  
  
  
  
  
  
  virtual bool StartRootDIE(uint64 offset, enum DwarfTag tag) { return true; }
};

class DIEDispatcher: public Dwarf2Handler {
 public:
  
  
  
  DIEDispatcher(RootDIEHandler *root_handler) : root_handler_(root_handler) { }
  
  
  ~DIEDispatcher();
  bool StartCompilationUnit(uint64 offset, uint8 address_size,
                            uint8 offset_size, uint64 cu_length,
                            uint8 dwarf_version);
  bool StartDIE(uint64 offset, enum DwarfTag tag);
  void ProcessAttributeUnsigned(uint64 offset,
                                enum DwarfAttribute attr,
                                enum DwarfForm form,
                                uint64 data);
  void ProcessAttributeSigned(uint64 offset,
                              enum DwarfAttribute attr,
                              enum DwarfForm form,
                              int64 data);
  void ProcessAttributeReference(uint64 offset,
                                 enum DwarfAttribute attr,
                                 enum DwarfForm form,
                                 uint64 data);
  void ProcessAttributeBuffer(uint64 offset,
                              enum DwarfAttribute attr,
                              enum DwarfForm form,
                              const char* data,
                              uint64 len);
  void ProcessAttributeString(uint64 offset,
                              enum DwarfAttribute attr,
                              enum DwarfForm form,
                              const string &data);
  void ProcessAttributeSignature(uint64 offset,
                                 enum DwarfAttribute attr,
                                 enum DwarfForm form,
                                 uint64 signature);
  void EndDIE(uint64 offset);

 private:

  
  
  
  
  struct HandlerStack {
    
    uint64 offset_;

    
    
    DIEHandler *handler_;

    
    bool reported_attributes_end_;
  };

  
  
  
  
  
  
  
  
  
  
  
  
  
  std::stack<HandlerStack> die_handlers_;

  
  
  RootDIEHandler *root_handler_;
};

} 
#endif  
