





#ifndef mozilla_a11y_ia2Accessible_h_
#define mozilla_a11y_ia2Accessible_h_

#include "nsISupports.h"

#include "Accessible2_2.h"

namespace mozilla {
namespace a11y {
class Attribute;

class ia2Accessible : public IAccessible2_2
{
public:

  
  STDMETHODIMP QueryInterface(REFIID, void**);

  
  virtual  HRESULT STDMETHODCALLTYPE get_nRelations(
     long* nRelations);

  virtual  HRESULT STDMETHODCALLTYPE get_relation(
     long relationIndex,
     IAccessibleRelation** relation);

  virtual  HRESULT STDMETHODCALLTYPE get_relations(
     long maxRelations,
     IAccessibleRelation** relation,
     long* nRelations);

  virtual HRESULT STDMETHODCALLTYPE role(
     long* role);

  virtual HRESULT STDMETHODCALLTYPE scrollTo(
     enum IA2ScrollType scrollType);

  virtual HRESULT STDMETHODCALLTYPE scrollToPoint(
     enum IA2CoordinateType coordinateType,
     long x,
     long y);

  virtual  HRESULT STDMETHODCALLTYPE get_groupPosition(
     long* groupLevel,
     long* similarItemsInGroup,
     long* positionInGroup);

  virtual  HRESULT STDMETHODCALLTYPE get_states(
     AccessibleStates* states);

  virtual  HRESULT STDMETHODCALLTYPE get_extendedRole(
     BSTR* extendedRole);

  virtual  HRESULT STDMETHODCALLTYPE get_localizedExtendedRole(
     BSTR* localizedExtendedRole);

  virtual  HRESULT STDMETHODCALLTYPE get_nExtendedStates(
     long* nExtendedStates);

  virtual  HRESULT STDMETHODCALLTYPE get_extendedStates(
     long maxExtendedStates,
     BSTR** extendedStates,
     long* nExtendedStates);

  virtual  HRESULT STDMETHODCALLTYPE get_localizedExtendedStates(
     long maxLocalizedExtendedStates,
     BSTR** localizedExtendedStates,
     long* nLocalizedExtendedStates);

  virtual  HRESULT STDMETHODCALLTYPE get_uniqueID(
     long* uniqueID);

  virtual  HRESULT STDMETHODCALLTYPE get_windowHandle(
     HWND* windowHandle);

  virtual  HRESULT STDMETHODCALLTYPE get_indexInParent(
     long* indexInParent);

  virtual  HRESULT STDMETHODCALLTYPE get_locale(
     IA2Locale* locale);

  virtual  HRESULT STDMETHODCALLTYPE get_attributes(
     BSTR* attributes);

  
  virtual  HRESULT STDMETHODCALLTYPE get_attribute(
     BSTR name,
     VARIANT* attribute);

  virtual  HRESULT STDMETHODCALLTYPE get_accessibleWithCaret(
     IUnknown** accessible,
     long* caretOffset);

  virtual  HRESULT STDMETHODCALLTYPE get_relationTargetsOfType(
     BSTR type,
     long maxTargets,
     IUnknown*** targets,
     long* nTargets
  );

  
  static HRESULT ConvertToIA2Attributes(nsIPersistentProperties* aAttributes,
                                        BSTR* aIA2Attributes);
  static HRESULT ConvertToIA2Attributes(nsTArray<Attribute>* aAttributes,
                                        BSTR* aIA2Attributes);
};

} 
} 

#endif
