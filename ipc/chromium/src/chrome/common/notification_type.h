



#ifndef CHROME_COMMON_NOTIFICATION_TYPE_H_
#define CHROME_COMMON_NOTIFICATION_TYPE_H_











class NotificationType {
 public:
  enum Type {
    

    
    
    ALL = 0,

    
    
    IDLE,

    
    
    BUSY,

    
    
    
    USER_ACTION,

    

    
    
    
    
    
    
    
    
    
    
    NAV_ENTRY_PENDING,

    
    
    
    
    
    
    NAV_ENTRY_COMMITTED,

    
    
    
    
    
    
    
    
    
    
    
    
    NAV_LIST_PRUNED,

    
    
    
    
    
    
    
    
    NAV_ENTRY_CHANGED,

    

    
    
    
    LOAD_START,

    
    
    
    
    LOAD_STOP,

    
    
    
    
    FRAME_PROVISIONAL_LOAD_START,

    
    
    
    
    LOAD_FROM_MEMORY_CACHE,

    
    
    
    
    FAIL_PROVISIONAL_LOAD_WITH_ERROR,

    
    
    
    
    RESOURCE_RESPONSE_STARTED,

    
    
    
    
    RESOURCE_RESPONSE_COMPLETED,

    
    
    
    
    RESOURCE_RECEIVED_REDIRECT,

    
    
    
    
    
    
    
    
    
    
    SSL_VISIBLE_STATE_CHANGED,

    
    
    
    
    
    
    
    SSL_INTERNAL_STATE_CHANGED,

    
    
    RESOURCE_MESSAGE_FILTER_SHUTDOWN,

    

    
    
    VIEW_REMOVED,

    

    
    
    
    BROWSER_OPENED,

    
    
    
    
    
    
    BROWSER_CLOSED,

    
    
    
    
    ALL_APPWINDOWS_CLOSED,

    
    
    WINDOW_CREATED,

    
    
    WINDOW_CLOSED,

    
    
    INFO_BUBBLE_CREATED,

    

    
    
    
    TAB_PARENTED,

    
    
    
    
    
    TAB_CLOSING,

    
    
    TAB_CLOSED,

    
    
    
    
    
    TAB_CONTENTS_CONNECTED,

    
    
    
    
    
    TAB_CONTENTS_SWAPPED,

    
    
    
    TAB_CONTENTS_DISCONNECTED,

    
    
    
    
    
    TAB_CONTENTS_INFOBAR_ADDED,

    
    
    
    
    
    TAB_CONTENTS_INFOBAR_REMOVED,

    
    
    EXTERNAL_TAB_CREATED,

    
    
    EXTERNAL_TAB_CLOSED,

    
    
    
    
    
    INITIAL_NEW_TAB_UI_LOAD,

    
    
    TAB_CONTENTS_HIDDEN,

    
    
    
    
    TAB_CONTENTS_DESTROYED,

    

    
    
    
    
    CWINDOW_CLOSED,

    
    
    RENDERER_PROCESS_TERMINATED,

    
    
    
    
    
    RENDERER_PROCESS_CLOSED,

    
    
    
    RENDERER_PROCESS_HANG,

    
    
    
    
    RENDERER_PROCESS_IN_SBOX,

    
    
    
    
    RENDER_VIEW_HOST_CHANGED,

    
    
    RENDER_WIDGET_HOST_DESTROYED,

    
    
    DOM_INSPECT_ELEMENT_RESPONSE,

    
    
    DOM_OPERATION_RESPONSE,

    
    
    BOOKMARK_BUBBLE_HIDDEN,

    
    
    
    
    
    FIND_RESULT_AVAILABLE,

    
    
    
    BOOKMARK_BAR_VISIBILITY_PREF_CHANGED,

    
    
    
    
    
    WEB_CACHE_STATS_OBSERVED,

    

    
    
    
    
    CHILD_PROCESS_HOST_CONNECTED,

    
    
    
    
    CHILD_PROCESS_HOST_DISCONNECTED,

    
    
    
    
    CHILD_PROCESS_CRASHED,

    
    
    
    
    
    
    
    
    CHILD_INSTANCE_CREATED,

    
    
    
    
    CHROME_PLUGIN_UNLOADED,

    
    
    
    
    AUTH_NEEDED,

    
    
    
    
    
    AUTH_SUPPLIED,

    

    
    
    
    
    
    HISTORY_CREATED,

    
    
    
    HISTORY_LOADED,

    
    
    
    
    
    
    
    HISTORY_TYPED_URLS_MODIFIED,

    
    
    
    
    HISTORY_URL_VISITED,

    
    
    
    
    HISTORY_URLS_DELETED,

    
    
    
    FAVICON_CHANGED,

    

    
    
    
    
    URLS_STARRED,

    
    
    BOOKMARK_MODEL_LOADED,

    
    
    
    
    SPELLCHECKER_REINITIALIZED,

    
    
    BOOKMARK_BUBBLE_SHOWN,

    

    
    
    
    TEMPLATE_URL_MODEL_LOADED,

    
    
    
    WEB_APP_INSTALL_CHANGED,

    
    PREF_CHANGED,

    
    
    
    
    DEFAULT_REQUEST_CONTEXT_AVAILABLE,

    

    
    
    
    AUTOCOMPLETE_CONTROLLER_RESULT_UPDATED,

    
    
    AUTOCOMPLETE_CONTROLLER_SYNCHRONOUS_MATCHES_AVAILABLE,

    
    
    OMNIBOX_OPENED_URL,

    
    AUTOCOMPLETE_EDIT_DESTROYED,

    
    
    
    GOOGLE_URL_UPDATED,

    

    
    
    
    
    PRINTED_DOCUMENT_UPDATED,

    
    
    
    PRINT_JOB_EVENT,

    

    
    
    URL_REQUEST_CONTEXT_RELEASED,

    
    
    
    SESSION_END,

    

    PERSONALIZATION,
    PERSONALIZATION_CREATED,

    

    
    
    USER_SCRIPTS_LOADED,

    

    
    EXTENSIONS_LOADED,

    
    EXTENSION_INSTALLED,

    

    
    RENDER_VIEW_HOST_DELETED,

    
    
    
    NOTIFICATION_TYPE_COUNT
  };

  NotificationType(Type v) : value(v) {}

  bool operator==(NotificationType t) const { return value == t.value; }
  bool operator!=(NotificationType t) const { return value != t.value; }

  
  bool operator==(Type v) const { return value == v; }
  bool operator!=(Type v) const { return value != v; }

  Type value;
};

inline bool operator==(NotificationType::Type a, NotificationType b) {
  return a == b.value;
}
inline bool operator!=(NotificationType::Type a, NotificationType b) {
  return a != b.value;
}

#endif
