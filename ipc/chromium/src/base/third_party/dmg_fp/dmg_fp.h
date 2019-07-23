



#ifndef THIRD_PARTY_DMG_FP_H_
#define THIRD_PARTY_DMG_FP_H_

namespace dmg_fp {





double strtod(const char* s00, char** se);



char* dtoa(double d, int mode, int ndigits,
           int* decpt, int* sign, char** rve);


void freedtoa(char* s);



char* g_fmt(char* b, double x);

}  

#endif  
