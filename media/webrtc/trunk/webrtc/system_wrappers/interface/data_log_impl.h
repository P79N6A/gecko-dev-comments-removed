















#ifndef WEBRTC_SYSTEM_WRAPPERS_INTERFACE_DATA_LOG_IMPL_H_
#define WEBRTC_SYSTEM_WRAPPERS_INTERFACE_DATA_LOG_IMPL_H_

#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class CriticalSectionWrapper;
class EventWrapper;
class LogTable;
class RWLockWrapper;
class ThreadWrapper;



class Container {
 public:
  virtual ~Container() {}

  virtual void ToString(std::string* container_string) const = 0;
};

template<class T>
class ValueContainer : public Container {
 public:
  explicit ValueContainer(T data) : data_(data) {}

  virtual void ToString(std::string* container_string) const {
    *container_string = "";
    std::stringstream ss;
    ss << data_ << ",";
    ss >> *container_string;
  }

 private:
  T   data_;
};

template<class T>
class MultiValueContainer : public Container {
 public:
  MultiValueContainer(const T* data, int length)
    : data_(data, data + length) {
  }

  virtual void ToString(std::string* container_string) const {
    *container_string = "";
    std::stringstream ss;
    for (size_t i = 0; i < data_.size(); ++i)
      ss << data_[i] << ",";
    *container_string += ss.str();
  }

 private:
  std::vector<T>  data_;
};

class DataLogImpl {
 public:
  ~DataLogImpl();

  
  
  static int CreateLog();

  
  
  static DataLogImpl* StaticInstance();

  
  
  static void ReturnLog();

  
  
  int AddTable(const std::string& table_name);

  
  
  int AddColumn(const std::string& table_name,
                const std::string& column_name,
                int multi_value_length);

  
  
  
  int InsertCell(const std::string& table_name,
                 const std::string& column_name,
                 const Container* value_container);

  
  
  int NextRow(const std::string& table_name);

 private:
  DataLogImpl();

  
  
  int Init();

  
  
  
  void Flush();

  
  static bool Run(void* obj);

  
  
  
  void Process();

  
  void StopThread();

  
  typedef std::map<std::string, LogTable*> TableMap;
  typedef webrtc::scoped_ptr<CriticalSectionWrapper> CritSectScopedPtr;

  static CritSectScopedPtr  crit_sect_;
  static DataLogImpl*       instance_;
  int                       counter_;
  TableMap                  tables_;
  EventWrapper*             flush_event_;
  ThreadWrapper*            file_writer_thread_;
  RWLockWrapper*            tables_lock_;
};

}  

#endif  
