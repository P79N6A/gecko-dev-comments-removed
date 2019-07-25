




#ifndef _a11yGeneric_H_
#define _a11yGeneric_H_

#include "nsThreadUtils.h"






#define NS_INTERFACE_MAP_STATIC_AMBIGUOUS(_class)                              \
  if (aIID.Equals(NS_GET_IID(_class))) {                                       \
    NS_ADDREF(this);                                                           \
    *aInstancePtr = this;                                                      \
    return NS_OK;                                                              \
  } else

#define NS_ENSURE_A11Y_SUCCESS(res, ret)                                       \
  PR_BEGIN_MACRO                                                               \
    nsresult __rv = res; /* Don't evaluate |res| more than once */             \
    if (NS_FAILED(__rv)) {                                                     \
      NS_ENSURE_SUCCESS_BODY(res, ret)                                         \
      return ret;                                                              \
    }                                                                          \
    if (__rv == NS_OK_DEFUNCT_OBJECT)                                          \
      return ret;                                                              \
  PR_END_MACRO





















#define NS_DECL_RUNNABLEMETHOD_HELPER(ClassType, Method)                       \
  void Revoke()                                                                \
  {                                                                            \
    NS_IF_RELEASE(mObj);                                                       \
  }                                                                            \
                                                                               \
protected:                                                                     \
  virtual ~nsRunnableMethod_##Method()                                         \
  {                                                                            \
    NS_IF_RELEASE(mObj);                                                       \
  }                                                                            \
                                                                               \
private:                                                                       \
  ClassType *mObj;                                                             \


#define NS_DECL_RUNNABLEMETHOD(ClassType, Method)                              \
class nsRunnableMethod_##Method : public nsRunnable                            \
{                                                                              \
public:                                                                        \
  nsRunnableMethod_##Method(ClassType *aObj) : mObj(aObj)                      \
  {                                                                            \
    NS_IF_ADDREF(mObj);                                                        \
  }                                                                            \
                                                                               \
  NS_IMETHODIMP Run()                                                          \
  {                                                                            \
    if (!mObj)                                                                 \
      return NS_OK;                                                            \
    (mObj-> Method)();                                                         \
    return NS_OK;                                                              \
  }                                                                            \
                                                                               \
  NS_DECL_RUNNABLEMETHOD_HELPER(ClassType, Method)                             \
                                                                               \
};

#define NS_DECL_RUNNABLEMETHOD_ARG1(ClassType, Method, Arg1Type)               \
class nsRunnableMethod_##Method : public nsRunnable                            \
{                                                                              \
public:                                                                        \
  nsRunnableMethod_##Method(ClassType *aObj, Arg1Type aArg1) :                 \
    mObj(aObj), mArg1(aArg1)                                                   \
  {                                                                            \
    NS_IF_ADDREF(mObj);                                                        \
  }                                                                            \
                                                                               \
  NS_IMETHODIMP Run()                                                          \
  {                                                                            \
    if (!mObj)                                                                 \
      return NS_OK;                                                            \
    (mObj-> Method)(mArg1);                                                    \
    return NS_OK;                                                              \
  }                                                                            \
                                                                               \
  NS_DECL_RUNNABLEMETHOD_HELPER(ClassType, Method)                             \
  Arg1Type mArg1;                                                              \
};

#define NS_DECL_RUNNABLEMETHOD_ARG2(ClassType, Method, Arg1Type, Arg2Type)     \
class nsRunnableMethod_##Method : public nsRunnable                            \
{                                                                              \
public:                                                                        \
                                                                               \
  nsRunnableMethod_##Method(ClassType *aObj,                                   \
                            Arg1Type aArg1, Arg2Type aArg2) :                  \
    mObj(aObj), mArg1(aArg1), mArg2(aArg2)                                     \
  {                                                                            \
    NS_IF_ADDREF(mObj);                                                        \
  }                                                                            \
                                                                               \
  NS_IMETHODIMP Run()                                                          \
  {                                                                            \
    if (!mObj)                                                                 \
      return NS_OK;                                                            \
    (mObj-> Method)(mArg1, mArg2);                                             \
    return NS_OK;                                                              \
  }                                                                            \
                                                                               \
  NS_DECL_RUNNABLEMETHOD_HELPER(ClassType, Method)                             \
  Arg1Type mArg1;                                                              \
  Arg2Type mArg2;                                                              \
};

#define NS_DISPATCH_RUNNABLEMETHOD(Method, Obj)                                \
{                                                                              \
  nsCOMPtr<nsIRunnable> runnable =                                             \
    new nsRunnableMethod_##Method(Obj);                                        \
  if (runnable)                                                                \
    NS_DispatchToMainThread(runnable);                                         \
}

#define NS_DISPATCH_RUNNABLEMETHOD_ARG1(Method, Obj, Arg1)                     \
{                                                                              \
  nsCOMPtr<nsIRunnable> runnable =                                             \
    new nsRunnableMethod_##Method(Obj, Arg1);                                  \
  if (runnable)                                                                \
    NS_DispatchToMainThread(runnable);                                         \
}

#define NS_DISPATCH_RUNNABLEMETHOD_ARG2(Method, Obj, Arg1, Arg2)               \
{                                                                              \
  nsCOMPtr<nsIRunnable> runnable =                                             \
    new nsRunnableMethod_##Method(Obj, Arg1, Arg2);                            \
  if (runnable)                                                                \
    NS_DispatchToMainThread(runnable);                                         \
}

#endif
