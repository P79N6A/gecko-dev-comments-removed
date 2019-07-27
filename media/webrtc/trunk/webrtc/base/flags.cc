









#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#if defined(WEBRTC_WIN)
#include "webrtc/base/win32.h"
#include <shellapi.h>
#endif

#include "webrtc/base/flags.h"

namespace rtc {



Flag::Flag(const char* file, const char* name, const char* comment,
           Type type, void* variable, FlagValue default__)
    : file_(file),
      name_(name),
      comment_(comment),
      type_(type),
      variable_(reinterpret_cast<FlagValue*>(variable)),
      default_(default__) {
  FlagList::Register(this);
}


void Flag::SetToDefault() {
  
  
  
  
  
  switch (type_) {
    case Flag::BOOL:
      variable_->b = default_.b;
      return;
    case Flag::INT:
      variable_->i = default_.i;
      return;
    case Flag::FLOAT:
      variable_->f = default_.f;
      return;
    case Flag::STRING:
      variable_->s = default_.s;
      return;
  }
  FATAL() << "unreachable code";
}


static const char* Type2String(Flag::Type type) {
  switch (type) {
    case Flag::BOOL: return "bool";
    case Flag::INT: return "int";
    case Flag::FLOAT: return "float";
    case Flag::STRING: return "string";
  }
  FATAL() << "unreachable code";
}


static void PrintFlagValue(Flag::Type type, FlagValue* p) {
  switch (type) {
    case Flag::BOOL:
      printf("%s", (p->b ? "true" : "false"));
      return;
    case Flag::INT:
      printf("%d", p->i);
      return;
    case Flag::FLOAT:
      printf("%f", p->f);
      return;
    case Flag::STRING:
      printf("%s", p->s);
      return;
  }
  FATAL() << "unreachable code";
}


void Flag::Print(bool print_current_value) {
  printf("  --%s (%s)  type: %s  default: ", name_, comment_,
          Type2String(type_));
  PrintFlagValue(type_, &default_);
  if (print_current_value) {
    printf("  current value: ");
    PrintFlagValue(type_, variable_);
  }
  printf("\n");
}





Flag* FlagList::list_ = NULL;


FlagList::FlagList() {
  list_ = NULL;
}

void FlagList::Print(const char* file, bool print_current_value) {
  
  
  const char* current = NULL;
  for (Flag* f = list_; f != NULL; f = f->next()) {
    if (file == NULL || file == f->file()) {
      if (current != f->file()) {
        printf("Flags from %s:\n", f->file());
        current = f->file();
      }
      f->Print(print_current_value);
    }
  }
}


Flag* FlagList::Lookup(const char* name) {
  Flag* f = list_;
  while (f != NULL && strcmp(name, f->name()) != 0)
    f = f->next();
  return f;
}


void FlagList::SplitArgument(const char* arg,
                             char* buffer, int buffer_size,
                             const char** name, const char** value,
                             bool* is_bool) {
  *name = NULL;
  *value = NULL;
  *is_bool = false;

  if (*arg == '-') {
    
    arg++;  
    if (*arg == '-')
      arg++;  
    if (arg[0] == 'n' && arg[1] == 'o') {
      arg += 2;  
      *is_bool = true;
    }
    *name = arg;

    
    while (*arg != '\0' && *arg != '=')
      arg++;

    
    if (*arg == '=') {
      
      int n = static_cast<int>(arg - *name);
      CHECK_LT(n, buffer_size);
      memcpy(buffer, *name, n * sizeof(char));
      buffer[n] = '\0';
      *name = buffer;
      
      *value = arg + 1;
    }
  }
}


int FlagList::SetFlagsFromCommandLine(int* argc, const char** argv,
                                      bool remove_flags) {
  
  for (int i = 1; i < *argc; ) {
    int j = i;  
    const char* arg = argv[i++];

    
    char buffer[1024];
    const char* name;
    const char* value;
    bool is_bool;
    SplitArgument(arg, buffer, sizeof buffer, &name, &value, &is_bool);

    if (name != NULL) {
      
      Flag* flag = Lookup(name);
      if (flag == NULL) {
        fprintf(stderr, "Error: unrecognized flag %s\n", arg);
        return j;
      }

      
      if (flag->type() != Flag::BOOL && value == NULL) {
        if (i < *argc) {
          value = argv[i++];
        } else {
          fprintf(stderr, "Error: missing value for flag %s of type %s\n",
            arg, Type2String(flag->type()));
          return j;
        }
      }

      
      char empty[] = { '\0' };
      char* endp = empty;
      switch (flag->type()) {
        case Flag::BOOL:
          *flag->bool_variable() = !is_bool;
          break;
        case Flag::INT:
          *flag->int_variable() = strtol(value, &endp, 10);
          break;
        case Flag::FLOAT:
          *flag->float_variable() = strtod(value, &endp);
          break;
        case Flag::STRING:
          *flag->string_variable() = value;
          break;
      }

      
      if ((flag->type() == Flag::BOOL && value != NULL) ||
          (flag->type() != Flag::BOOL && is_bool) ||
          *endp != '\0') {
        fprintf(stderr, "Error: illegal value for flag %s of type %s\n",
          arg, Type2String(flag->type()));
        return j;
      }

      
      if (remove_flags)
        while (j < i)
          argv[j++] = NULL;
    }
  }

  
  if (remove_flags) {
    int j = 1;
    for (int i = 1; i < *argc; i++) {
      if (argv[i] != NULL)
        argv[j++] = argv[i];
    }
    *argc = j;
  }

  
  return 0;
}

void FlagList::Register(Flag* flag) {
  assert(flag != NULL && strlen(flag->name()) > 0);
  CHECK(!Lookup(flag->name())) << "flag " << flag->name() << " declared twice";
  flag->next_ = list_;
  list_ = flag;
}

#if defined(WEBRTC_WIN)
WindowsCommandLineArguments::WindowsCommandLineArguments() {
  
  LPTSTR command_line = ::GetCommandLine();
   
  LPWSTR *wide_argv = ::CommandLineToArgvW(command_line, &argc_);
  
  argv_ = new char*[argc_];

  
  for(int i = 0; i < argc_; ++i) {
    std::string s = rtc::ToUtf8(wide_argv[i], wcslen(wide_argv[i]));
    char *buffer = new char[s.length() + 1];
    rtc::strcpyn(buffer, s.length() + 1, s.c_str());

    
    argv_[i] = buffer;
  }
  LocalFree(wide_argv);
}

WindowsCommandLineArguments::~WindowsCommandLineArguments() {
  
  for(int i = 0; i < argc_; i++) {
    delete[] argv_[i];
  }

  delete[] argv_;
}
#endif  

}  
