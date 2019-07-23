







































#ifndef _ACCESSIBLE_ACTION_H
#define _ACCESSIBLE_ACTION_H

#include "nsISupports.h"

#include "AccessibleAction.h"

class CAccessibleAction: public nsISupports,
                         public IAccessibleAction
{
public:

  
  STDMETHODIMP QueryInterface(REFIID, void**);

  
  virtual HRESULT STDMETHODCALLTYPE nActions(
       long *nActions);

  virtual HRESULT STDMETHODCALLTYPE doAction(
       long actionIndex);

  virtual  HRESULT STDMETHODCALLTYPE get_description(
       long actionIndex,
       BSTR *description);

  virtual  HRESULT STDMETHODCALLTYPE get_keyBinding(
       long actionIndex,
       long nMaxBinding,
       BSTR **keyBinding,
       long *nBinding);

  virtual  HRESULT STDMETHODCALLTYPE get_name(
       long actionIndex,
       BSTR *name);

  virtual  HRESULT STDMETHODCALLTYPE get_localizedName(
       long actionIndex,
       BSTR *localizedName);

};


#define FORWARD_IACCESSIBLEACTION(Class)                                       \
virtual HRESULT STDMETHODCALLTYPE nActions(long *nActions)                     \
{                                                                              \
  return Class::nActions(nActions);                                            \
}                                                                              \
                                                                               \
virtual HRESULT STDMETHODCALLTYPE doAction(long actionIndex)                   \
{                                                                              \
  return Class::doAction(actionIndex);                                         \
}                                                                              \
                                                                               \
virtual HRESULT STDMETHODCALLTYPE get_description(long actionIndex,            \
                                                  BSTR *description)           \
{                                                                              \
  return Class::get_description(actionIndex, description);                     \
}                                                                              \
                                                                               \
virtual HRESULT STDMETHODCALLTYPE get_keyBinding(long actionIndex,             \
                                                 long nMaxBinding,             \
                                                 BSTR **keyBinding,            \
                                                 long *nBinding)               \
{                                                                              \
  return Class::get_keyBinding(actionIndex, nMaxBinding, keyBinding, nBinding);\
}                                                                              \
                                                                               \
virtual HRESULT STDMETHODCALLTYPE get_name(long actionIndex, BSTR *name)       \
{                                                                              \
  return Class::get_name(actionIndex, name);                                   \
}                                                                              \
                                                                               \
virtual HRESULT STDMETHODCALLTYPE get_localizedName(long actionIndex,          \
                                                    BSTR *localizedName)       \
{                                                                              \
  return Class::get_localizedName(actionIndex, localizedName);                 \
}                                                                              \
                                                                               \

#endif

