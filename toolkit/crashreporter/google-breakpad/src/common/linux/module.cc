




























#include <cerrno>
#include <cstring>
#include "common/linux/module.h"

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
  for (vector<Function *>::iterator it = functions_.begin();
       it != functions_.end(); it++)
    delete *it;
}

void Module::SetLoadAddress(Address address) {
  load_address_ = address;
}

void Module::AddFunction(Function *function) {
  functions_.push_back(function);
}

void Module::AddFunctions(vector<Function *>::iterator begin,
                          vector<Function *>::iterator end) {
  functions_.insert(functions_.end(), begin, end);
}

Module::File *Module::FindFile(const string &name) {
  
  
  
  
  
  
  
  
  
  
  FileByNameMap::iterator destiny = files_.lower_bound(&name);
  if (destiny == files_.end()
      || *destiny->first != name) {  
    File *file = new File;
    file->name_ = name;
    file->source_id_ = -1;
    destiny = files_.insert(destiny,
                            FileByNameMap::value_type(&file->name_, file));
  }
  return destiny->second;
}

Module::File *Module::FindFile(const char *name) {
  string name_string = name;
  return FindFile(name_string);
}

void Module::AssignSourceIds() {
  
  for (FileByNameMap::iterator file_it = files_.begin();
       file_it != files_.end(); file_it++)
    file_it->second->source_id_ = -1;

  
  
  for (vector<Function *>::const_iterator func_it = functions_.begin();
       func_it != functions_.end(); func_it++) {
    Function *func = *func_it;
    for (vector<Line>::iterator line_it = func->lines_.begin();
         line_it != func->lines_.end(); line_it++)
      line_it->file_->source_id_ = 0;
  }

  
  
  
  
  int next_source_id = 0;
  for (FileByNameMap::iterator file_it = files_.begin();
       file_it != files_.end(); file_it++)
    if (! file_it->second->source_id_)
      file_it->second->source_id_ = next_source_id++;
}

bool Module::ReportError() {
  fprintf(stderr, "error writing symbol file: %s\n",
          strerror (errno));
  return false;
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
    if (file->source_id_ >= 0) {
      if (0 > fprintf(stream, "FILE %d %s\n",
                      file->source_id_, file->name_.c_str()))
        return ReportError();
    }
  }

  
  for (vector<Function *>::const_iterator func_it = functions_.begin();
       func_it != functions_.end(); func_it++) {
    Function *func = *func_it;
    if (0 > fprintf(stream, "FUNC %lx %lx %lu %s\n",
                    (unsigned long) (func->address_ - load_address_),
                    (unsigned long) func->size_,
                    (unsigned long) func->parameter_size_,
                    func->name_.c_str()))
      return ReportError();
    for (vector<Line>::iterator line_it = func->lines_.begin();
         line_it != func->lines_.end(); line_it++)
      if (0 > fprintf(stream, "%lx %lx %d %d\n",
                      (unsigned long) (line_it->address_ - load_address_),
                      (unsigned long) line_it->size_,
                      line_it->number_,
                      line_it->file_->source_id_))
        return ReportError();
  }

  return true;
}

} 
