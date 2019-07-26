




































#ifndef COMMON_LINUX_MODULE_H__
#define COMMON_LINUX_MODULE_H__

#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "common/symbol_data.h"
#include "common/using_std_string.h"
#include "common/unique_string.h"
#include "google_breakpad/common/breakpad_types.h"

namespace google_breakpad {

using std::set;
using std::vector;
using std::map;





class Module {
 public:
  
  typedef uint64_t Address;
  struct File;
  struct Function;
  struct Line;
  struct Extern;

  
  
  
  

  
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

  
  struct Extern {
    Address address;
    string name;
  };

  
  
  
  
  enum ExprHow {
    kExprInvalid = 1,
    kExprPostfix,
    kExprSimple,
    kExprSimpleMem
  };
  struct Expr {
    
    Expr(const UniqueString* ident, long offset, bool deref) {
      if (ident == ustr__empty()) {
        Expr();
      } else {
        postfix_ = "";
        ident_ = ident;
        offset_ = offset;
        how_ = deref ? kExprSimpleMem : kExprSimple;
      }
    }
    
    Expr(string postfix) {
      if (postfix.empty()) {
        Expr();
      } else {
        postfix_ = postfix;
        ident_ = NULL;
        offset_ = 0;
        how_ = kExprPostfix;
      }
    }
    
    Expr() {
      postfix_ = "";
      ident_ = NULL;
      offset_ = 0;
      how_ = kExprInvalid;
    }
    bool isExprInvalid() const { return how_ == kExprInvalid; }

    
    
    
    string getExprPostfix() const {
      switch (how_) {
        case kExprPostfix:
          return postfix_;
        case kExprSimple:
        case kExprSimpleMem: {
          char buf[40];
          sprintf(buf, " %ld %c%s", labs(offset_), offset_ < 0 ? '-' : '+',
                                    how_ == kExprSimple ? "" : " ^");
          return string(FromUniqueString(ident_)) + string(buf);
        }
        case kExprInvalid:
        default:
          assert(0 && "getExprPostfix: invalid Module::Expr type");
          return "Expr::genExprPostfix: kExprInvalid";
      }
    }

    bool operator==(const Expr& other) const {
      return how_ == other.how_ &&
          ident_ == other.ident_ &&
          offset_ == other.offset_ &&
          postfix_ == other.postfix_;
    }

    
    Expr add_delta(long delta) {
      if (delta == 0) {
        return *this;
      }
      
      
      
      
      
      
      
      
      switch (how_) {
        case kExprSimpleMem:
        case kExprPostfix: {
          char buf[40];
          sprintf(buf, " %ld %c", labs(delta), delta < 0 ? '-' : '+');
          return Expr(getExprPostfix() + string(buf));
        }
        case kExprSimple:
          return Expr(ident_, offset_ + delta, false);
        case kExprInvalid:
        default:
          assert(0 && "add_delta: invalid Module::Expr type");
          
          return Expr();
      }
    }

    
    Expr deref() {
      
      
      
      switch (how_) {
        case kExprSimple: {
          Expr t = *this;
          t.how_ = kExprSimpleMem;
          return t;
        }
        case kExprSimpleMem:
        case kExprPostfix: {
          return Expr(getExprPostfix() + " ^");
        }
        case kExprInvalid:
        default:
          assert(0 && "deref: invalid Module::Expr type");
          
          return Expr();
      }
    }

    
    const UniqueString* ident_;
    
    long    offset_;
    
    string  postfix_;
    
    ExprHow how_;

    friend std::ostream& operator<<(std::ostream& stream, const Expr& expr);
  };

  
  
  
  
  typedef map<const UniqueString*, Expr> RuleMap;

  
  
  typedef map<Address, RuleMap> RuleChangeMap;

  
  
  
  struct StackFrameEntry {
    
    
    Address address, size;

    
    
    RuleMap initial_rules;

    
    
    
    
    RuleChangeMap rule_changes;
  };

  struct FunctionCompare {
    bool operator() (const Function *lhs,
                     const Function *rhs) const {
      if (lhs->address == rhs->address)
        return lhs->name < rhs->name;
      return lhs->address < rhs->address;
    }
  };

  struct ExternCompare {
    bool operator() (const Extern *lhs,
                     const Extern *rhs) const {
      return lhs->address < rhs->address;
    }
  };

  struct StackFrameEntryCompare {
    bool operator() (const StackFrameEntry* lhs,
                     const StackFrameEntry* rhs) const {
      return lhs->address < rhs->address;
    }
  };

  
  
  Module(const string &name, const string &os, const string &architecture,
         const string &id);
  ~Module();

  
  
  
  
  
  
  
  
  
  
  
  
  void SetLoadAddress(Address load_address);

  
  
  
  void AddFunction(Function *function);

  
  
  
  void AddFunctions(vector<Function *>::iterator begin,
                    vector<Function *>::iterator end);

  
  
  
  void AddStackFrameEntry(StackFrameEntry *stack_frame_entry);

  
  
  
  void AddExtern(Extern *ext);

  
  
  
  
  File *FindFile(const string &name);
  File *FindFile(const char *name);

  
  
  File *FindExistingFile(const string &name);

  
  
  
  
  
  void GetFunctions(vector<Function *> *vec, vector<Function *>::iterator i);

  
  
  Function* FindFunctionByAddress(Address address);

  
  
  
  
  
  void GetExterns(vector<Extern *> *vec, vector<Extern *>::iterator i);

  
  
  Extern* FindExternByAddress(Address address);

  
  
  
  
  
  void GetFiles(vector<File *> *vec);

  
  
  
  
  
  void GetStackFrameEntries(vector<StackFrameEntry *> *vec);

  
  
  StackFrameEntry* FindStackFrameEntryByAddress(Address address);

  
  
  
  
  
  void AssignSourceIds();

  
  
  
  
  
  
  
  
  
  
  
  
  bool Write(std::ostream &stream, SymbolData symbol_data);

 private:
  
  
  static bool ReportError();

  
  
  
  static bool WriteRuleMap(const RuleMap &rule_map, std::ostream &stream);

  
  string name_, os_, architecture_, id_;

  
  
  
  Address load_address_;

  
  
  struct CompareStringPtrs {
    bool operator()(const string *x, const string *y) const { return *x < *y; }
  };

  
  
  typedef map<const string *, File *, CompareStringPtrs> FileByNameMap;

  
  typedef set<Function *, FunctionCompare> FunctionSet;

  
  typedef set<Extern *, ExternCompare> ExternSet;

  
  typedef set<StackFrameEntry*, StackFrameEntryCompare> StackFrameEntrySet;

  
  
  
  FileByNameMap files_;    
  FunctionSet functions_;  

  
  
  StackFrameEntrySet stack_frame_entries_;

  
  
  ExternSet externs_;
};

}  

#endif  
