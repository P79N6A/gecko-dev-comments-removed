






























#ifndef COMMON_LINUX_MODULE_H__
#define COMMON_LINUX_MODULE_H__

#include <map>
#include <string>
#include <vector>
#include <cstdio>

#include "google_breakpad/common/breakpad_types.h"

namespace google_breakpad {

using std::string;
using std::vector;
using std::map;





class Module {
 public:
  
  typedef u_int64_t Address;
  struct File;
  struct Function;
  struct Line;

  
  
  
  

  
  struct File {
    
    string name_;

    
    
    
    int source_id_;
  };

  
  struct Function {
    
    
    static bool CompareByAddress(const Function *x, const Function *y) {
      return x->address_ < y->address_;
    }

    
    string name_;
    
    
    Address address_, size_;

    
    Address parameter_size_;

    
    
    vector<Line> lines_;
  };

  
  struct Line {
    
    
    static bool CompareByAddress(const Module::Line &x, const Module::Line &y) {
      return x.address_ < y.address_;
    }

    Address address_, size_;    
    File *file_;                
    int number_;                
  };
    
  
  
  Module(const string &name, const string &os, const string &architecture, 
         const string &id);
  ~Module();

  
  
  
  
  
  
  void SetLoadAddress(Address load_address);

  
  
  
  void AddFunction(Function *function);

  
  
  
  void AddFunctions(vector<Function *>::iterator begin,
                    vector<Function *>::iterator end);

  
  
  
  
  File *FindFile(const string &name);
  File *FindFile(const char *name);

  
  
  
  
  
  
  
  
  bool Write(FILE *stream);

private:

  
  
  
  
  
  void AssignSourceIds();

  
  
  static bool ReportError();

  
  string name_, os_, architecture_, id_;

  
  
  
  Address load_address_;

  
  
  struct CompareStringPtrs {
    bool operator()(const string *x, const string *y) { return *x < *y; };
  };

  
  
  typedef map<const string *, File *, CompareStringPtrs> FileByNameMap;

  
  
  
  FileByNameMap files_;                 
  vector<Function *> functions_;        
};

} 

#endif  
