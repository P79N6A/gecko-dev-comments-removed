



#ifndef BASE_CALLBACK_FORWARD_H_
#define BASE_CALLBACK_FORWARD_H_

namespace base {

template <typename Sig>
class Callback;

typedef Callback<void(void)> Closure;

}  

#endif  
