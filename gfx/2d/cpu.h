



#ifndef BASE_CPU_H_
#define BASE_CPU_H_

#include <string>

namespace base {


class CPU {
 public:
  
  CPU();

  
  const std::string& vendor_name() const { return cpu_vendor_; }
  int stepping() const { return stepping_; }
  int model() const { return model_; }
  int family() const { return family_; }
  int type() const { return type_; }
  int extended_model() const { return ext_model_; }
  int extended_family() const { return ext_family_; }

 private:
  
  void Initialize();

  int type_;  
  int family_;  
  int model_;  
  int stepping_;  
  int ext_model_;
  int ext_family_;
  std::string cpu_vendor_;
};

}  

#endif  
