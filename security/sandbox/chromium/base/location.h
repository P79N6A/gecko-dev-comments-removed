



#ifndef BASE_LOCATION_H_
#define BASE_LOCATION_H_

#include <string>

#include "base/base_export.h"
#include "base/basictypes.h"

namespace tracked_objects {



class BASE_EXPORT Location {
 public:
  
  
  
  Location(const char* function_name,
           const char* file_name,
           int line_number,
           const void* program_counter);

  
  Location();

  
  
  
  
  
  
  bool operator < (const Location& other) const {
    if (line_number_ != other.line_number_)
      return line_number_ < other.line_number_;
    if (file_name_ != other.file_name_)
      return file_name_ < other.file_name_;
    return function_name_ < other.function_name_;
  }

  const char* function_name()   const { return function_name_; }
  const char* file_name()       const { return file_name_; }
  int line_number()             const { return line_number_; }
  const void* program_counter() const { return program_counter_; }

  std::string ToString() const;

  
  
  
  
  void Write(bool display_filename, bool display_function_name,
             std::string* output) const;

  
  void WriteFunctionName(std::string* output) const;

 private:
  const char* function_name_;
  const char* file_name_;
  int line_number_;
  const void* program_counter_;
};



struct BASE_EXPORT LocationSnapshot {
  
  LocationSnapshot();
  explicit LocationSnapshot(const tracked_objects::Location& location);
  ~LocationSnapshot();

  std::string file_name;
  std::string function_name;
  int line_number;
};

BASE_EXPORT const void* GetProgramCounter();


#define FROM_HERE FROM_HERE_WITH_EXPLICIT_FUNCTION(__FUNCTION__)

#define FROM_HERE_WITH_EXPLICIT_FUNCTION(function_name)                        \
    ::tracked_objects::Location(function_name,                                 \
                                __FILE__,                                      \
                                __LINE__,                                      \
                                ::tracked_objects::GetProgramCounter())

}  

#endif  
