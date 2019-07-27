

































#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/stubs/common.h>

namespace google {
namespace protobuf {
namespace io {

Printer::Printer(ZeroCopyOutputStream* output, char variable_delimiter)
  : variable_delimiter_(variable_delimiter),
    output_(output),
    buffer_(NULL),
    buffer_size_(0),
    at_start_of_line_(true),
    failed_(false) {
}

Printer::~Printer() {
  
  if (buffer_size_ > 0 && !failed_) {
    output_->BackUp(buffer_size_);
  }
}

void Printer::Print(const map<string, string>& variables, const char* text) {
  int size = strlen(text);
  int pos = 0;  

  for (int i = 0; i < size; i++) {
    if (text[i] == '\n') {
      
      
      WriteRaw(text + pos, i - pos + 1);
      pos = i + 1;

      
      
      at_start_of_line_ = true;

    } else if (text[i] == variable_delimiter_) {
      

      
      WriteRaw(text + pos, i - pos);
      pos = i + 1;

      
      const char* end = strchr(text + pos, variable_delimiter_);
      if (end == NULL) {
        GOOGLE_LOG(DFATAL) << " Unclosed variable name.";
        end = text + pos;
      }
      int endpos = end - text;

      string varname(text + pos, endpos - pos);
      if (varname.empty()) {
        
        WriteRaw(&variable_delimiter_, 1);
      } else {
        
        map<string, string>::const_iterator iter = variables.find(varname);
        if (iter == variables.end()) {
          GOOGLE_LOG(DFATAL) << " Undefined variable: " << varname;
        } else {
          WriteRaw(iter->second.data(), iter->second.size());
        }
      }

      
      i = endpos;
      pos = endpos + 1;
    }
  }

  
  WriteRaw(text + pos, size - pos);
}

void Printer::Print(const char* text) {
  static map<string, string> empty;
  Print(empty, text);
}

void Printer::Print(const char* text,
                    const char* variable, const string& value) {
  map<string, string> vars;
  vars[variable] = value;
  Print(vars, text);
}

void Printer::Print(const char* text,
                    const char* variable1, const string& value1,
                    const char* variable2, const string& value2) {
  map<string, string> vars;
  vars[variable1] = value1;
  vars[variable2] = value2;
  Print(vars, text);
}

void Printer::Print(const char* text,
                    const char* variable1, const string& value1,
                    const char* variable2, const string& value2,
                    const char* variable3, const string& value3) {
  map<string, string> vars;
  vars[variable1] = value1;
  vars[variable2] = value2;
  vars[variable3] = value3;
  Print(vars, text);
}

void Printer::Indent() {
  indent_ += "  ";
}

void Printer::Outdent() {
  if (indent_.empty()) {
    GOOGLE_LOG(DFATAL) << " Outdent() without matching Indent().";
    return;
  }

  indent_.resize(indent_.size() - 2);
}

void Printer::PrintRaw(const string& data) {
  WriteRaw(data.data(), data.size());
}

void Printer::PrintRaw(const char* data) {
  if (failed_) return;
  WriteRaw(data, strlen(data));
}

void Printer::WriteRaw(const char* data, int size) {
  if (failed_) return;
  if (size == 0) return;

  if (at_start_of_line_ && (size > 0) && (data[0] != '\n')) {
    
    at_start_of_line_ = false;
    WriteRaw(indent_.data(), indent_.size());
    if (failed_) return;
  }

  while (size > buffer_size_) {
    
    
    memcpy(buffer_, data, buffer_size_);
    data += buffer_size_;
    size -= buffer_size_;
    void* void_buffer;
    failed_ = !output_->Next(&void_buffer, &buffer_size_);
    if (failed_) return;
    buffer_ = reinterpret_cast<char*>(void_buffer);
  }

  
  memcpy(buffer_, data, size);
  buffer_ += size;
  buffer_size_ -= size;
}

}  
}  
}  
