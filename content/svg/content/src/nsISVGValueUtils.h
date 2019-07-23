





































#ifndef __NS_ISVGVALUEUTILS_H__
#define __NS_ISVGVALUEUTILS_H__






#define NS_ADD_SVGVALUE_OBSERVER(value)                               \
  PR_BEGIN_MACRO                                                      \
  {                                                                   \
    nsCOMPtr<nsISVGValue> v = do_QueryInterface(value);               \
    NS_ASSERTION(v, "can't find nsISVGValue interface on " #value );  \
    if (v)                                                            \
      v->AddObserver(this);                                           \
  }                                                                   \
  PR_END_MACRO






#define NS_REMOVE_SVGVALUE_OBSERVER(value)                            \
  PR_BEGIN_MACRO                                                      \
  {                                                                   \
    nsCOMPtr<nsISVGValue> v = do_QueryInterface(value);               \
    NS_ASSERTION(v, "can't find nsISVGValue interface on " #value );  \
    if (v)                                                            \
      v->RemoveObserver(this);                                        \
  }                                                                   \
  PR_END_MACRO

  

#endif 
