












const FILE_SIMPLE_NO_PIB_MAR = "simple_no_pib.mar";
const SIZE_SIMPLE_NO_PIB_MAR = "351";
const MD5_HASH_SIMPLE_NO_PIB_MAR    = "d0a7f84dacc55a252ab916668a7cb216";
const SHA1_HASH_SIMPLE_NO_PIB_MAR   = "f5053f9552d087c6c6ed83f9b19405eccf1436" +
                                      "fc";
const SHA256_HASH_SIMPLE_NO_PIB_MAR = "663c7cbd11fe45b0a71438387db924d205997a" +
                                      "b85ccf5b40aebbdaef179796ab";
const SHA384_HASH_SIMPLE_NO_PIB_MAR = "a57250554755a9f42b91932993599bb6b05e06" +
                                      "3dcbd71846e350232945dbad2b0c83208a0781" +
                                      "0cf798b3d1139399c453";
const SHA512_HASH_SIMPLE_NO_PIB_MAR = "55d3e2a86acaeb0abb7a444c13bba748846fcb" +
                                      "ac7ff058f8ee9c9260ba01e6aef86fa4a6c46a" +
                                      "3016b675ef94e77e63fbe912f64d155bed9b1c" +
                                      "341dd56e575a26";

const FILE_SIMPLE_MAR = "simple.mar";
const SIZE_SIMPLE_MAR = "735";
const MD5_HASH_SIMPLE_MAR    = "025228705a76117b5499a22d9eed9271";
const SHA1_HASH_SIMPLE_MAR   = "87a9b1f57b3ff9bb20b677c5c0924b46b6f6781e";
const SHA256_HASH_SIMPLE_MAR = "443ef6a8ddcd2af88f9b72b1732d335454a483127766a" +
                               "30ca3a5647d2ae159da";
const SHA384_HASH_SIMPLE_MAR = "c8a493e50569c190a1c67793be80f0f9d13b08d839d79" +
                               "dd8d7be2859cfd2f75b242f272a8fbbcbe4419d399259" +
                               "35d732";
const SHA512_HASH_SIMPLE_MAR = "9f9afbb5a6fa70e71fa28c73c5b1968cf8831ae02ebd4" +
                               "28839ed2acf0b257c08c76c1f286c94c029dd3a4ad708" +
                               "f9703f61e6bf5f30982e9deee7d6838c1dbe14";

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

var gTestserverPort;








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
  if (!aUpdates || aUpdates == "")
    return "<updates xmlns=\"http://www.mozilla.org/2005/app-update\"/>"
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
  let billboardURL = aBillboardURL ? "billboardURL=\"" +
                                     aBillboardURL + "\" "
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
  let url = aURL ? aURL : URL_HOST +
                          (gTestserverPort ? ":" + gTestserverPort : "") +
                          "/" + URL_PATH + "/" + FILE_SIMPLE_MAR;
  let hashFunction = aHashFunction ? aHashFunction : "MD5";
  let hashValue = aHashValue ? aHashValue : MD5_HASH_SIMPLE_MAR;
  let size = aSize ? aSize : SIZE_SIMPLE_MAR;
  return "    <patch type=\"" + type + "\" " +
                     "URL=\"" + url + "\" " +
                     "hashFunction=\"" + hashFunction + "\" " +
                     "hashValue=\"" + hashValue + "\" " +
                     "size=\"" + size + "\"";
}
