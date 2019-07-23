





























#include <a.out.h>
#include <stab.h>
#include <cstring>
#include <cassert>

#include "common/linux/stabs_reader.h"

namespace google_breakpad {

StabsReader::StabsReader(const uint8_t *stab,    size_t stab_size,
                         const uint8_t *stabstr, size_t stabstr_size,
                         StabsHandler *handler) :
    stabstr_(stabstr),
    stabstr_size_(stabstr_size),
    handler_(handler),
    symbol_(NULL),
    current_source_file_(NULL) { 
  symbols_ = reinterpret_cast<const struct nlist *>(stab);
  symbols_end_ = symbols_ + (stab_size / sizeof (*symbols_));
}

const char *StabsReader::SymbolString() {
  ptrdiff_t offset = symbol_->n_un.n_strx;
  if (offset < 0 || (size_t) offset >= stabstr_size_) {
    handler_->Warning("symbol %d: name offset outside the string section",
                      symbol_ - symbols_);
    
    
    offset = 0;
  }
  return reinterpret_cast<const char *>(stabstr_ + offset);
}

bool StabsReader::Process() {
  symbol_ = symbols_;
  while (symbol_ < symbols_end_) {
    if (symbol_->n_type == N_SO) {
      if (! ProcessCompilationUnit())
        return false;
    } else
      symbol_++;
  }
  return true;
}

bool StabsReader::ProcessCompilationUnit() {
  assert(symbol_ < symbols_end_ && symbol_->n_type == N_SO);

  
  
  
  const char *build_directory = NULL;  
  {
    const char *name = SymbolString();
    if (name[0] && name[strlen(name) - 1] == '/') {
      build_directory = name;
      symbol_++;
    }
  }
      
  
  
  {
    if (symbol_ >= symbols_end_ || symbol_->n_type != N_SO)
      return true;
    const char *name = SymbolString();
    if (name[0] == '\0')
      return true;
    current_source_file_ = name;
  }

  if (! handler_->StartCompilationUnit(current_source_file_,
                                       SymbolValue(),
                                       build_directory))
    return false;

  symbol_++;

  
  
  
  

  
  while (symbol_ < symbols_end_ && symbol_->n_type != N_SO) {
    if (symbol_->n_type == N_FUN) {
      if (! ProcessFunction())
        return false;
    } else
      
      symbol_++;
  }

  
  
  uint64_t ending_address = 0;
  if (symbol_ < symbols_end_) {
    assert(symbol_->n_type == N_SO);
    const char *name = SymbolString();
    if (name[0] == '\0') {
      ending_address = SymbolValue();
      symbol_++;
    }
  }

  if (! handler_->EndCompilationUnit(ending_address))
    return false;

  return true;
}          

bool StabsReader::ProcessFunction() {
  assert(symbol_ < symbols_end_ && symbol_->n_type == N_FUN);

  uint64_t function_address = SymbolValue();
  
  
  
  const char *stab_string = SymbolString();
  const char *name_end = strchr(stab_string, ':');
  if (! name_end)
    name_end = stab_string + strlen(stab_string);
  std::string name(stab_string, name_end - stab_string);
  if (! handler_->StartFunction(name, function_address))
    return false;
  symbol_++;

  while (symbol_ < symbols_end_) {
    if (symbol_->n_type == N_SO || symbol_->n_type == N_FUN)
      break;
    else if (symbol_->n_type == N_SLINE) {
      
      
      uint64_t line_address = function_address + SymbolValue();
      
      
      
      uint16_t line_number = symbol_->n_desc;
      if (! handler_->Line(line_address, current_source_file_, line_number))
        return false;
      symbol_++;
    } else if (symbol_->n_type == N_SOL) {
      current_source_file_ = SymbolString();
      symbol_++;
    } else
      
      symbol_++;
  }

  
  
  uint64_t ending_address = 0;
  if (symbol_ < symbols_end_) {
    assert(symbol_->n_type == N_SO || symbol_->n_type == N_FUN);
    ending_address = SymbolValue();
    
  }

  if (! handler_->EndFunction(ending_address))
    return false;

  return true;
}

} 
