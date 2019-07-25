




































#ifndef COMMON_LINUX_MODULE_H__
#define COMMON_LINUX_MODULE_H__

#include <stdio.h>

#include <map>
#include <string>
#include <vector>

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
    
    string name;

    
    
    
    int source_id;
  };

  
  struct Function {
    
    
    static bool CompareByAddress(const Function *x, const Function *y) {
      return x->address < y->address;
    }

    
    string name;
    
    
    Address address, size;

    
    Address parameter_size;

    
    
    vector<Line> lines;
  };

  
  struct Line {
    
    
    static bool CompareByAddress(const Module::Line &x, const Module::Line &y) {
      return x.address < y.address;
    }

    Address address, size;    
    File *file;                
    int number;                
  };

  
  
  
  
  typedef map<string, string> RuleMap;

  
  
  typedef map<Address, RuleMap> RuleChangeMap;
  
  
  
  
  struct StackFrameEntry {
    
    
    Address address, size;

    
    
    RuleMap initial_rules;

    
    
    
    
    RuleChangeMap rule_changes;
  };
    
  
  
  Module(const string &name, const string &os, const string &architecture, 
         const string &id);
  ~Module();

  
  
  
  
  
  
  
  
  
  
  
  
  void SetLoadAddress(Address load_address);

  
  
  
  void AddFunction(Function *function);

  
  
  
  void AddFunctions(vector<Function *>::iterator begin,
                    vector<Function *>::iterator end);

  
  
  
  
  void AddStackFrameEntry(StackFrameEntry *stack_frame_entry);

  
  
  
  
  File *FindFile(const string &name);
  File *FindFile(const char *name);

  
  
  File *FindExistingFile(const string &name);

  
  
  
  
  
  void GetFunctions(vector<Function *> *vec, vector<Function *>::iterator i);

  
  
  
  
  
  void GetFiles(vector<File *> *vec);

  
  
  
  
  
  void GetStackFrameEntries(vector<StackFrameEntry *> *vec);

  
  
  
  
  
  void AssignSourceIds();

  
  
  
  
  
  
  
  
  bool Write(FILE *stream);

 private:

  
  
  static bool ReportError();

  
  
  
  static bool WriteRuleMap(const RuleMap &rule_map, FILE *stream);

  
  string name_, os_, architecture_, id_;

  
  
  
  Address load_address_;

  
  
  struct CompareStringPtrs {
    bool operator()(const string *x, const string *y) { return *x < *y; };
  };

  
  
  typedef map<const string *, File *, CompareStringPtrs> FileByNameMap;

  
  
  
  FileByNameMap files_;                 
  vector<Function *> functions_;        

  
  
  vector<StackFrameEntry *> stack_frame_entries_;
};

} 

#endif  
