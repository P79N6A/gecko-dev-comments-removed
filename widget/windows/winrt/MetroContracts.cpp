




#include "FrameworkView.h"
#include "MetroUtils.h"
#include "nsICommandLineRunner.h"
#include "nsNetUtil.h"
#include "nsIDOMChromeWindow.h"
#include "nsIURI.h"
#include "nsPrintfCString.h"
#include "mozilla/Services.h"
#include <wrl/wrappers/corewrappers.h>
#include <shellapi.h>
#include <DXGIFormat.h>
#include <d2d1_1.h>
#include <printpreview.h>
#include <D3D10.h>
#include "MetroUIUtils.h"
#include "nsIStringBundle.h"

using namespace mozilla;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Foundation::Collections;
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;


using namespace ABI::Windows::Media::PlayTo;


using namespace ABI::Windows::ApplicationModel::Activation;
using namespace ABI::Windows::ApplicationModel::DataTransfer;
using namespace ABI::Windows::ApplicationModel::Search;


using namespace ABI::Windows::UI::ApplicationSettings;
using namespace ABI::Windows::UI::Popups;


using namespace ABI::Windows::Graphics::Printing;

namespace mozilla {
namespace widget {
namespace winrt {

extern nsTArray<nsString>* sSettingsArray;

void
FrameworkView::SearchActivated(ComPtr<ISearchActivatedEventArgs>& aArgs)
{
  if (!aArgs)
    return;

  HString data;
  AssertHRESULT(aArgs->get_QueryText(data.GetAddressOf()));
  if (WindowsIsStringEmpty(data.Get()))
    return;

  unsigned int length;
  Log(L"SearchActivated text=", data.GetRawBuffer(&length));
  PerformURILoadOrSearch(data);
}

void
FrameworkView::FileActivated(ComPtr<IFileActivatedEventArgs>& aArgs)
{
  if (!aArgs)
    return;

  ComPtr<IVectorView<ABI::Windows::Storage::IStorageItem*>> list;
  AssertHRESULT(aArgs->get_Files(list.GetAddressOf()));
  ComPtr<ABI::Windows::Storage::IStorageItem> item;
  AssertHRESULT(list->GetAt(0, item.GetAddressOf()));
  HString filePath;
  AssertHRESULT(item->get_Path(filePath.GetAddressOf()));

  ComPtr<IUriRuntimeClass> uri;
  AssertHRESULT(MetroUtils::CreateUri(filePath, uri));
  PerformURILoad(uri);
}

void
FrameworkView::LaunchActivated(ComPtr<ILaunchActivatedEventArgs>& aArgs)
{
  if (!aArgs)
    return;
  HString data;
  AssertHRESULT(aArgs->get_Arguments(data.GetAddressOf()));
  if (WindowsIsStringEmpty(data.Get()))
    return;

  int argc;
  unsigned int length;
  LPWSTR* argv = CommandLineToArgvW(data.GetRawBuffer(&length), &argc);
  nsCOMPtr<nsICommandLineRunner> cmdLine =
    (do_CreateInstance("@mozilla.org/toolkit/command-line;1"));
  if (!cmdLine) {
    NS_WARNING("Unable to instantiate command line runner.");
    return;
  }

  LPSTR *argvUTF8 = new LPSTR[argc];
  for (int i = 0; i < argc; ++i) {
    NS_ConvertUTF16toUTF8 arg(argv[i]);
    argvUTF8[i] = new char[arg.Length() + 1];
    strcpy(argvUTF8[i], const_cast<char *>(arg.BeginReading()));
    Log(L"Launch arg[%d]: '%s'", i, argv[i]);
  }

  nsresult rv = cmdLine->Init(argc,
                              argvUTF8,
                              nullptr,
                              nsICommandLine::STATE_REMOTE_EXPLICIT);
  if (NS_SUCCEEDED(rv)) {
    cmdLine->Run();
  } else {
    NS_WARNING("cmdLine->Init failed.");
  }

  for (int i = 0; i < argc; ++i) {
    delete[] argvUTF8[i];
  }
  delete[] argvUTF8;
}

void
FrameworkView::RunStartupArgs(IActivatedEventArgs* aArgs)
{
  ActivationKind kind;
  if (!aArgs || FAILED(aArgs->get_Kind(&kind)))
    return;
  ComPtr<IActivatedEventArgs> args(aArgs);
  if (kind == ActivationKind::ActivationKind_Protocol) {
    Log(L"Activation argument kind: Protocol");
    ComPtr<IProtocolActivatedEventArgs> protoArgs;
    AssertHRESULT(args.As(&protoArgs));
    ComPtr<IUriRuntimeClass> uri;
    AssertHRESULT(protoArgs->get_Uri(uri.GetAddressOf()));
    PerformURILoad(uri);
  } else if (kind == ActivationKind::ActivationKind_Search) {
    Log(L"Activation argument kind: Search");
    ComPtr<ISearchActivatedEventArgs> searchArgs;
    args.As(&searchArgs);
    SearchActivated(searchArgs);
  } else if (kind == ActivationKind::ActivationKind_File) {
    Log(L"Activation argument kind: File");
    ComPtr<IFileActivatedEventArgs> fileArgs;
    args.As(&fileArgs);
    FileActivated(fileArgs);
  } else if (kind == ActivationKind::ActivationKind_Launch) {
    Log(L"Activation argument kind: Launch");
    ComPtr<ILaunchActivatedEventArgs> launchArgs;
    args.As(&launchArgs);
    LaunchActivated(launchArgs);
  }
}

void
FrameworkView::SetupContracts()
{
  LogFunction();
  HRESULT hr;

  
  ComPtr<IDataTransferManagerStatics> transStatics;
  hr = GetActivationFactory(HStringReference(RuntimeClass_Windows_ApplicationModel_DataTransfer_DataTransferManager).Get(),
                            transStatics.GetAddressOf());
  AssertHRESULT(hr);
  ComPtr<IDataTransferManager> trans;
  AssertHRESULT(transStatics->GetForCurrentView(trans.GetAddressOf()));
  trans->add_DataRequested(Callback<__FITypedEventHandler_2_Windows__CApplicationModel__CDataTransfer__CDataTransferManager_Windows__CApplicationModel__CDataTransfer__CDataRequestedEventArgs_t>(
    this, &FrameworkView::OnDataShareRequested).Get(), &mDataTransferRequested);

  
  ComPtr<ISearchPaneStatics> searchStatics;
  hr = GetActivationFactory(HStringReference(RuntimeClass_Windows_ApplicationModel_Search_SearchPane).Get(),
                            searchStatics.GetAddressOf());
  AssertHRESULT(hr);
  ComPtr<ISearchPane> searchPane;
  AssertHRESULT(searchStatics->GetForCurrentView(searchPane.GetAddressOf()));
  searchPane->add_QuerySubmitted(Callback<__FITypedEventHandler_2_Windows__CApplicationModel__CSearch__CSearchPane_Windows__CApplicationModel__CSearch__CSearchPaneQuerySubmittedEventArgs_t>(
    this, &FrameworkView::OnSearchQuerySubmitted).Get(), &mSearchQuerySubmitted);

  
  ComPtr<IPlayToManagerStatics> playToStatics;
  hr = GetActivationFactory(HStringReference(RuntimeClass_Windows_Media_PlayTo_PlayToManager).Get(),
                            playToStatics.GetAddressOf());
  AssertHRESULT(hr);
  ComPtr<IPlayToManager> playTo;
  AssertHRESULT(playToStatics->GetForCurrentView(playTo.GetAddressOf()));
  playTo->add_SourceRequested(Callback<__FITypedEventHandler_2_Windows__CMedia__CPlayTo__CPlayToManager_Windows__CMedia__CPlayTo__CPlayToSourceRequestedEventArgs_t>(
    this, &FrameworkView::OnPlayToSourceRequested).Get(), &mPlayToRequested);

  
  ComPtr<ISettingsPaneStatics> settingsPaneStatics;
  hr = GetActivationFactory(HStringReference(RuntimeClass_Windows_UI_ApplicationSettings_SettingsPane).Get(),
                            settingsPaneStatics.GetAddressOf());
  AssertHRESULT(hr);
  ComPtr<ISettingsPane> settingsPane;
  AssertHRESULT(settingsPaneStatics->GetForCurrentView(settingsPane.GetAddressOf()));
  settingsPane->add_CommandsRequested(Callback<__FITypedEventHandler_2_Windows__CUI__CApplicationSettings__CSettingsPane_Windows__CUI__CApplicationSettings__CSettingsPaneCommandsRequestedEventArgs_t>(
    this, &FrameworkView::OnSettingsCommandsRequested).Get(), &mSettingsPane);

  
  ComPtr<IPrintManagerStatic> printStatics;
  hr = GetActivationFactory(HStringReference(RuntimeClass_Windows_Graphics_Printing_PrintManager).Get(),
                            printStatics.GetAddressOf());
  AssertHRESULT(hr);
  ComPtr<IPrintManager> printManager;
  AssertHRESULT(printStatics->GetForCurrentView(printManager.GetAddressOf()));
  printManager->add_PrintTaskRequested(Callback<__FITypedEventHandler_2_Windows__CGraphics__CPrinting__CPrintManager_Windows__CGraphics__CPrinting__CPrintTaskRequestedEventArgs_t>(
    this, &FrameworkView::OnPrintTaskRequested).Get(), &mPrintManager);
}

void
FrameworkView::PerformURILoad(ComPtr<IUriRuntimeClass>& aURI)
{
  LogFunction();
  if (!aURI)
    return;

  HString data;
  AssertHRESULT(aURI->get_AbsoluteUri(data.GetAddressOf()));
  if (WindowsIsStringEmpty(data.Get()))
    return;

  unsigned int length;
  Log(L"PerformURILoad uri=%s", data.GetRawBuffer(&length));

  nsCOMPtr<nsICommandLineRunner> cmdLine =
    (do_CreateInstance("@mozilla.org/toolkit/command-line;1"));
  if (!cmdLine) {
    NS_WARNING("Unable to instantiate command line runner.");
    return;
  }

  nsAutoCString utf8data(NS_ConvertUTF16toUTF8(data.GetRawBuffer(&length)));
  const char *argv[] = { "metrobrowser",
                         "-url",
                         utf8data.BeginReading() };
  nsresult rv = cmdLine->Init(ArrayLength(argv),
                              const_cast<char **>(argv), nullptr,
                              nsICommandLine::STATE_REMOTE_EXPLICIT);
  if (NS_FAILED(rv)) {
    NS_WARNING("cmdLine->Init failed.");
    return;
  }
  cmdLine->Run();
}

void
FrameworkView::PerformSearch(HString& aQuery)
{
  LogFunction();

  nsCOMPtr<nsICommandLineRunner> cmdLine =
    (do_CreateInstance("@mozilla.org/toolkit/command-line;1"));
  if (!cmdLine) {
    NS_WARNING("Unable to instantiate command line runner.");
    return;
  }

  nsAutoCString parameter;
  parameter.AppendLiteral("\"");
  unsigned int length;
  parameter.Append(NS_ConvertUTF16toUTF8(aQuery.GetRawBuffer(&length)));
  parameter.AppendLiteral("\"");

  const char *argv[] = { "metrobrowser",
                         "-search",
                         parameter.BeginReading() };
  nsresult rv = cmdLine->Init(ArrayLength(argv),
                              const_cast<char **>(argv), nullptr,
                              nsICommandLine::STATE_REMOTE_EXPLICIT);
  if (NS_FAILED(rv)) {
    NS_WARNING("cmdLine->Init failed.");
    return;
  }
  cmdLine->Run();
}

void
FrameworkView::PerformURILoadOrSearch(HString& aString)
{
  LogFunction();

  if (WindowsIsStringEmpty(aString.Get())) {
    Log(L"Emptry string passed to PerformURILoadOrSearch");
    return;
  }

  
  ComPtr<IUriRuntimeClass> uri;
  MetroUtils::CreateUri(aString.Get(), uri);
  if (uri) {
    PerformURILoad(uri);
  } else {
    PerformSearch(aString);
  }
}

HRESULT
FrameworkView::OnDataShareRequested(IDataTransferManager* aDTM,
                                    IDataRequestedEventArgs* aArg)
{
  
  nsCOMPtr<nsIMetroUIUtils> metroUIUtils = do_CreateInstance("@mozilla.org/metro-ui-utils;1");
  if (!metroUIUtils)
      return E_FAIL;

  nsString url, title;
  nsresult rv = metroUIUtils->GetCurrentPageURI(url);
  nsresult rv2 = metroUIUtils->GetCurrentPageTitle(title);
  if (NS_FAILED(rv) || NS_FAILED(rv2)) {
    return E_FAIL;
  }

  
  HRESULT hr;
  ComPtr<IDataRequest> request;
  AssertRetHRESULT(hr = aArg->get_Request(request.GetAddressOf()), hr);
  ComPtr<IDataPackage> dataPackage;
  AssertRetHRESULT(hr = request->get_Data(dataPackage.GetAddressOf()), hr);
  ComPtr<IDataPackagePropertySet> props;
  AssertRetHRESULT(hr = dataPackage->get_Properties(props.GetAddressOf()), hr);

  
  
  
  bool hasSelectedContent = false;
  metroUIUtils->GetHasSelectedContent(&hasSelectedContent);
  if (!hasSelectedContent) {
    ComPtr<IUriRuntimeClass> uri;
    AssertRetHRESULT(hr = MetroUtils::CreateUri(HStringReference(url.BeginReading()).Get(), uri), hr);

    
    
    HString schemeHString;
    uri->get_SchemeName(schemeHString.GetAddressOf());
    unsigned int length;
    LPCWSTR scheme = schemeHString.GetRawBuffer(&length);
    if (!scheme || wcscmp(scheme, L"http") && wcscmp(scheme, L"https") &&
        wcscmp(scheme, L"ftp") && wcscmp(scheme, L"file")) {
      return S_OK;
    }

    AssertRetHRESULT(hr = dataPackage->SetUri(uri.Get()), hr);
  }

  
  nsString shareText;
  if (NS_SUCCEEDED(metroUIUtils->GetShareText(shareText)) && shareText.Length()) {
    AssertRetHRESULT(hr = dataPackage->SetText(HStringReference(shareText.BeginReading()).Get()) ,hr);
  }

  
  nsString shareHTML;
  if (NS_SUCCEEDED(metroUIUtils->GetShareHTML(shareHTML)) && shareHTML.Length()) {
    
    ComPtr<IHtmlFormatHelperStatics> htmlFormatHelper;
    hr = GetActivationFactory(HStringReference(RuntimeClass_Windows_ApplicationModel_DataTransfer_HtmlFormatHelper).Get(),
                              htmlFormatHelper.GetAddressOf());
    AssertRetHRESULT(hr, hr);
    HSTRING fixedHTML;
    htmlFormatHelper->CreateHtmlFormat(HStringReference(shareHTML.BeginReading()).Get(), &fixedHTML);

    
    AssertRetHRESULT(hr = dataPackage->SetHtmlFormat(fixedHTML), hr);
  }

  
  nsCOMPtr<nsIStringBundleService> bundleService = 
    do_GetService(NS_STRINGBUNDLE_CONTRACTID);
  NS_ENSURE_TRUE(bundleService, E_FAIL);
  nsCOMPtr<nsIStringBundle> brandBundle;
  nsString brandName;
  bundleService->CreateBundle("chrome://branding/locale/brand.properties",
    getter_AddRefs(brandBundle));
  NS_ENSURE_TRUE(brandBundle, E_FAIL);
  if(brandBundle) {
    brandBundle->GetStringFromName(NS_LITERAL_STRING("brandFullName").get(),
                                   getter_Copies(brandName));
  }

  
  
  
  props->put_ApplicationName(HStringReference(brandName.BeginReading()).Get());
  if (title.Length()) {
    props->put_Title(HStringReference(title.BeginReading()).Get());
  } else {
    props->put_Title(HStringReference(brandName.BeginReading()).Get());
  }
  props->put_Description(HStringReference(url.BeginReading()).Get());

  return S_OK;
}

HRESULT
FrameworkView::OnSearchQuerySubmitted(ISearchPane* aPane,
                                      ISearchPaneQuerySubmittedEventArgs* aArgs)
{
  LogFunction();
  HString aQuery;
  aArgs->get_QueryText(aQuery.GetAddressOf());
  PerformURILoadOrSearch(aQuery);
  return S_OK;
}

HRESULT
FrameworkView::OnSettingsCommandInvoked(IUICommand* aCommand)
{
  LogFunction();
  HRESULT hr;
  uint32_t id;
  ComPtr<IPropertyValue> prop;
  AssertRetHRESULT(hr = aCommand->get_Id((IInspectable**)prop.GetAddressOf()), hr);
  AssertRetHRESULT(hr = prop->GetUInt32(&id), hr);

  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
  if (obs) {
    NS_ConvertASCIItoUTF16 idStr(nsPrintfCString("%lu", id));
    obs->NotifyObservers(nullptr, "metro-settings-entry-selected", idStr.BeginReading());
  }

  return S_OK;
}

void
FrameworkView::AddSetting(ISettingsPaneCommandsRequestedEventArgs* aArgs,
                          uint32_t aId, HString& aSettingName)
{
  HRESULT hr;

  ComPtr<ABI::Windows::UI::ApplicationSettings::ISettingsPaneCommandsRequest> request;
  AssertHRESULT(aArgs->get_Request(request.GetAddressOf()));

  
  ComPtr<IVector<ABI::Windows::UI::ApplicationSettings::SettingsCommand*>> list;
  AssertHRESULT(request->get_ApplicationCommands(list.GetAddressOf()));

  ComPtr<IUICommand> command;
  ComPtr<ISettingsCommandFactory> factory;
  hr = GetActivationFactory(HStringReference(RuntimeClass_Windows_UI_ApplicationSettings_SettingsCommand).Get(),
                            factory.GetAddressOf());
  AssertHRESULT(hr);

  
  ComPtr<IInspectable> prop;
  ComPtr<IPropertyValueStatics> propStatics;
  hr = GetActivationFactory(HStringReference(RuntimeClass_Windows_Foundation_PropertyValue).Get(),
                            propStatics.GetAddressOf());
  AssertHRESULT(hr);
  hr = propStatics->CreateUInt32(aId, prop.GetAddressOf());
  AssertHRESULT(hr);

  
  hr = factory->CreateSettingsCommand(prop.Get(), aSettingName.Get(),
    Callback<ABI::Windows::UI::Popups::IUICommandInvokedHandler>(
      this, &FrameworkView::OnSettingsCommandInvoked).Get(), command.GetAddressOf());
  AssertHRESULT(hr);

  
  hr = list->Append(command.Get());
  AssertHRESULT(hr);
}

HRESULT
FrameworkView::OnSettingsCommandsRequested(ISettingsPane* aPane,
                                           ISettingsPaneCommandsRequestedEventArgs* aArgs)
{
  if (!sSettingsArray)
    return E_FAIL;
  if (!sSettingsArray->Length())
    return S_OK;
  for (uint32_t i = 0; i < sSettingsArray->Length(); i++) {
    HString label;
    label.Set(sSettingsArray->ElementAt(i).BeginReading());
    AddSetting(aArgs, i, label);
  }
  return S_OK;
}

HRESULT
FrameworkView::OnPlayToSourceRequested(IPlayToManager* aPlayToManager,
                                       IPlayToSourceRequestedEventArgs* aArgs)
{
  
  
  
  
  
 return S_OK;
}

HRESULT
FrameworkView::OnPrintTaskSourceRequested(IPrintTaskSourceRequestedArgs* aArgs)
{
 return S_OK;
}

HRESULT
FrameworkView::OnPrintTaskRequested(IPrintManager* aPrintManager,
                                    IPrintTaskRequestedEventArgs* aArgs)
{
 return S_OK;
}

void
FrameworkView::CreatePrintControl(IPrintDocumentPackageTarget* docPackageTarget,
                                  D2D1_PRINT_CONTROL_PROPERTIES* printControlProperties)
{
}

HRESULT
FrameworkView::ClosePrintControl()
{
  return S_OK;
}



void FrameworkView::PrintPage(uint32_t pageNumber,
                              D2D1_RECT_F imageableArea,
                              D2D1_SIZE_F pageSize,
                              IStream* pagePrintTicketStream)
{
}

} } }
