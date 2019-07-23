



#ifndef CHROME_COMMON_BINDINGS_POLICY_H__
#define CHROME_COMMON_BINDINGS_POLICY_H__



class BindingsPolicy {
 public:
  enum {
    
    
    DOM_UI = 1 << 0,
    
    
    
    
    DOM_AUTOMATION = 1 << 1,
    
    EXTERNAL_HOST = 1 << 2,
    
    
    EXTENSION = 1 << 3,
  };

  static bool is_dom_ui_enabled(int flags) {
    return (flags & DOM_UI) != 0;
  }
  static bool is_dom_automation_enabled(int flags) {
    return (flags & DOM_AUTOMATION) != 0;
  }
  static bool is_external_host_enabled(int flags) {
    return (flags & EXTERNAL_HOST) != 0;
  }
  static bool is_extension_enabled(int flags) {
    return (flags & EXTENSION) != 0;
  }
};

#endif
