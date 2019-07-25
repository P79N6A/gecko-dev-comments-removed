




































#include "mozilla/jetpack/JetpackParent.h"

namespace mozilla {
namespace jetpack {

JetpackParent::JetpackParent()
  : mSubprocess(new JetpackProcessParent())
{
  mSubprocess->Launch();
  Open(mSubprocess->GetChannel(),
       mSubprocess->GetChildProcessHandle());
}

JetpackParent::~JetpackParent()
{
  XRE_GetIOMessageLoop()
    ->PostTask(FROM_HERE, new DeleteTask<JetpackProcessParent>(mSubprocess));
}

NS_IMPL_ISUPPORTS1(JetpackParent, nsIJetpack)

NS_IMETHODIMP
JetpackParent::LoadImplementation(const nsAString& aURI)
{
  
  if (!SendLoadImplementation(NS_ConvertUTF16toUTF8(aURI)))
    return NS_ERROR_FAILURE;

  return NS_OK;
}

} 
} 
