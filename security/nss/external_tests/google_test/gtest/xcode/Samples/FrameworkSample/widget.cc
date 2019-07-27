




































#include "widget.h"

Widget::Widget(int number, const std::string& name)
    : number_(number),
      name_(name) {}

Widget::~Widget() {}

float Widget::GetFloatValue() const {
  return number_;
}

int Widget::GetIntValue() const {
  return static_cast<int>(number_);
}

std::string Widget::GetStringValue() const {
  return name_;
}

void Widget::GetCharPtrValue(char* buffer, size_t max_size) const {
  
  strncpy(buffer, name_.c_str(), max_size-1);
  buffer[max_size-1] = '\0';
  return;
}
