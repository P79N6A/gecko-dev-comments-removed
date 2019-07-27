









#ifndef BASE_WIN_REGISTRY_H_
#define BASE_WIN_REGISTRY_H_

namespace base {
namespace win {

class BASE_EXPORT RegKey {
 public:
  RegKey(HKEY rootkey, const wchar_t* subkey, REGSAM access) {}
  ~RegKey() {}

  LONG ReadValue(const wchar_t* name, std::wstring* out_value) const
  {
    return 0;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(RegKey);
};

}  
}  

#endif  
