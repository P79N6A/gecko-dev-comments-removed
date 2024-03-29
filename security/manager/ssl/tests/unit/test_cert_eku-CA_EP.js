





"use strict";

do_get_profile(); 
const certdb = Cc["@mozilla.org/security/x509certdb;1"]
                 .getService(Ci.nsIX509CertDB);

function cert_from_file(filename) {
  return constructCertFromFile(`test_cert_eku/${filename}.pem`);
}

function load_cert(cert_name, trust_string) {
  addCertFromFile(certdb, `test_cert_eku/${cert_name}.pem`, trust_string);
  return cert_from_file(cert_name);
}

function run_test() {
  load_cert("ca", "CT,CT,CT");

  checkCertErrorGeneric(certdb, load_cert('int-EKU-CA_EP', ',,'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageSSLCA);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-CA-int-EKU-CA_EP'), PRErrorCodeSuccess, certificateUsageSSLClient);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-CA-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageSSLServer);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-CA-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_KEY_USAGE, certificateUsageSSLCA);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-CA-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageEmailSigner);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-CA-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageEmailRecipient);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-CA-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageStatusResponder);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-CA_EP-int-EKU-CA_EP'), PRErrorCodeSuccess, certificateUsageSSLClient);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-CA_EP-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageSSLServer);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-CA_EP-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_KEY_USAGE, certificateUsageSSLCA);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-CA_EP-int-EKU-CA_EP'), PRErrorCodeSuccess, certificateUsageEmailSigner);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-CA_EP-int-EKU-CA_EP'), PRErrorCodeSuccess, certificateUsageEmailRecipient);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-CA_EP-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageStatusResponder);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-CA_EP_NS_OS_SA_TS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageSSLClient);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-CA_EP_NS_OS_SA_TS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageSSLServer);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-CA_EP_NS_OS_SA_TS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_KEY_USAGE, certificateUsageSSLCA);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-CA_EP_NS_OS_SA_TS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageEmailSigner);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-CA_EP_NS_OS_SA_TS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageEmailRecipient);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-CA_EP_NS_OS_SA_TS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageStatusResponder);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-CA_NS-int-EKU-CA_EP'), PRErrorCodeSuccess, certificateUsageSSLClient);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-CA_NS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageSSLServer);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-CA_NS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_KEY_USAGE, certificateUsageSSLCA);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-CA_NS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageEmailSigner);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-CA_NS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageEmailRecipient);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-CA_NS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageStatusResponder);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-CA_OS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageSSLClient);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-CA_OS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageSSLServer);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-CA_OS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_KEY_USAGE, certificateUsageSSLCA);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-CA_OS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageEmailSigner);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-CA_OS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageEmailRecipient);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-CA_OS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageStatusResponder);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-CA_SA-int-EKU-CA_EP'), PRErrorCodeSuccess, certificateUsageSSLClient);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-CA_SA-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageSSLServer);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-CA_SA-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_KEY_USAGE, certificateUsageSSLCA);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-CA_SA-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageEmailSigner);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-CA_SA-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageEmailRecipient);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-CA_SA-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageStatusResponder);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-CA_TS-int-EKU-CA_EP'), PRErrorCodeSuccess, certificateUsageSSLClient);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-CA_TS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageSSLServer);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-CA_TS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_KEY_USAGE, certificateUsageSSLCA);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-CA_TS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageEmailSigner);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-CA_TS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageEmailRecipient);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-CA_TS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageStatusResponder);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-EP-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageSSLClient);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-EP-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageSSLServer);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-EP-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_KEY_USAGE, certificateUsageSSLCA);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-EP-int-EKU-CA_EP'), PRErrorCodeSuccess, certificateUsageEmailSigner);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-EP-int-EKU-CA_EP'), PRErrorCodeSuccess, certificateUsageEmailRecipient);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-EP-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageStatusResponder);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-EP_NS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageSSLClient);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-EP_NS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageSSLServer);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-EP_NS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_KEY_USAGE, certificateUsageSSLCA);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-EP_NS-int-EKU-CA_EP'), PRErrorCodeSuccess, certificateUsageEmailSigner);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-EP_NS-int-EKU-CA_EP'), PRErrorCodeSuccess, certificateUsageEmailRecipient);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-EP_NS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageStatusResponder);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-EP_OS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageSSLClient);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-EP_OS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageSSLServer);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-EP_OS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_KEY_USAGE, certificateUsageSSLCA);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-EP_OS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageEmailSigner);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-EP_OS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageEmailRecipient);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-EP_OS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageStatusResponder);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-EP_SA-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageSSLClient);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-EP_SA-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageSSLServer);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-EP_SA-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_KEY_USAGE, certificateUsageSSLCA);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-EP_SA-int-EKU-CA_EP'), PRErrorCodeSuccess, certificateUsageEmailSigner);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-EP_SA-int-EKU-CA_EP'), PRErrorCodeSuccess, certificateUsageEmailRecipient);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-EP_SA-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageStatusResponder);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-EP_TS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageSSLClient);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-EP_TS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageSSLServer);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-EP_TS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_KEY_USAGE, certificateUsageSSLCA);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-EP_TS-int-EKU-CA_EP'), PRErrorCodeSuccess, certificateUsageEmailSigner);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-EP_TS-int-EKU-CA_EP'), PRErrorCodeSuccess, certificateUsageEmailRecipient);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-EP_TS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageStatusResponder);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-NONE-int-EKU-CA_EP'), PRErrorCodeSuccess, certificateUsageSSLClient);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-NONE-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageSSLServer);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-NONE-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_KEY_USAGE, certificateUsageSSLCA);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-NONE-int-EKU-CA_EP'), PRErrorCodeSuccess, certificateUsageEmailSigner);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-NONE-int-EKU-CA_EP'), PRErrorCodeSuccess, certificateUsageEmailRecipient);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-NONE-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageStatusResponder);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-NS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageSSLClient);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-NS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageSSLServer);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-NS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_KEY_USAGE, certificateUsageSSLCA);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-NS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageEmailSigner);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-NS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageEmailRecipient);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-NS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageStatusResponder);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-NS_OS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageSSLClient);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-NS_OS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageSSLServer);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-NS_OS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_KEY_USAGE, certificateUsageSSLCA);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-NS_OS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageEmailSigner);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-NS_OS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageEmailRecipient);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-NS_OS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageStatusResponder);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-NS_SA-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageSSLClient);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-NS_SA-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageSSLServer);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-NS_SA-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_KEY_USAGE, certificateUsageSSLCA);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-NS_SA-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageEmailSigner);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-NS_SA-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageEmailRecipient);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-NS_SA-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageStatusResponder);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-NS_TS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageSSLClient);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-NS_TS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageSSLServer);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-NS_TS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_KEY_USAGE, certificateUsageSSLCA);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-NS_TS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageEmailSigner);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-NS_TS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageEmailRecipient);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-NS_TS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageStatusResponder);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-OS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageSSLClient);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-OS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageSSLServer);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-OS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_KEY_USAGE, certificateUsageSSLCA);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-OS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageEmailSigner);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-OS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageEmailRecipient);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-OS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageStatusResponder);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-OS_SA-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageSSLClient);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-OS_SA-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageSSLServer);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-OS_SA-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_KEY_USAGE, certificateUsageSSLCA);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-OS_SA-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageEmailSigner);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-OS_SA-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageEmailRecipient);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-OS_SA-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageStatusResponder);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-OS_TS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageSSLClient);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-OS_TS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageSSLServer);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-OS_TS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_KEY_USAGE, certificateUsageSSLCA);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-OS_TS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageEmailSigner);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-OS_TS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageEmailRecipient);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-OS_TS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageStatusResponder);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-SA-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageSSLClient);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-SA-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageSSLServer);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-SA-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_KEY_USAGE, certificateUsageSSLCA);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-SA-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageEmailSigner);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-SA-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageEmailRecipient);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-SA-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageStatusResponder);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-SA_TS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageSSLClient);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-SA_TS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageSSLServer);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-SA_TS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_KEY_USAGE, certificateUsageSSLCA);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-SA_TS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageEmailSigner);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-SA_TS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageEmailRecipient);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-SA_TS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageStatusResponder);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-TS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageSSLClient);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-TS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageSSLServer);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-TS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_KEY_USAGE, certificateUsageSSLCA);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-TS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageEmailSigner);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-TS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageEmailRecipient);
  checkCertErrorGeneric(certdb, cert_from_file('ee-EKU-TS-int-EKU-CA_EP'), SEC_ERROR_INADEQUATE_CERT_TYPE, certificateUsageStatusResponder);
}
