



#ifndef CHROME_COMMON_PAGE_ACTION_H_
#define CHROME_COMMON_PAGE_ACTION_H_

#include <string>
#include <map>

#include "base/file_path.h"
#include "googleurl/src/gurl.h"

class PageAction {
 public:
  PageAction();
  virtual ~PageAction();

  typedef enum {
    PERMANENT = 0,
    TAB = 1,
  } PageActionType;

  std::string id() const { return id_; }
  void set_id(std::string id) { id_ = id; }

  PageActionType type() const { return type_; }
  void set_type(PageActionType type) { type_ = type; }

  std::string extension_id() const { return extension_id_; }
  void set_extension_id(std::string extension_id) {
    extension_id_ = extension_id;
  }

  std::string name() const { return name_; }
  void set_name(const std::string& name) { name_ = name; }

  FilePath icon_path() const { return icon_path_; }
  void set_icon_path(const FilePath& icon_path) {
    icon_path_ = icon_path;
  }

  std::string tooltip() const { return tooltip_; }
  void set_tooltip(const std::string& tooltip) {
    tooltip_ = tooltip;
  }

 private:
  
  std::string id_;

  
  PageActionType type_;

  
  
  std::string extension_id_;

  
  std::string name_;

  
  FilePath icon_path_;

  
  std::string tooltip_;
};

typedef std::map<std::string, PageAction*> PageActionMap;

#endif  
