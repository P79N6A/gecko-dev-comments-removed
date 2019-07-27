
































#include "common/module.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <algorithm>
#include <iostream>
#include <utility>

namespace google_breakpad {

using std::dec;
using std::endl;
using std::hex;


Module::Module(const string &name, const string &os,
               const string &architecture, const string &id) :
    name_(name),
    os_(os),
    architecture_(architecture),
    id_(id),
    load_address_(0) { }

Module::~Module() {
  for (FileByNameMap::iterator it = files_.begin(); it != files_.end(); ++it)
    delete it->second;
  for (FunctionSet::iterator it = functions_.begin();
       it != functions_.end(); ++it) {
    delete *it;
  }
  for (StackFrameEntrySet::iterator it = stack_frame_entries_.begin();
       it != stack_frame_entries_.end(); ++it) {
    delete *it;
  }
  for (ExternSet::iterator it = externs_.begin(); it != externs_.end(); ++it)
    delete *it;
}

void Module::SetLoadAddress(Address address) {
  load_address_ = address;
}

void Module::AddFunction(Function *function) {
  
  
  assert(!function->name.empty());
  std::pair<FunctionSet::iterator,bool> ret = functions_.insert(function);
  if (!ret.second) {
    
    
    delete function;
  }
}

void Module::AddFunctions(vector<Function *>::iterator begin,
                          vector<Function *>::iterator end) {
  for (vector<Function *>::iterator it = begin; it != end; ++it)
    AddFunction(*it);
}

void Module::AddStackFrameEntry(StackFrameEntry* stack_frame_entry) {
  std::pair<StackFrameEntrySet::iterator,bool> ret =
      stack_frame_entries_.insert(stack_frame_entry);
  if (!ret.second) {
    
    
    delete stack_frame_entry;
  }
}

void Module::AddExtern(Extern *ext) {
  Function func;
  func.name = ext->name;
  func.address = ext->address;

  
  
  
  if (functions_.find(&func) == functions_.end()) {
    std::pair<ExternSet::iterator,bool> ret = externs_.insert(ext);
    if (!ret.second) {
      
      
      delete ext;
    }
  } else {
    delete ext;
  }
}

void Module::GetFunctions(vector<Function *> *vec,
                          vector<Function *>::iterator i) {
  vec->insert(i, functions_.begin(), functions_.end());
}

template<typename T>
bool EntryContainsAddress(T entry, Module::Address address) {
  return entry->address <= address && address < entry->address + entry->size;
}

Module::Function* Module::FindFunctionByAddress(Address address) {
  Function search;
  search.address = address;
  
  
  
  search.name = "\xFF";
  FunctionSet::iterator it = functions_.upper_bound(&search);
  if (it == functions_.begin())
    return NULL;

  it--;

  if (EntryContainsAddress(*it, address))
    return *it;

  return NULL;
}

void Module::GetExterns(vector<Extern *> *vec,
                        vector<Extern *>::iterator i) {
  vec->insert(i, externs_.begin(), externs_.end());
}

Module::Extern* Module::FindExternByAddress(Address address) {
  Extern search;
  search.address = address;
  ExternSet::iterator it = externs_.upper_bound(&search);

  if (it == externs_.begin())
    return NULL;

  it--;
  if ((*it)->address > address)
    return NULL;

  return *it;
}

Module::File *Module::FindFile(const string &name) {
  
  
  
  
  
  
  
  
  
  
  FileByNameMap::iterator destiny = files_.lower_bound(&name);
  if (destiny == files_.end()
      || *destiny->first != name) {  
    File *file = new File;
    file->name = name;
    file->source_id = -1;
    destiny = files_.insert(destiny,
                            FileByNameMap::value_type(&file->name, file));
  }
  return destiny->second;
}

Module::File *Module::FindFile(const char *name) {
  string name_string = name;
  return FindFile(name_string);
}

Module::File *Module::FindExistingFile(const string &name) {
  FileByNameMap::iterator it = files_.find(&name);
  return (it == files_.end()) ? NULL : it->second;
}

void Module::GetFiles(vector<File *> *vec) {
  vec->clear();
  for (FileByNameMap::iterator it = files_.begin(); it != files_.end(); ++it)
    vec->push_back(it->second);
}

void Module::GetStackFrameEntries(vector<StackFrameEntry *>* vec) {
  vec->clear();
  vec->insert(vec->begin(), stack_frame_entries_.begin(),
              stack_frame_entries_.end());
}

Module::StackFrameEntry* Module::FindStackFrameEntryByAddress(Address address) {
  StackFrameEntry search;
  search.address = address;
  StackFrameEntrySet::iterator it = stack_frame_entries_.upper_bound(&search);

  if (it == stack_frame_entries_.begin())
    return NULL;

  it--;
  if (EntryContainsAddress(*it, address))
    return *it;

  return NULL;
}

void Module::AssignSourceIds() {
  
  for (FileByNameMap::iterator file_it = files_.begin();
       file_it != files_.end(); ++file_it) {
    file_it->second->source_id = -1;
  }

  
  
  for (FunctionSet::const_iterator func_it = functions_.begin();
       func_it != functions_.end(); ++func_it) {
    Function *func = *func_it;
    for (vector<Line>::iterator line_it = func->lines.begin();
         line_it != func->lines.end(); ++line_it)
      line_it->file->source_id = 0;
  }

  
  
  
  
  int next_source_id = 0;
  for (FileByNameMap::iterator file_it = files_.begin();
       file_it != files_.end(); ++file_it) {
    if (!file_it->second->source_id)
      file_it->second->source_id = next_source_id++;
  }
}

bool Module::ReportError() {
  fprintf(stderr, "error writing symbol file: %s\n",
          strerror(errno));
  return false;
}

std::ostream& operator<<(std::ostream& stream, const Module::Expr& expr) {
  assert(!expr.isExprInvalid());
  switch (expr.how_) {
    case Module::kExprSimple:
      stream << FromUniqueString(expr.ident_) << " " << expr.offset_ << " +";
      break;
    case Module::kExprSimpleMem:
      stream << FromUniqueString(expr.ident_) << " " << expr.offset_ << " + ^";
      break;
    case Module::kExprPostfix:
      stream << expr.postfix_; break;
    case Module::kExprInvalid:
    default:
      break;
  }
  return stream;
}

bool Module::WriteRuleMap(const RuleMap &rule_map, std::ostream &stream) {
  
  
  
  
  std::vector<const UniqueString*> rr_names;
  for (RuleMap::const_iterator it = rule_map.begin();
       it != rule_map.end(); ++it) {
    rr_names.push_back(it->first);
  }

  std::sort(rr_names.begin(), rr_names.end(), LessThan_UniqueString);

  
  for (std::vector<const UniqueString*>::const_iterator name = rr_names.begin();
       name != rr_names.end();
       ++name) {
    if (name != rr_names.begin())
      stream << " ";
    stream << FromUniqueString(*name) << ": " << rule_map.find(*name)->second;
  }
  return stream.good();
}

bool Module::Write(std::ostream &stream, SymbolData symbol_data) {
  stream << "MODULE " << os_ << " " << architecture_ << " "
         << id_ << " " << name_ << endl;
  if (!stream.good())
    return ReportError();

  if (symbol_data != ONLY_CFI) {
    AssignSourceIds();

    
    for (FileByNameMap::iterator file_it = files_.begin();
         file_it != files_.end(); ++file_it) {
      File *file = file_it->second;
      if (file->source_id >= 0) {
        stream << "FILE " << file->source_id << " " <<  file->name << endl;
        if (!stream.good())
          return ReportError();
      }
    }

    
    for (FunctionSet::const_iterator func_it = functions_.begin();
         func_it != functions_.end(); ++func_it) {
      Function *func = *func_it;
      stream << "FUNC " << hex
             << (func->address - load_address_) << " "
             << func->size << " "
             << func->parameter_size << " "
             << func->name << dec << endl;
      if (!stream.good())
        return ReportError();

      for (vector<Line>::iterator line_it = func->lines.begin();
           line_it != func->lines.end(); ++line_it) {
        stream << hex
               << (line_it->address - load_address_) << " "
               << line_it->size << " "
               << dec
               << line_it->number << " "
               << line_it->file->source_id << endl;
        if (!stream.good())
          return ReportError();
      }
    }

    
    for (ExternSet::const_iterator extern_it = externs_.begin();
         extern_it != externs_.end(); ++extern_it) {
      Extern *ext = *extern_it;
      stream << "PUBLIC " << hex
             << (ext->address - load_address_) << " 0 "
             << ext->name << dec << endl;
    }
  }

  if (symbol_data != NO_CFI) {
    
    StackFrameEntrySet::const_iterator frame_it;
    for (frame_it = stack_frame_entries_.begin();
         frame_it != stack_frame_entries_.end(); ++frame_it) {
      StackFrameEntry *entry = *frame_it;
      stream << "STACK CFI INIT " << hex
             << (entry->address - load_address_) << " "
             << entry->size << " " << dec;
      if (!stream.good()
          || !WriteRuleMap(entry->initial_rules, stream))
        return ReportError();

      stream << endl;

      
      for (RuleChangeMap::const_iterator delta_it = entry->rule_changes.begin();
           delta_it != entry->rule_changes.end(); ++delta_it) {
        stream << "STACK CFI " << hex
               << (delta_it->first - load_address_) << " " << dec;
        if (!stream.good()
            || !WriteRuleMap(delta_it->second, stream))
          return ReportError();

        stream << endl;
      }
    }
  }

  return true;
}

}  
