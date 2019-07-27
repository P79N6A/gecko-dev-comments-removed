












const FILE_SIMPLE_MAR = "simple.mar";
const SIZE_SIMPLE_MAR = "1031";
const MD5_HASH_SIMPLE_MAR    = "1f8c038577bb6845d94ccec4999113ee";
const SHA1_HASH_SIMPLE_MAR   = "5d49a672c87f10f31d7e326349564a11272a028b";
const SHA256_HASH_SIMPLE_MAR = "1aabbed5b1dd6e16e139afc5b43d479e254e0c26" +
                               "3c8fb9249c0a1bb93071c5fb";
const SHA384_HASH_SIMPLE_MAR = "26615014ea034af32ef5651492d5f493f5a7a1a48522e" +
                               "d24c366442a5ec21d5ef02e23fb58d79729b8ca2f9541" +
                               "99dd53";
const SHA512_HASH_SIMPLE_MAR = "922e5ae22081795f6e8d65a3c508715c9a314054179a8" +
                               "bbfe5f50dc23919ad89888291bc0a07586ab17dd0304a" +
                               "b5347473601127571c66f61f5080348e05c36b";

const STATE_NONE            = "null";
const STATE_DOWNLOADING     = "downloading";
const STATE_PENDING         = "pending";
const STATE_PENDING_SVC     = "pending-service";
const STATE_APPLYING        = "applying";
const STATE_APPLIED         = "applied";
const STATE_APPLIED_SVC     = "applied-service";
const STATE_SUCCEEDED       = "succeeded";
const STATE_DOWNLOAD_FAILED = "download-failed";
const STATE_FAILED          = "failed";

const STATE_FAILED_READ_ERROR                      = STATE_FAILED + ": 6";
const STATE_FAILED_WRITE_ERROR                     = STATE_FAILED + ": 7";
const STATE_FAILED_CHANNEL_MISMATCH_ERROR          = STATE_FAILED + ": 22";
const STATE_FAILED_VERSION_DOWNGRADE_ERROR         = STATE_FAILED + ": 23";
const STATE_FAILED_UNEXPECTED_FILE_OPERATION_ERROR = STATE_FAILED + ": 42";








function getRemoteUpdatesXMLString(aUpdates) {
  return "<?xml version=\"1.0\"?>\n" +
         "<updates>\n" +
           aUpdates +
         "</updates>\n";
}









function getRemoteUpdateString(aPatches, aType, aName, aDisplayVersion,
                               aAppVersion, aPlatformVersion, aBuildID,
                               aDetailsURL, aBillboardURL, aLicenseURL,
                               aShowPrompt, aShowNeverForVersion, aPromptWaitTime,
                               aShowSurvey, aVersion, aExtensionVersion, aCustom1,
                               aCustom2) {
  return getUpdateString(aType, aName, aDisplayVersion, aAppVersion,
                         aPlatformVersion, aBuildID, aDetailsURL,
                         aBillboardURL, aLicenseURL, aShowPrompt,
                         aShowNeverForVersion, aPromptWaitTime, aShowSurvey,
                         aVersion, aExtensionVersion, aCustom1, aCustom2) + ">\n" +
              aPatches +
         "  </update>\n";
}







function getRemotePatchString(aType, aURL, aHashFunction, aHashValue, aSize) {
  return getPatchString(aType, aURL, aHashFunction, aHashValue, aSize) +
         "/>\n";
}








function getLocalUpdatesXMLString(aUpdates) {
  if (!aUpdates || aUpdates == "") {
    return "<updates xmlns=\"http://www.mozilla.org/2005/app-update\"/>"
  }
  return ("<updates xmlns=\"http://www.mozilla.org/2005/app-update\">" +
            aUpdates +
          "</updates>").replace(/>\s+\n*</g,'><');
}



























function getLocalUpdateString(aPatches, aType, aName, aDisplayVersion,
                              aAppVersion, aPlatformVersion, aBuildID,
                              aDetailsURL, aBillboardURL, aLicenseURL,
                              aServiceURL, aInstallDate, aStatusText,
                              aIsCompleteUpdate, aChannel, aForegroundDownload,
                              aShowPrompt, aShowNeverForVersion, aPromptWaitTime,
                              aShowSurvey, aVersion, aExtensionVersion,
                              aPreviousAppVersion, aCustom1, aCustom2) {
  let serviceURL = aServiceURL ? aServiceURL : "http://test_service/";
  let installDate = aInstallDate ? aInstallDate : "1238441400314";
  let statusText = aStatusText ? aStatusText : "Install Pending";
  let isCompleteUpdate =
    typeof(aIsCompleteUpdate) == "string" ? aIsCompleteUpdate : "true";
  let channel = aChannel ? aChannel
                         : gDefaultPrefBranch.getCharPref(PREF_APP_UPDATE_CHANNEL);
  let foregroundDownload =
    typeof(aForegroundDownload) == "string" ? aForegroundDownload : "true";
  let previousAppVersion = aPreviousAppVersion ? "previousAppVersion=\"" +
                                                 aPreviousAppVersion + "\" "
                                               : "";
  return getUpdateString(aType, aName, aDisplayVersion, aAppVersion,
                         aPlatformVersion, aBuildID, aDetailsURL, aBillboardURL,
                         aLicenseURL, aShowPrompt, aShowNeverForVersion,
                         aPromptWaitTime, aShowSurvey, aVersion, aExtensionVersion,
                         aCustom1, aCustom2) +
                   " " +
                   previousAppVersion +
                   "serviceURL=\"" + serviceURL + "\" " +
                   "installDate=\"" + installDate + "\" " +
                   "statusText=\"" + statusText + "\" " +
                   "isCompleteUpdate=\"" + isCompleteUpdate + "\" " +
                   "channel=\"" + channel + "\" " +
                   "foregroundDownload=\"" + foregroundDownload + "\">"  +
              aPatches +
         "  </update>";
}














function getLocalPatchString(aType, aURL, aHashFunction, aHashValue, aSize,
                             aSelected, aState) {
  let selected = typeof(aSelected) == "string" ? aSelected : "true";
  let state = aState ? aState : STATE_SUCCEEDED;
  return getPatchString(aType, aURL, aHashFunction, aHashValue, aSize) + " " +
         "selected=\"" + selected + "\" " +
         "state=\"" + state + "\"/>\n";
}



































































function getUpdateString(aType, aName, aDisplayVersion, aAppVersion,
                         aPlatformVersion, aBuildID, aDetailsURL, aBillboardURL,
                         aLicenseURL, aShowPrompt, aShowNeverForVersion,
                         aPromptWaitTime, aShowSurvey, aVersion, aExtensionVersion,
                         aCustom1, aCustom2) {
  let type = aType ? aType : "major";
  let name = aName ? aName : "App Update Test";
  let displayVersion = "";
  if (aDisplayVersion || !aVersion) {
    displayVersion = "displayVersion=\"" +
                     (aDisplayVersion ? aDisplayVersion
                                      : "version " + DEFAULT_UPDATE_VERSION) +
                     "\" ";
  }
  
  
  let version = aVersion ? "version=\"" + aVersion + "\" " : "";
  let appVersion = "";
  if (aAppVersion || !aExtensionVersion) {
    appVersion = "appVersion=\"" +
                 (aAppVersion ? aAppVersion : DEFAULT_UPDATE_VERSION) +
                 "\" ";
  }
  
  
  let extensionVersion = aExtensionVersion ? "extensionVersion=\"" +
                                             aExtensionVersion + "\" "
                                           : "";
  let platformVersion = "";
  if (aPlatformVersion) {
    platformVersion = "platformVersion=\"" +
                      (aPlatformVersion ? aPlatformVersion
                                        : DEFAULT_UPDATE_VERSION) + "\" ";
  }
  let buildID = aBuildID ? aBuildID : "20080811053724";
  

  let detailsURL = "detailsURL=\"" +
                   (aDetailsURL ? aDetailsURL
                                : "http://test_details/") + "\" ";
  let billboardURL = aBillboardURL ? "billboardURL=\"" + aBillboardURL + "\" "
                                   : "";
  let licenseURL = aLicenseURL ? "licenseURL=\"" + aLicenseURL + "\" " : "";
  let showPrompt = aShowPrompt ? "showPrompt=\"" + aShowPrompt + "\" " : "";
  let showNeverForVersion = aShowNeverForVersion ? "showNeverForVersion=\"" +
                                                   aShowNeverForVersion + "\" "
                                                 : "";
  let promptWaitTime = aPromptWaitTime ? "promptWaitTime=\"" + aPromptWaitTime +
                                         "\" "
                                       : "";
  let custom1 = aCustom1 ? aCustom1 + " " : "";
  let custom2 = aCustom2 ? aCustom2 + " " : "";
  return "  <update type=\"" + type + "\" " +
                   "name=\"" + name + "\" " +
                    displayVersion +
                    version +
                    appVersion +
                    extensionVersion +
                    platformVersion +
                    detailsURL +
                    billboardURL +
                    licenseURL +
                    showPrompt +
                    showNeverForVersion +
                    promptWaitTime +
                    custom1 +
                    custom2 +
                   "buildID=\"" + buildID + "\"";
}
























function getPatchString(aType, aURL, aHashFunction, aHashValue, aSize) {
  let type = aType ? aType : "complete";
  let url = aURL ? aURL : gURLData + FILE_SIMPLE_MAR;
  let hashFunction = aHashFunction ? aHashFunction : "MD5";
  let hashValue = aHashValue ? aHashValue : MD5_HASH_SIMPLE_MAR;
  let size = aSize ? aSize : SIZE_SIMPLE_MAR;
  return "    <patch type=\"" + type + "\" " +
                     "URL=\"" + url + "\" " +
                     "hashFunction=\"" + hashFunction + "\" " +
                     "hashValue=\"" + hashValue + "\" " +
                     "size=\"" + size + "\"";
}
