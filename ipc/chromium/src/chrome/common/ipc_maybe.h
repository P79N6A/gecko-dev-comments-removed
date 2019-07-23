#ifndef CHROME_COMMON_MAYBE_H_
#define CHROME_COMMON_MAYBE_H_

namespace IPC {






template<typename A>
struct Maybe {
  bool valid;
  A value;
};

}  

#endif  
