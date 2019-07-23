



#ifndef CHROME_COMMON_ACCESSIBILITY_TYPES_H_
#define CHROME_COMMON_ACCESSIBILITY_TYPES_H_









class AccessibilityTypes {
 public:
  
  
  
  
  enum Role {
    ROLE_APPLICATION,
    ROLE_BUTTONDROPDOWN,
    ROLE_CLIENT,
    ROLE_GROUPING,
    ROLE_PAGETAB,
    ROLE_PUSHBUTTON,
    ROLE_TEXT,
    ROLE_TOOLBAR
  };

  
  
  
  
  enum State {
    STATE_HASPOPUP,
    STATE_READONLY
  };

 private:
  
  AccessibilityTypes() {}
  ~AccessibilityTypes() {}
};

#endif  
