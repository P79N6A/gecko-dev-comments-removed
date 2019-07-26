





























#ifndef WEBRTC_SYSTEM_WRAPPERS_INTERFACE_DATA_LOG_H_
#define WEBRTC_SYSTEM_WRAPPERS_INTERFACE_DATA_LOG_H_

#include <string>

#include "webrtc/system_wrappers/interface/data_log_impl.h"

namespace webrtc {

class DataLog {
 public:
  
  
  
  
  
  static int CreateLog();

  
  
  
  static void ReturnLog();

  
  
  static std::string Combine(const std::string& table_name, int table_id);

  
  
  
  static int AddTable(const std::string& table_name);

  
  
  
  static int AddColumn(const std::string& table_name,
                       const std::string& column_name,
                       int multi_value_length);

  
  
  
  
  
  
  
  template<class T>
  static int InsertCell(const std::string& table_name,
                        const std::string& column_name,
                        T value) {
    DataLogImpl* data_log = DataLogImpl::StaticInstance();
    if (data_log == NULL)
      return -1;
    return data_log->InsertCell(
             table_name,
             column_name,
             new ValueContainer<T>(value));
  }

  
  
  
  
  
  
  
  template<class T>
  static int InsertCell(const std::string& table_name,
                        const std::string& column_name,
                        const T* array,
                        int length) {
    DataLogImpl* data_log = DataLogImpl::StaticInstance();
    if (data_log == NULL)
      return -1;
    return data_log->InsertCell(
             table_name,
             column_name,
             new MultiValueContainer<T>(array, length));
  }

  
  
  
  static int NextRow(const std::string& table_name);
};

}  

#endif  
