
































#include "common/stabs_reader.h"

#include <assert.h>
#include <stab.h>
#include <string.h>

using std::vector;

namespace google_breakpad {

StabsReader::EntryIterator::EntryIterator(const ByteBuffer *buffer,
                                          bool big_endian, size_t value_size)
    : value_size_(value_size), cursor_(buffer, big_endian) {
  
  
  assert(value_size == 4 || value_size == 8);
  entry_.index = 0;
  Fetch();
}

void StabsReader::EntryIterator::Fetch() {
  cursor_
      .Read(4, false, &entry_.name_offset)
      .Read(1, false, &entry_.type)
      .Read(1, false, &entry_.other)
      .Read(2, false, &entry_.descriptor)
      .Read(value_size_, false, &entry_.value);
  entry_.at_end = !cursor_;
}

StabsReader::StabsReader(const uint8_t *stab,    size_t stab_size,
                         const uint8_t *stabstr, size_t stabstr_size,
                         bool big_endian, size_t value_size, bool unitized,
                         StabsHandler *handler)
    : entries_(stab, stab_size),
      strings_(stabstr, stabstr_size),
      iterator_(&entries_, big_endian, value_size),
      unitized_(unitized),
      handler_(handler),
      string_offset_(0),
      next_cu_string_offset_(0),
      current_source_file_(NULL) { }

const char *StabsReader::SymbolString() {
  ptrdiff_t offset = string_offset_ + iterator_->name_offset;
  if (offset < 0 || (size_t) offset >= strings_.Size()) {
    handler_->Warning("symbol %d: name offset outside the string section\n",
                      iterator_->index);
    
    
    offset = 0;
  }
  return reinterpret_cast<const char *>(strings_.start + offset);
}

bool StabsReader::Process() {
  while (!iterator_->at_end) {
    if (iterator_->type == N_SO) {
      if (! ProcessCompilationUnit())
        return false;
    } else if (iterator_->type == N_UNDF && unitized_) {
      
      
      
      
      
      
      
      
      
      
      
      
      string_offset_ = next_cu_string_offset_;
      next_cu_string_offset_ = iterator_->value;
      ++iterator_;
    } else
      ++iterator_;
  }
  return true;
}

bool StabsReader::ProcessCompilationUnit() {
  assert(!iterator_->at_end && iterator_->type == N_SO);

  
  
  
  const char *build_directory = NULL;  
  {
    const char *name = SymbolString();
    if (name[0] && name[strlen(name) - 1] == '/') {
      build_directory = name;
      ++iterator_;
    }
  }
      
  
  
  {
    if (iterator_->at_end || iterator_->type != N_SO)
      return true;
    const char *name = SymbolString();
    if (name[0] == '\0') {
      
      
      
      ++iterator_;
      return true;
    }
    current_source_file_ = name;
  }

  if (! handler_->StartCompilationUnit(current_source_file_,
                                       iterator_->value,
                                       build_directory))
    return false;

  ++iterator_;

  
  
  
  

  
  while (!iterator_->at_end && iterator_->type != N_SO) {
    if (iterator_->type == N_FUN) {
      if (! ProcessFunction())
        return false;
    } else if (iterator_->type == N_SLINE) {
      
      Line line;
      
      
      line.address = iterator_->value;
      line.filename = current_source_file_;
      
      
      
      line.number = (uint16_t) iterator_->descriptor;
      queued_lines_.push_back(line);
      ++iterator_;
    } else if (iterator_->type == N_SOL) {
      current_source_file_ = SymbolString();
      ++iterator_;
    } else {
      
      ++iterator_;
    }
  }

  
  
  uint64_t ending_address = 0;
  if (!iterator_->at_end) {
    assert(iterator_->type == N_SO);
    const char *name = SymbolString();
    if (name[0] == '\0') {
      ending_address = iterator_->value;
      ++iterator_;
    }
  }

  if (! handler_->EndCompilationUnit(ending_address))
    return false;

  queued_lines_.clear();

  return true;
}          

bool StabsReader::ProcessFunction() {
  assert(!iterator_->at_end && iterator_->type == N_FUN);

  uint64_t function_address = iterator_->value;
  
  
  
  const char *stab_string = SymbolString();
  const char *name_end = strchr(stab_string, ':');
  if (! name_end)
    name_end = stab_string + strlen(stab_string);
  std::string name(stab_string, name_end - stab_string);
  if (! handler_->StartFunction(name, function_address))
    return false;
  ++iterator_;

  
  for (vector<Line>::const_iterator it = queued_lines_.begin();
       it != queued_lines_.end(); it++) {
    if (!handler_->Line(it->address, it->filename, it->number))
      return false;
  }
  queued_lines_.clear();
  
  while (!iterator_->at_end) {
    if (iterator_->type == N_SO || iterator_->type == N_FUN)
      break;
    else if (iterator_->type == N_SLINE) {
      
      
      uint64_t line_address = function_address + iterator_->value;
      
      
      
      uint16_t line_number = iterator_->descriptor;
      if (! handler_->Line(line_address, current_source_file_, line_number))
        return false;
      ++iterator_;
    } else if (iterator_->type == N_SOL) {
      current_source_file_ = SymbolString();
      ++iterator_;
    } else
      
      ++iterator_;
  }

  
  
  uint64_t ending_address = 0;
  if (!iterator_->at_end) {
    assert(iterator_->type == N_SO || iterator_->type == N_FUN);
    if (iterator_->type == N_FUN) {
      const char *name = SymbolString();
      if (name[0] == '\0') {
        
        
        ending_address = function_address + iterator_->value;
        ++iterator_;
      } else {
        
        
        
        ending_address = iterator_->value;
      }
    } else {
      
      
      
      
      ending_address = iterator_->value;
    }
  }

  if (! handler_->EndFunction(ending_address))
    return false;

  return true;
}

} 
