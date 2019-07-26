#include <stdio.h>

#include <algorithm>
#ifndef mozilla_algorithm_h
#  error "failed to wrap <algorithm>"
#endif

#include <vector>
#ifndef mozilla_vector_h
#  error "failed to wrap <vector>"
#endif



#ifdef _MSC_VER
   
   
#  pragma warning( disable : 4530 )
#  define TRY       try
#  define CATCH(e)  catch (e)
#else
#  define TRY
#  define CATCH(e)  if (0)
#endif

int main() {
    std::vector<int> v;
    int rv = 1;

    TRY {
      

      
      
      rv += v.at(1) ? 1 : 2;
    } CATCH(const std::out_of_range&) {
      fputs("TEST-FAIL | TestSTLWrappers.cpp | caught an exception?\n",
            stderr);
      return 1;
    }

    fputs("TEST-FAIL | TestSTLWrappers.cpp | didn't abort()?\n",
          stderr);
    return rv;
}
