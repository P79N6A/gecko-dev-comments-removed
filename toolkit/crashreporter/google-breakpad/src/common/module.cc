
































#include "common/module.h"

#include <errno.h>
#include <string.h>

namespace google_breakpad {

Module::Module(const string &name, const string &os,
               const string &architecture, const string &id) :
    name_(name),
    os_(os),
    architecture_(architecture),
    id_(id),
    load_address_(0) { }

Module::~Module() {
  for (FileByNameMap::iterator it = files_.begin(); it != files_.end(); it++)
    delete it->second;
  for (FunctionSet::iterator it = functions_.begin();
       it != functions_.end(); it++)
    delete *it;
  for (vector<StackFrameEntry *>::iterator it = stack_frame_entries_.begin();
       it != stack_frame_entries_.end(); it++)
    delete *it;
}

void Module::SetLoadAddress(Address address) {
  load_address_ = address;
}

void Module::AddFunction(Function *function) {
  std::pair<FunctionSet::iterator,bool> ret = functions_.insert(function);
  if (!ret.second) {
    
    delete function;
  }
}

void Module::AddFunctions(vector<Function *>::iterator begin,
                          vector<Function *>::iterator end) {
  for (vector<Function *>::iterator it = begin; it != end; it++)
    AddFunction(*it);
}

void Module::AddStackFrameEntry(StackFrameEntry *stack_frame_entry) {
  stack_frame_entries_.push_back(stack_frame_entry);
}

void Module::GetFunctions(vector<Function *> *vec,
                          vector<Function *>::iterator i) {
  vec->insert(i, functions_.begin(), functions_.end());
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
  for (FileByNameMap::iterator it = files_.begin(); it != files_.end(); it++)
    vec->push_back(it->second);
}

void Module::GetStackFrameEntries(vector<StackFrameEntry *> *vec) {
  *vec = stack_frame_entries_;
}

void Module::AssignSourceIds() {
  
  for (FileByNameMap::iterator file_it = files_.begin();
       file_it != files_.end(); file_it++)
    file_it->second->source_id = -1;

  
  
  for (FunctionSet::const_iterator func_it = functions_.begin();
       func_it != functions_.end(); func_it++) {
    Function *func = *func_it;
    for (vector<Line>::iterator line_it = func->lines.begin();
         line_it != func->lines.end(); line_it++)
      line_it->file->source_id = 0;
  }

  
  
  
  
  int next_source_id = 0;
  for (FileByNameMap::iterator file_it = files_.begin();
       file_it != files_.end(); file_it++)
    if (!file_it->second->source_id)
      file_it->second->source_id = next_source_id++;
}

bool Module::ReportError() {
  fprintf(stderr, "error writing symbol file: %s\n",
          strerror(errno));
  return false;
}

bool Module::WriteRuleMap(const RuleMap &rule_map, FILE *stream) {
  for (RuleMap::const_iterator it = rule_map.begin();
       it != rule_map.end(); it++) {
    if (it != rule_map.begin() &&
        0 > putc(' ', stream))
      return false;
    if (0 > fprintf(stream, "%s: %s", it->first.c_str(), it->second.c_str()))
      return false;
  }
  return true;
}

bool Module::Write(FILE *stream) {
  if (0 > fprintf(stream, "MODULE %s %s %s %s\n",
                  os_.c_str(), architecture_.c_str(), id_.c_str(),
                  name_.c_str()))
    return ReportError();

  AssignSourceIds();

  
  for (FileByNameMap::iterator file_it = files_.begin();
       file_it != files_.end(); file_it++) {
    File *file = file_it->second;
    if (file->source_id >= 0) {
      if (0 > fprintf(stream, "FILE %d %s\n",
                      file->source_id, file->name.c_str()))
        return ReportError();
    }
  }

  
  for (FunctionSet::const_iterator func_it = functions_.begin();
       func_it != functions_.end(); func_it++) {
    Function *func = *func_it;
    if (0 > fprintf(stream, "FUNC %llx %llx %llx %s\n",
                    (unsigned long long) (func->address - load_address_),
                    (unsigned long long) func->size,
                    (unsigned long long) func->parameter_size,
                    func->name.c_str()))
      return ReportError();
    for (vector<Line>::iterator line_it = func->lines.begin();
         line_it != func->lines.end(); line_it++)
      if (0 > fprintf(stream, "%llx %llx %d %d\n",
                      (unsigned long long) (line_it->address - load_address_),
                      (unsigned long long) line_it->size,
                      line_it->number,
                      line_it->file->source_id))
        return ReportError();
  }

  
  vector<StackFrameEntry *>::const_iterator frame_it;
  for (frame_it = stack_frame_entries_.begin();
       frame_it != stack_frame_entries_.end(); frame_it++) {
    StackFrameEntry *entry = *frame_it;
    if (0 > fprintf(stream, "STACK CFI INIT %llx %llx ",
                    (unsigned long long) entry->address - load_address_,
                    (unsigned long long) entry->size)
        || !WriteRuleMap(entry->initial_rules, stream)
        || 0 > putc('\n', stream))
      return ReportError();

    
    for (RuleChangeMap::const_iterator delta_it = entry->rule_changes.begin();
         delta_it != entry->rule_changes.end(); delta_it++) {
      if (0 > fprintf(stream, "STACK CFI %llx ",
                      (unsigned long long) delta_it->first - load_address_)
          || !WriteRuleMap(delta_it->second, stream)
          || 0 > putc('\n', stream))
        return ReportError();
    }
  }

  return true;
}

} 
