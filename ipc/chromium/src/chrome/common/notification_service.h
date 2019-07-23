







#ifndef CHROME_COMMON_NOTIFICATION_SERVICE_H_
#define CHROME_COMMON_NOTIFICATION_SERVICE_H_

#include <map>

#include "base/observer_list.h"
#include "chrome/common/notification_details.h"
#include "chrome/common/notification_observer.h"
#include "chrome/common/notification_source.h"
#include "chrome/common/notification_type.h"

class NotificationObserver;

class NotificationService {
 public:
  
  
  static NotificationService* current();

  
  
  NotificationService();
  ~NotificationService();

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  void AddObserver(NotificationObserver* observer,
                   NotificationType type, const NotificationSource& source);

  
  
  
  void RemoveObserver(NotificationObserver* observer,
                      NotificationType type, const NotificationSource& source);

  
  
  
  
  
  
  
  
  void Notify(NotificationType type,
              const NotificationSource& source,
              const NotificationDetails& details);

  
  
  static Source<void> AllSources() { return Source<void>(NULL); }

  
  
  static Details<void> NoDetails() { return Details<void>(NULL); }

 private:
  typedef ObserverList<NotificationObserver> NotificationObserverList;
  typedef std::map<uintptr_t, NotificationObserverList*> NotificationSourceMap;

  
  
  static bool HasKey(const NotificationSourceMap& map,
                     const NotificationSource& source);

  
  
  
  NotificationSourceMap observers_[NotificationType::NOTIFICATION_TYPE_COUNT];

#ifndef NDEBUG
  
  
  int observer_counts_[NotificationType::NOTIFICATION_TYPE_COUNT];
#endif

  DISALLOW_COPY_AND_ASSIGN(NotificationService);
};

#endif  
