






































#import <string>

class Widget {
 public:
  Widget(int number, const std::string& name);
  ~Widget();

  
  float GetFloatValue() const;
  int GetIntValue() const;

  
  std::string GetStringValue() const;
  void GetCharPtrValue(char* buffer, size_t max_size) const;

 private:
  
  float number_;
  std::string name_;
};
