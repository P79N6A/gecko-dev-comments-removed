



#ifndef CHROME_COMMON_NOTIFICATION_REGISTRAR_H_
#define CHROME_COMMON_NOTIFICATION_REGISTRAR_H_

#include <vector>

#include "base/basictypes.h"
#include "chrome/common/notification_observer.h"








class NotificationRegistrar {
 public:
  
  
  NotificationRegistrar();
  ~NotificationRegistrar();

  
  void Add(NotificationObserver* observer,
           NotificationType type,
           const NotificationSource& source);
  void Remove(NotificationObserver* observer,
              NotificationType type,
              const NotificationSource& source);

  
  void RemoveAll();

 private:
  struct Record;

  
  
  
  
  typedef std::vector<Record> RecordVector;

  
  RecordVector registered_;

  DISALLOW_COPY_AND_ASSIGN(NotificationRegistrar);
};

#endif  
