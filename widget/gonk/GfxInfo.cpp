




#include "GfxInfo.h"

using namespace mozilla::widget;



nsresult
GfxInfo::GetD2DEnabled(bool *aEnabled)
{
  return NS_ERROR_FAILURE;
}

nsresult
GfxInfo::GetDWriteEnabled(bool *aEnabled)
{
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
GfxInfo::GetDWriteVersion(nsAString & aDwriteVersion)
{
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
GfxInfo::GetCleartypeParameters(nsAString & aCleartypeParams)
{
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDescription(nsAString & aAdapterDescription)
{
  aAdapterDescription.AssignLiteral("");
  return NS_OK;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDescription2(nsAString & aAdapterDescription)
{
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
GfxInfo::GetAdapterRAM(nsAString & aAdapterRAM)
{
  aAdapterRAM.AssignLiteral("");
  return NS_OK;
}


NS_IMETHODIMP
GfxInfo::GetAdapterRAM2(nsAString & aAdapterRAM)
{
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDriver(nsAString & aAdapterDriver)
{
  aAdapterDriver.AssignLiteral("");
  return NS_OK;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDriver2(nsAString & aAdapterDriver)
{
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDriverVersion(nsAString & aAdapterDriverVersion)
{
  aAdapterDriverVersion.AssignLiteral("");
  return NS_OK;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDriverVersion2(nsAString & aAdapterDriverVersion)
{
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDriverDate(nsAString & aAdapterDriverDate)
{
  aAdapterDriverDate.AssignLiteral("");
  return NS_OK;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDriverDate2(nsAString & aAdapterDriverDate)
{
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
GfxInfo::GetAdapterVendorID(nsAString & aAdapterVendorID)
{
  aAdapterVendorID.AssignLiteral("");
  return NS_OK;
}


NS_IMETHODIMP
GfxInfo::GetAdapterVendorID2(nsAString & aAdapterVendorID)
{
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDeviceID(nsAString & aAdapterDeviceID)
{
  aAdapterDeviceID.AssignLiteral("");
  return NS_OK;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDeviceID2(nsAString & aAdapterDeviceID)
{
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
GfxInfo::GetIsGPU2Active(bool* aIsGPU2Active)
{
  return NS_ERROR_FAILURE;
}

const nsTArray<GfxDriverInfo>&
GfxInfo::GetGfxDriverInfo()
{
  return *mDriverInfo;
}

#ifdef DEBUG




NS_IMETHODIMP GfxInfo::SpoofVendorID(const nsAString & aVendorID)
{
  return NS_OK;
}


NS_IMETHODIMP GfxInfo::SpoofDeviceID(const nsAString & aDeviceID)
{
  return NS_OK;
}


NS_IMETHODIMP GfxInfo::SpoofDriverVersion(const nsAString & aDriverVersion)
{
  return NS_OK;
}


NS_IMETHODIMP GfxInfo::SpoofOSVersion(uint32_t aVersion)
{
  return NS_OK;
}

#endif

uint32_t GfxInfo::OperatingSystemVersion() const
{
  return 0;
}
