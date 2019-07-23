



#include "chrome/common/notification_service.h"

#include "base/lazy_instance.h"
#include "base/thread_local.h"

static base::LazyInstance<base::ThreadLocalPointer<NotificationService> >
    lazy_tls_ptr(base::LINKER_INITIALIZED);


NotificationService* NotificationService::current() {
  return lazy_tls_ptr.Pointer()->Get();
}


bool NotificationService::HasKey(const NotificationSourceMap& map,
                                 const NotificationSource& source) {
  return map.find(source.map_key()) != map.end();
}

NotificationService::NotificationService() {
  DCHECK(current() == NULL);
#ifndef NDEBUG
  memset(observer_counts_, 0, sizeof(observer_counts_));
#endif

  lazy_tls_ptr.Pointer()->Set(this);
}

void NotificationService::AddObserver(NotificationObserver* observer,
                                      NotificationType type,
                                      const NotificationSource& source) {
  DCHECK(type.value < NotificationType::NOTIFICATION_TYPE_COUNT);

  
  
  
  
  
  CHECK(observer);

  NotificationObserverList* observer_list;
  if (HasKey(observers_[type.value], source)) {
    observer_list = observers_[type.value][source.map_key()];
  } else {
    observer_list = new NotificationObserverList;
    observers_[type.value][source.map_key()] = observer_list;
  }

  observer_list->AddObserver(observer);
#ifndef NDEBUG
  ++observer_counts_[type.value];
#endif
}

void NotificationService::RemoveObserver(NotificationObserver* observer,
                                         NotificationType type,
                                         const NotificationSource& source) {
  DCHECK(type.value < NotificationType::NOTIFICATION_TYPE_COUNT);
  DCHECK(HasKey(observers_[type.value], source));

  NotificationObserverList* observer_list =
      observers_[type.value][source.map_key()];
  if (observer_list) {
    observer_list->RemoveObserver(observer);
#ifndef NDEBUG
    --observer_counts_[type.value];
#endif
  }

  
}

void NotificationService::Notify(NotificationType type,
                                 const NotificationSource& source,
                                 const NotificationDetails& details) {
  DCHECK(type.value > NotificationType::ALL) <<
      "Allowed for observing, but not posting.";
  DCHECK(type.value < NotificationType::NOTIFICATION_TYPE_COUNT);

  
  

  
  if (HasKey(observers_[NotificationType::ALL], AllSources()) &&
      source != AllSources()) {
    FOR_EACH_OBSERVER(NotificationObserver,
       *observers_[NotificationType::ALL][AllSources().map_key()],
       Observe(type, source, details));
  }

  
  if (HasKey(observers_[NotificationType::ALL], source)) {
    FOR_EACH_OBSERVER(NotificationObserver,
        *observers_[NotificationType::ALL][source.map_key()],
        Observe(type, source, details));
  }

  
  if (HasKey(observers_[type.value], AllSources()) &&
      source != AllSources()) {
    FOR_EACH_OBSERVER(NotificationObserver,
                      *observers_[type.value][AllSources().map_key()],
                      Observe(type, source, details));
  }

  
  if (HasKey(observers_[type.value], source)) {
    FOR_EACH_OBSERVER(NotificationObserver,
                      *observers_[type.value][source.map_key()],
                      Observe(type, source, details));
  }
}


NotificationService::~NotificationService() {
  lazy_tls_ptr.Pointer()->Set(NULL);

#ifndef NDEBUG
  for (int i = 0; i < NotificationType::NOTIFICATION_TYPE_COUNT; i++) {
    if (observer_counts_[i] > 0) {
      
      
      
      LOG(WARNING) << observer_counts_[i] << " notification observer(s) leaked"
          << " of notification type " << i;
    }
  }
#endif

  for (int i = 0; i < NotificationType::NOTIFICATION_TYPE_COUNT; i++) {
    NotificationSourceMap omap = observers_[i];
    for (NotificationSourceMap::iterator it = omap.begin();
         it != omap.end(); ++it) {
      delete it->second;
    }
  }
}

NotificationObserver::~NotificationObserver() {}
