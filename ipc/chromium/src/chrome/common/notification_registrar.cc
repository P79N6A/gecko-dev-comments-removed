



#include "chrome/common/notification_registrar.h"

#include <algorithm>

#include "base/logging.h"
#include "chrome/common/notification_service.h"

struct NotificationRegistrar::Record {
  bool operator==(const Record& other) const;

  NotificationObserver* observer;
  NotificationType type;
  NotificationSource source;
};

bool NotificationRegistrar::Record::operator==(const Record& other) const {
  return observer == other.observer &&
         type == other.type &&
         source == other.source;
}

NotificationRegistrar::NotificationRegistrar() {
}

NotificationRegistrar::~NotificationRegistrar() {
  RemoveAll();
}

void NotificationRegistrar::Add(NotificationObserver* observer,
                                NotificationType type,
                                const NotificationSource& source) {
  Record record = { observer, type, source };
  DCHECK(std::find(registered_.begin(), registered_.end(), record) ==
         registered_.end()) << "Duplicate registration.";
  registered_.push_back(record);

  NotificationService::current()->AddObserver(observer, type, source);
}

void NotificationRegistrar::Remove(NotificationObserver* observer,
                                   NotificationType type,
                                   const NotificationSource& source) {
  Record record = { observer, type, source };
  RecordVector::iterator found = std::find(
      registered_.begin(), registered_.end(), record);
  if (found != registered_.end()) {
    registered_.erase(found);
  } else {
    
    
    
    
    NOTREACHED();
  }

  NotificationService::current()->RemoveObserver(observer, type, source);
}

void NotificationRegistrar::RemoveAll() {
  NotificationService* service = NotificationService::current();
  for (size_t i = 0; i < registered_.size(); i++) {
    service->RemoveObserver(registered_[i].observer,
                            registered_[i].type,
                            registered_[i].source);
  }
  registered_.clear();
}
