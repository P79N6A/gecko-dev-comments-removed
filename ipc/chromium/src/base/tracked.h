
















#ifndef BASE_TRACKED_H_
#define BASE_TRACKED_H_

#include <string>

#include "base/time.h"

#ifndef NDEBUG
#ifndef TRACK_ALL_TASK_OBJECTS
#define TRACK_ALL_TASK_OBJECTS
#endif   
#endif  

namespace tracked_objects {





class Location {
 public:
  
  
  
  Location(const char* function_name, const char* file_name, int line_number)
      : function_name_(function_name),
        file_name_(file_name),
        line_number_(line_number) { }

  
  Location()
      : function_name_("Unknown"),
        file_name_("Unknown"),
        line_number_(-1) { }

  
  
  
  
  
  
  bool operator < (const Location& other) const {
    if (line_number_ != other.line_number_)
      return line_number_ < other.line_number_;
    if (file_name_ != other.file_name_)
      return file_name_ < other.file_name_;
    return function_name_ < other.function_name_;
  }

  const char* function_name() const { return function_name_; }
  const char* file_name()     const { return file_name_; }
  int line_number()           const { return line_number_; }

  void Write(bool display_filename, bool display_function_name,
             std::string* output) const;

  
  void WriteFunctionName(std::string* output) const;

 private:
  const char* const function_name_;
  const char* const file_name_;
  const int line_number_;
};





#define FROM_HERE tracked_objects::Location(__FUNCTION__, __FILE__, __LINE__)





class Births;

class Tracked {
 public:
  Tracked();
  virtual ~Tracked();

  
  void SetBirthPlace(const Location& from_here);

  
  
  
  void ResetBirthTime();

  bool MissingBirthplace() const;

 private:
#ifdef TRACK_ALL_TASK_OBJECTS

  
  
  Births* tracked_births_;
  
  
  
  base::Time tracked_birth_time_;

#endif  

  DISALLOW_COPY_AND_ASSIGN(Tracked);
};

}  

#endif  
