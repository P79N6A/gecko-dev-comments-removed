





#include "nsPluginArray.h"

#include "mozilla/dom/PluginArrayBinding.h"
#include "mozilla/dom/PluginBinding.h"

#include "nsMimeTypeArray.h"
#include "Navigator.h"
#include "nsIDocShell.h"
#include "nsIWebNavigation.h"
#include "nsPluginHost.h"
#include "nsPluginTags.h"
#include "nsIObserverService.h"
#include "nsIWeakReference.h"
#include "mozilla/Services.h"
#include "nsIInterfaceRequestorUtils.h"

using namespace mozilla;
using namespace mozilla::dom;

nsPluginArray::nsPluginArray(nsPIDOMWindow* aWindow)
  : mWindow(aWindow)
{
}

void
nsPluginArray::Init()
{
  nsCOMPtr<nsIObserverService> obsService =
    mozilla::services::GetObserverService();
  if (obsService) {
    obsService->AddObserver(this, "plugin-info-updated", true);
  }
}

nsPluginArray::~nsPluginArray()
{
}

nsPIDOMWindow*
nsPluginArray::GetParentObject() const
{
  MOZ_ASSERT(mWindow);
  return mWindow;
}

JSObject*
nsPluginArray::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return PluginArrayBinding::Wrap(aCx, this, aGivenProto);
}

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsPluginArray)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsPluginArray)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsPluginArray)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIObserver)
  NS_INTERFACE_MAP_ENTRY(nsIObserver)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(nsPluginArray,
                                      mWindow,
                                      mPlugins)

static void
GetPluginMimeTypes(const nsTArray<nsRefPtr<nsPluginElement> >& aPlugins,
                   nsTArray<nsRefPtr<nsMimeType> >& aMimeTypes)
{
  for (uint32_t i = 0; i < aPlugins.Length(); ++i) {
    nsPluginElement *plugin = aPlugins[i];
    aMimeTypes.AppendElements(plugin->MimeTypes());
  }
}

static bool
operator<(const nsRefPtr<nsMimeType>& lhs, const nsRefPtr<nsMimeType>& rhs)
{
  
  return lhs->Type() < rhs->Type();
}

void
nsPluginArray::GetMimeTypes(nsTArray<nsRefPtr<nsMimeType>>& aMimeTypes)
{
  aMimeTypes.Clear();

  if (!AllowPlugins()) {
    return;
  }

  EnsurePlugins();

  GetPluginMimeTypes(mPlugins, aMimeTypes);

  
  
  aMimeTypes.Sort();
}

nsPluginElement*
nsPluginArray::Item(uint32_t aIndex)
{
  bool unused;
  return IndexedGetter(aIndex, unused);
}

nsPluginElement*
nsPluginArray::NamedItem(const nsAString& aName)
{
  bool unused;
  return NamedGetter(aName, unused);
}

void
nsPluginArray::Refresh(bool aReloadDocuments)
{
  nsRefPtr<nsPluginHost> pluginHost = nsPluginHost::GetInst();

  if(!AllowPlugins() || !pluginHost) {
    return;
  }

  
  
  if (pluginHost->ReloadPlugins() ==
      NS_ERROR_PLUGINS_PLUGINSNOTCHANGED) {
    nsTArray<nsRefPtr<nsPluginTag> > newPluginTags;
    pluginHost->GetPlugins(newPluginTags);

    
    
    
    
    
    
    
    if (newPluginTags.Length() == mPlugins.Length()) {
      return;
    }
  }

  mPlugins.Clear();

  nsCOMPtr<nsIDOMNavigator> navigator;
  mWindow->GetNavigator(getter_AddRefs(navigator));

  if (!navigator) {
    return;
  }

  static_cast<mozilla::dom::Navigator*>(navigator.get())->RefreshMIMEArray();

  nsCOMPtr<nsIWebNavigation> webNav = do_GetInterface(mWindow);
  if (aReloadDocuments && webNav) {
    webNav->Reload(nsIWebNavigation::LOAD_FLAGS_NONE);
  }
}

nsPluginElement*
nsPluginArray::IndexedGetter(uint32_t aIndex, bool &aFound)
{
  aFound = false;

  if (!AllowPlugins()) {
    return nullptr;
  }

  EnsurePlugins();

  aFound = aIndex < mPlugins.Length();

  return aFound ? mPlugins[aIndex] : nullptr;
}

void
nsPluginArray::Invalidate()
{
  nsCOMPtr<nsIObserverService> obsService =
    mozilla::services::GetObserverService();
  if (obsService) {
    obsService->RemoveObserver(this, "plugin-info-updated");
  }
}

static nsPluginElement*
FindPlugin(const nsTArray<nsRefPtr<nsPluginElement> >& aPlugins,
           const nsAString& aName)
{
  for (uint32_t i = 0; i < aPlugins.Length(); ++i) {
    nsAutoString pluginName;
    nsPluginElement* plugin = aPlugins[i];
    plugin->GetName(pluginName);

    if (pluginName.Equals(aName)) {
      return plugin;
    }
  }

  return nullptr;
}

nsPluginElement*
nsPluginArray::NamedGetter(const nsAString& aName, bool &aFound)
{
  aFound = false;

  if (!AllowPlugins()) {
    return nullptr;
  }

  EnsurePlugins();

  nsPluginElement* plugin = FindPlugin(mPlugins, aName);
  aFound = (plugin != nullptr);
  return plugin;
}

bool
nsPluginArray::NameIsEnumerable(const nsAString& aName)
{
  return true;
}

uint32_t
nsPluginArray::Length()
{
  if (!AllowPlugins()) {
    return 0;
  }

  EnsurePlugins();

  return mPlugins.Length();
}

void
nsPluginArray::GetSupportedNames(unsigned, nsTArray<nsString>& aRetval)
{
  aRetval.Clear();

  if (!AllowPlugins()) {
    return;
  }

  for (uint32_t i = 0; i < mPlugins.Length(); ++i) {
    nsAutoString pluginName;
    mPlugins[i]->GetName(pluginName);

    aRetval.AppendElement(pluginName);
  }
}

NS_IMETHODIMP
nsPluginArray::Observe(nsISupports *aSubject, const char *aTopic,
                       const char16_t *aData) {
  if (!nsCRT::strcmp(aTopic, "plugin-info-updated")) {
    Refresh(false);
  }

  return NS_OK;
}

bool
nsPluginArray::AllowPlugins() const
{
  nsCOMPtr<nsIDocShell> docShell = mWindow ? mWindow->GetDocShell() : nullptr;

  return docShell && docShell->PluginsAllowedInCurrentDoc();
}

static bool
operator<(const nsRefPtr<nsPluginElement>& lhs,
          const nsRefPtr<nsPluginElement>& rhs)
{
  
  return lhs->PluginTag()->mName < rhs->PluginTag()->mName;
}

void
nsPluginArray::EnsurePlugins()
{
  if (!mPlugins.IsEmpty()) {
    
    return;
  }

  nsRefPtr<nsPluginHost> pluginHost = nsPluginHost::GetInst();
  if (!pluginHost) {
    
    return;
  }

  nsTArray<nsRefPtr<nsPluginTag> > pluginTags;
  pluginHost->GetPlugins(pluginTags);

  
  
  for (uint32_t i = 0; i < pluginTags.Length(); ++i) {
    nsPluginTag* pluginTag = pluginTags[i];
    mPlugins.AppendElement(new nsPluginElement(mWindow, pluginTag));
  }

  
  
  mPlugins.Sort();
}



NS_IMPL_CYCLE_COLLECTING_ADDREF(nsPluginElement)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsPluginElement)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsPluginElement)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(nsPluginElement, mWindow, mMimeTypes)

nsPluginElement::nsPluginElement(nsPIDOMWindow* aWindow,
                                 nsPluginTag* aPluginTag)
  : mWindow(aWindow),
    mPluginTag(aPluginTag)
{
}

nsPluginElement::~nsPluginElement()
{
}

nsPIDOMWindow*
nsPluginElement::GetParentObject() const
{
  MOZ_ASSERT(mWindow);
  return mWindow;
}

JSObject*
nsPluginElement::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return PluginBinding::Wrap(aCx, this, aGivenProto);
}

void
nsPluginElement::GetDescription(nsString& retval) const
{
  CopyUTF8toUTF16(mPluginTag->mDescription, retval);
}

void
nsPluginElement::GetFilename(nsString& retval) const
{
  CopyUTF8toUTF16(mPluginTag->mFileName, retval);
}

void
nsPluginElement::GetVersion(nsString& retval) const
{
  CopyUTF8toUTF16(mPluginTag->mVersion, retval);
}

void
nsPluginElement::GetName(nsString& retval) const
{
  CopyUTF8toUTF16(mPluginTag->mName, retval);
}

nsMimeType*
nsPluginElement::Item(uint32_t aIndex)
{
  EnsurePluginMimeTypes();

  return mMimeTypes.SafeElementAt(aIndex);
}

nsMimeType*
nsPluginElement::NamedItem(const nsAString& aName)
{
  bool unused;
  return NamedGetter(aName, unused);
}

nsMimeType*
nsPluginElement::IndexedGetter(uint32_t aIndex, bool &aFound)
{
  EnsurePluginMimeTypes();

  aFound = aIndex < mMimeTypes.Length();

  return aFound ? mMimeTypes[aIndex] : nullptr;
}

nsMimeType*
nsPluginElement::NamedGetter(const nsAString& aName, bool &aFound)
{
  EnsurePluginMimeTypes();

  aFound = false;

  for (uint32_t i = 0; i < mMimeTypes.Length(); ++i) {
    if (mMimeTypes[i]->Type().Equals(aName)) {
      aFound = true;

      return mMimeTypes[i];
    }
  }

  return nullptr;
}

bool
nsPluginElement::NameIsEnumerable(const nsAString& aName)
{
  return true;
}

uint32_t
nsPluginElement::Length()
{
  EnsurePluginMimeTypes();

  return mMimeTypes.Length();
}

void
nsPluginElement::GetSupportedNames(unsigned, nsTArray<nsString>& retval)
{
  EnsurePluginMimeTypes();

  for (uint32_t i = 0; i < mMimeTypes.Length(); ++i) {
    retval.AppendElement(mMimeTypes[i]->Type());
  }
}

nsTArray<nsRefPtr<nsMimeType> >&
nsPluginElement::MimeTypes()
{
  EnsurePluginMimeTypes();

  return mMimeTypes;
}

void
nsPluginElement::EnsurePluginMimeTypes()
{
  if (!mMimeTypes.IsEmpty()) {
    return;
  }

  for (uint32_t i = 0; i < mPluginTag->mMimeTypes.Length(); ++i) {
    NS_ConvertUTF8toUTF16 type(mPluginTag->mMimeTypes[i]);
    mMimeTypes.AppendElement(new nsMimeType(mWindow, this, i, type));
  }
}
