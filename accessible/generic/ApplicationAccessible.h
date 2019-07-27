






#ifndef mozilla_a11y_ApplicationAccessible_h__
#define mozilla_a11y_ApplicationAccessible_h__

#include "AccessibleWrap.h"
#include "xpcAccessibleApplication.h"

#include "nsIMutableArray.h"
#include "nsIXULAppInfo.h"

namespace mozilla {
namespace a11y {











class ApplicationAccessible : public AccessibleWrap,
                              public xpcAccessibleApplication
{
public:

  ApplicationAccessible();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  virtual void Shutdown();
  virtual nsIntRect Bounds() const MOZ_OVERRIDE;
  virtual already_AddRefed<nsIPersistentProperties> NativeAttributes() MOZ_OVERRIDE;
  virtual GroupPos GroupPosition();
  virtual ENameValueFlag Name(nsString& aName);
  virtual void ApplyARIAState(uint64_t* aState) const;
  virtual void Description(nsString& aDescription);
  virtual void Value(nsString& aValue);
  virtual mozilla::a11y::role NativeRole() MOZ_OVERRIDE;
  virtual uint64_t State() MOZ_OVERRIDE;
  virtual uint64_t NativeState() MOZ_OVERRIDE;
  virtual Relation RelationByType(RelationType aType) MOZ_OVERRIDE;

  virtual Accessible* ChildAtPoint(int32_t aX, int32_t aY,
                                   EWhichChildAtPoint aWhichChild);
  virtual Accessible* FocusedChild();

  virtual void InvalidateChildren();

  
  virtual KeyBinding AccessKey() const;

  
  void AppName(nsAString& aName) const
  {
    nsAutoCString cname;
    mAppInfo->GetName(cname);
    AppendUTF8toUTF16(cname, aName);
  }

  void AppVersion(nsAString& aVersion) const
  {
    nsAutoCString cversion;
    mAppInfo->GetVersion(cversion);
    AppendUTF8toUTF16(cversion, aVersion);
  }

  void PlatformName(nsAString& aName) const
  {
    aName.AssignLiteral("Gecko");
  }

  void PlatformVersion(nsAString& aVersion) const
  {
    nsAutoCString cversion;
    mAppInfo->GetPlatformVersion(cversion);
    AppendUTF8toUTF16(cversion, aVersion);
  }

protected:
  virtual ~ApplicationAccessible() {}

  
  virtual void CacheChildren();
  virtual Accessible* GetSiblingAtOffset(int32_t aOffset,
                                         nsresult *aError = nullptr) const;

private:
  nsCOMPtr<nsIXULAppInfo> mAppInfo;
};

} 
} 

#endif

