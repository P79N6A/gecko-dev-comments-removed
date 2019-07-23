






































#include "AppComponents.h"
#include "PromptService.h"
#include "UDownload.h"

#define NS_PROMPTSERVICE_CID \
  {0xa2112d6a, 0x0e28, 0x421f, {0xb4, 0x6a, 0x25, 0xc0, 0xb3, 0x8, 0xcb, 0xd0}}
#define NS_HELPERAPPLAUNCHERDIALOG_CID \
      {0xf68578eb, 0x6ec2, 0x4169, {0xae, 0x19, 0x8c, 0x62, 0x43, 0xf0, 0xab, 0xe1}}

NS_GENERIC_FACTORY_CONSTRUCTOR(CPromptService)
NS_GENERIC_FACTORY_CONSTRUCTOR(CDownload)
NS_GENERIC_FACTORY_CONSTRUCTOR(CHelperAppLauncherDialog)

static const nsModuleComponentInfo components[] = {
  {
    "Prompt Service",
    NS_PROMPTSERVICE_CID,
    "@mozilla.org/embedcomp/prompt-service;1",
    CPromptServiceConstructor
  },
  {
    "Download",
    NS_DOWNLOAD_CID,
    NS_TRANSFER_CONTRACTID,
    CDownloadConstructor
  },
  {
    NS_IHELPERAPPLAUNCHERDLG_CLASSNAME,
    NS_HELPERAPPLAUNCHERDIALOG_CID,
    NS_IHELPERAPPLAUNCHERDLG_CONTRACTID,
    CHelperAppLauncherDialogConstructor
  }
};

const nsModuleComponentInfo* GetAppModuleComponentInfo(int* outNumComponents)
{
  *outNumComponents = sizeof(components) / sizeof(components[0]);
  return components;
}

