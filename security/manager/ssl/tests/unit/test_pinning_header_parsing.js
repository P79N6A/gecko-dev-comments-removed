






let profileDir = do_get_profile();
const certdb = Cc["@mozilla.org/security/x509certdb;1"]
                 .getService(Ci.nsIX509CertDB);
let gSSService = Cc["@mozilla.org/ssservice;1"]
                   .getService(Ci.nsISiteSecurityService);

function certFromFile(cert_name) {
  return constructCertFromFile("test_pinning_dynamic/" + cert_name + ".pem");
}

function loadCert(cert_name, trust_string) {
  let cert_filename = "test_pinning_dynamic/" + cert_name + ".pem";
  addCertFromFile(certdb,  cert_filename, trust_string);
  return constructCertFromFile(cert_filename);
}

function checkFailParseInvalidPin(pinValue) {
  let sslStatus = new FakeSSLStatus(
                        certFromFile('cn-a.pinning2.example.com-pinningroot'));
  let uri = Services.io.newURI("https://a.pinning2.example.com", null, null);
  try {
    gSSService.processHeader(Ci.nsISiteSecurityService.HEADER_HPKP, uri,
                             pinValue, sslStatus, 0);
    ok(false, "Invalid pin should have been rejected");
  } catch (e) {
    ok(true, "Invalid pin should be rejected");
  }
}

function checkPassValidPin(pinValue, settingPin) {
  let sslStatus = new FakeSSLStatus(
                        certFromFile('cn-a.pinning2.example.com-pinningroot'));
  let uri = Services.io.newURI("https://a.pinning2.example.com", null, null);

  
  
  if (settingPin) {
    gSSService.removeState(Ci.nsISiteSecurityService.HEADER_HPKP, uri, 0);
  } else {
    
    let validPinValue ="max-age=5000;" + VALID_PIN1 + BACKUP_PIN1;
    gSSService.processHeader(Ci.nsISiteSecurityService.HEADER_HPKP, uri,
                             validPinValue, sslStatus, 0);
  }
  try {
    gSSService.processHeader(Ci.nsISiteSecurityService.HEADER_HPKP, uri,
                             pinValue, sslStatus, 0);
    ok(true, "Valid pin should be accepted");
  } catch (e) {
    ok(false, "Valid pin should have been accepted");
  }
  
  
  let hostIsPinned = gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HPKP,
                                             "a.pinning2.example.com", 0);
  if (settingPin) {
    ok(hostIsPinned, "Host should be considered pinned");
  } else {
    ok(!hostIsPinned, "Host should not be considered pinned");
  }
}

function checkPassSettingPin(pinValue) {
  return checkPassValidPin(pinValue, true);
}

function checkPassRemovingPin(pinValue) {
  return checkPassValidPin(pinValue, false);
}

const NON_ISSUED_KEY_HASH1 = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=";
const NON_ISSUED_KEY_HASH2 = "ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ=";
const PINNING_ROOT_KEY_HASH = "VCIlmPM9NkgFQtrs4Oa5TeFcDu6MWRTKSNdePEhOgD8=";
const MAX_AGE_ZERO = "max-age=0;"
const VALID_PIN1 = "pin-sha256=\""+ PINNING_ROOT_KEY_HASH + "\";";
const BACKUP_PIN1 = "pin-sha256=\""+ NON_ISSUED_KEY_HASH1 + "\";";
const BACKUP_PIN2 = "pin-sha256=\""+ NON_ISSUED_KEY_HASH2 + "\";";
const BROKEN_PIN1 = "pin-sha256=\"jdjsjsjs\";";
const GOOD_MAX_AGE = "max-age=69403;";
const INCLUDE_SUBDOMAINS = "includeSubdomains;";
const REPORT_URI = "report-uri=\"https://www.example.com/report/\";";
const UNRECOGNIZED_DIRECTIVE = "unreconized-dir=12343;";

function run_test() {
  Services.prefs.setIntPref("security.cert_pinning.enforcement_level", 2);
  Services.prefs.setBoolPref("security.cert_pinning.process_headers_from_non_builtin_roots", true);

  loadCert("pinningroot", "CTu,CTu,CTu");
  loadCert("badca", "CTu,CTu,CTu");

  checkFailParseInvalidPin("max-age=INVALID");
  
  checkFailParseInvalidPin(GOOD_MAX_AGE);
  checkFailParseInvalidPin(VALID_PIN1);
  checkFailParseInvalidPin(REPORT_URI);
  checkFailParseInvalidPin(UNRECOGNIZED_DIRECTIVE);
  checkFailParseInvalidPin(VALID_PIN1 + BACKUP_PIN1);
  checkFailParseInvalidPin(GOOD_MAX_AGE + VALID_PIN1);
  checkFailParseInvalidPin(GOOD_MAX_AGE + VALID_PIN1 + BROKEN_PIN1);
  
  checkFailParseInvalidPin(GOOD_MAX_AGE + VALID_PIN1 + VALID_PIN1);
  
  checkFailParseInvalidPin(GOOD_MAX_AGE + GOOD_MAX_AGE + VALID_PIN1 + BACKUP_PIN1);
  checkFailParseInvalidPin(GOOD_MAX_AGE + VALID_PIN1 + BACKUP_PIN1 + INCLUDE_SUBDOMAINS + INCLUDE_SUBDOMAINS);
  checkFailParseInvalidPin(GOOD_MAX_AGE + VALID_PIN1 + BACKUP_PIN1 + REPORT_URI + REPORT_URI);
  checkFailParseInvalidPin("thisisinvalidtest");
  checkFailParseInvalidPin("invalid" + GOOD_MAX_AGE + VALID_PIN1 + BACKUP_PIN1);

  checkPassRemovingPin("max-age=0"); 
  checkPassRemovingPin(MAX_AGE_ZERO);
  checkPassRemovingPin(MAX_AGE_ZERO + VALID_PIN1);

  checkPassRemovingPin(VALID_PIN1 + MAX_AGE_ZERO + VALID_PIN1);
  checkPassSettingPin(GOOD_MAX_AGE + VALID_PIN1 + BACKUP_PIN1);
  checkPassSettingPin(GOOD_MAX_AGE + VALID_PIN1 + BACKUP_PIN2);
  checkPassSettingPin(GOOD_MAX_AGE + VALID_PIN1 + BACKUP_PIN2 + INCLUDE_SUBDOMAINS);
  checkPassSettingPin(VALID_PIN1 + GOOD_MAX_AGE + BACKUP_PIN2 + INCLUDE_SUBDOMAINS);
  checkPassSettingPin(VALID_PIN1 + GOOD_MAX_AGE + BACKUP_PIN2 + REPORT_URI + INCLUDE_SUBDOMAINS);
  checkPassSettingPin(INCLUDE_SUBDOMAINS + VALID_PIN1 + GOOD_MAX_AGE + BACKUP_PIN2);
  checkPassSettingPin(GOOD_MAX_AGE + VALID_PIN1 + BACKUP_PIN1 + UNRECOGNIZED_DIRECTIVE);
}
