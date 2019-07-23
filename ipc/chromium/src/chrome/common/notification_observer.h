



#ifndef CHROME_COMMON_NOTIFICATION_OBSERVER_H_
#define CHROME_COMMON_NOTIFICATION_OBSERVER_H_

class NotificationDetails;
class NotificationSource;
class NotificationType;



class NotificationObserver {
 public:
  virtual ~NotificationObserver();

  virtual void Observe(NotificationType type,
                       const NotificationSource& source,
                       const NotificationDetails& details) = 0;
};

#endif  
