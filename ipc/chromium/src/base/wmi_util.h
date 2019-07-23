


















#ifndef BASE_WMI_UTIL_H__
#define BASE_WMI_UTIL_H__

#include <string>
#include <wbemidl.h>

class WMIUtil {
 public:
  
  
  
  
  
  
  
  static bool CreateLocalConnection(bool set_blanket,
                                    IWbemServices** wmi_services);

  
  
  
  
  
  
  static bool CreateClassMethodObject(IWbemServices* wmi_services,
                                      const std::wstring& class_name,
                                      const std::wstring& method_name,
                                      IWbemClassObject** class_instance);

  
  
  
  static bool SetParameter(IWbemClassObject* class_method,
                           const std::wstring& parameter_name,
                           VARIANT* parameter);
};



class WMIProcessUtil {
 public:
  
  
  
  
  
  
  
  
  
  static bool Launch(const std::wstring& command_line, int* process_id);
};

#endif  
