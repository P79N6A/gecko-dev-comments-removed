



#ifndef SANDBOX_SRC_POLICY_PARAMS_H__
#define SANDBOX_SRC_POLICY_PARAMS_H__

#include "sandbox/win/src/policy_engine_params.h"

namespace sandbox {

class ParameterSet;



#define POLPARAMS_BEGIN(type) class type { public: enum Args {
#define POLPARAM(arg) arg,
#define POLPARAMS_END(type) PolParamLast }; }; \
  typedef sandbox::ParameterSet type##Array [type::PolParamLast];


POLPARAMS_BEGIN(OpenFile)
  POLPARAM(NAME)
  POLPARAM(BROKER)   
  POLPARAM(ACCESS)
  POLPARAM(OPTIONS)
POLPARAMS_END(OpenFile)


POLPARAMS_BEGIN(FileName)
  POLPARAM(NAME)
  POLPARAM(BROKER)   
POLPARAMS_END(FileName)

COMPILE_ASSERT(OpenFile::NAME == static_cast<int>(FileName::NAME),
               to_simplify_fs_policies);
COMPILE_ASSERT(OpenFile::BROKER == static_cast<int>(FileName::BROKER),
               to_simplify_fs_policies);


POLPARAMS_BEGIN(NameBased)
  POLPARAM(NAME)
POLPARAMS_END(NameBased)


POLPARAMS_BEGIN(OpenEventParams)
  POLPARAM(NAME)
  POLPARAM(ACCESS)
POLPARAMS_END(OpenEventParams)


POLPARAMS_BEGIN(OpenKey)
  POLPARAM(NAME)
  POLPARAM(ACCESS)
POLPARAMS_END(OpenKey)


POLPARAMS_BEGIN(HandleTarget)
  POLPARAM(NAME)
  POLPARAM(TARGET)
POLPARAMS_END(HandleTarget)


}  

#endif  
