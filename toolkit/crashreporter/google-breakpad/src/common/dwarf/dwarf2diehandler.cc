
































#include "common/dwarf/dwarf2diehandler.h"

#include <assert.h>

namespace dwarf2reader {

DIEDispatcher::~DIEDispatcher() {
  while (!die_handlers_.empty()) {
    HandlerStack &entry = die_handlers_.top();
    if (entry.handler_ != root_handler_)
      delete entry.handler_;
    die_handlers_.pop();
  }
}

bool DIEDispatcher::StartCompilationUnit(uint64 offset, uint8 address_size,
                                         uint8 offset_size, uint64 cu_length,
                                         uint8 dwarf_version) {
  return root_handler_->StartCompilationUnit(offset, address_size,
                                             offset_size, cu_length,
                                             dwarf_version);
}

bool DIEDispatcher::StartDIE(uint64 offset, enum DwarfTag tag,
                             const AttributeList& attrs) {
  
  HandlerStack *parent = die_handlers_.empty() ? NULL : &die_handlers_.top();

  
  
  if (parent && parent->handler_ && !parent->reported_attributes_end_) {
    parent->reported_attributes_end_ = true;
    if (!parent->handler_->EndAttributes()) {
      
      
      parent->handler_->Finish();
      if (parent->handler_ != root_handler_)
        delete parent->handler_;
      parent->handler_ = NULL;
      return false;
    }
  }

  
  DIEHandler *handler;
  if (parent) {
    if (parent->handler_)
      
      handler = parent->handler_->FindChildHandler(offset, tag, attrs);
    else
      
      
      handler = NULL;
  } else {
    
    
    
    
    if (root_handler_->StartRootDIE(offset, tag, attrs))
      handler = root_handler_;
    else
      handler = NULL;
  }

  
  
  
  
  if (handler || !parent || parent->handler_) {
    HandlerStack entry;
    entry.offset_ = offset;
    entry.handler_ = handler;
    entry.reported_attributes_end_ = false;
    die_handlers_.push(entry);
  }

  return handler != NULL;
}

void DIEDispatcher::EndDIE(uint64 offset) {
  assert(!die_handlers_.empty());
  HandlerStack *entry = &die_handlers_.top();
  if (entry->handler_) {
    
    assert(entry->offset_ == offset);
    
    
    if (!entry->reported_attributes_end_)
      entry->handler_->EndAttributes(); 
    entry->handler_->Finish();
    if (entry->handler_ != root_handler_)
      delete entry->handler_;
  } else {
    
    
    if (entry->offset_ != offset)
      return;
  }
  die_handlers_.pop();
}

void DIEDispatcher::ProcessAttributeUnsigned(uint64 offset,
                                             enum DwarfAttribute attr,
                                             enum DwarfForm form,
                                             uint64 data) {
  HandlerStack &current = die_handlers_.top();
  
  assert(offset == current.offset_);
  current.handler_->ProcessAttributeUnsigned(attr, form, data);
}

void DIEDispatcher::ProcessAttributeSigned(uint64 offset,
                                           enum DwarfAttribute attr,
                                           enum DwarfForm form,
                                           int64 data) {
  HandlerStack &current = die_handlers_.top();
  
  assert(offset == current.offset_);
  current.handler_->ProcessAttributeSigned(attr, form, data);
}

void DIEDispatcher::ProcessAttributeReference(uint64 offset,
                                              enum DwarfAttribute attr,
                                              enum DwarfForm form,
                                              uint64 data) {
  HandlerStack &current = die_handlers_.top();
  
  assert(offset == current.offset_);
  current.handler_->ProcessAttributeReference(attr, form, data);
}

void DIEDispatcher::ProcessAttributeBuffer(uint64 offset,
                                           enum DwarfAttribute attr,
                                           enum DwarfForm form,
                                           const char* data,
                                           uint64 len) {
  HandlerStack &current = die_handlers_.top();
  
  assert(offset == current.offset_);
  current.handler_->ProcessAttributeBuffer(attr, form, data, len);
}

void DIEDispatcher::ProcessAttributeString(uint64 offset,
                                           enum DwarfAttribute attr,
                                           enum DwarfForm form,
                                           const string& data) {
  HandlerStack &current = die_handlers_.top();
  
  assert(offset == current.offset_);
  current.handler_->ProcessAttributeString(attr, form, data);
}

} 
