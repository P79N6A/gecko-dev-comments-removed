




































EXPORTED_SYMBOLS = ["MetadataCollector"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://testpilot/modules/string_sanitizer.js");

const LOCALE_PREF = "general.useragent.locale";
const EXTENSION_ID = "testpilot@labs.mozilla.com";
const PREFIX_NS_EM = "http://www.mozilla.org/2004/em-rdf#";
const PREFIX_ITEM_URI = "urn:mozilla:item:";
const UPDATE_CHANNEL_PREF = "app.update.channel";




const SURVEY_ANS = "extensions.testpilot.surveyAnswers.basic_panel_survey_2";

let Application = Cc["@mozilla.org/fuel/application;1"]
                  .getService(Ci.fuelIApplication);


function Weave_sha1(string) {
  let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"].
                  createInstance(Ci.nsIScriptableUnicodeConverter);
  converter.charset = "UTF-8";

  let hasher = Cc["@mozilla.org/security/hash;1"]
               .createInstance(Ci.nsICryptoHash);
  hasher.init(hasher.SHA1);

  let data = converter.convertToByteArray(string, {});
  hasher.update(data, data.length);
  let rawHash = hasher.finish(false);

  
  function toHexString(charCode) {
    return ("0" + charCode.toString(16)).slice(-2);
  }
  let hash = [toHexString(rawHash.charCodeAt(i)) for (i in rawHash)].join("");
  return hash;
}

let MetadataCollector = {
  
  getExtensions: function MetadataCollector_getExtensions(callback) {
    
    
    let myExtensions = [];
    if (Application.extensions) {
      for each (let ex in Application.extensions.all) {
        myExtensions.push({ id: Weave_sha1(ex.id), isEnabled: ex.enabled });
      }
      callback(myExtensions);
    } else {
      Application.getExtensions(function(extensions) {
        for each (let ex in extensions.all) {
          myExtensions.push({ id: Weave_sha1(ex.id), isEnabled: ex.enabled });
        }
        callback(myExtensions);
      });
    }
  },

  getAccessibilities : function MetadataCollector_getAccessibilities() {
    let prefs =
      Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefService);
    let branch = prefs.getBranch("accessibility.");
    let accessibilities = [];
    let children = branch.getChildList("", {});
    let length = children.length;
    let prefName;
    let prefValue;

    for (let i = 0; i < length; i++) {
      prefName = "accessibility." + children[i];
      prefValue =
        Application.prefs.getValue(prefName, "");
      accessibilities.push({ name: prefName, value: prefValue });
    }

    return accessibilities;
  },

  getLocation: function MetadataCollector_getLocation() {
    
    
    return Application.prefs.getValue(LOCALE_PREF, "");
  },

  getVersion: function MetadataCollector_getVersion() {
    return Application.version;
  },

  getOperatingSystem: function MetadataCollector_getOSVersion() {
    let oscpu = Cc["@mozilla.org/network/protocol;1?name=http"].getService(Ci.nsIHttpProtocolHandler).oscpu;
    let os = Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsIXULRuntime).OS;
    return os + " " + oscpu;
  },

  getSurveyAnswers: function MetadataCollector_getSurveyAnswers() {
    let answers = Application.prefs.getValue(SURVEY_ANS, "");
    if (answers == "") {
      return "";
    } else {
      return sanitizeJSONStrings( JSON.parse(answers) );
    }
  },

  getTestPilotVersion: function MetadataCollector_getTPVersion(callback) {
    
    if (Application.extensions) {
      callback(Application.extensions.get(EXTENSION_ID).version);
    } else {
      Application.getExtensions(function(extensions) {
        callback(extensions.get(EXTENSION_ID).version);
      });
    }
  },

  getUpdateChannel: function MetadataCollector_getUpdateChannel() {
    return Application.prefs.getValue(UPDATE_CHANNEL_PREF, "");
  },

  getMetadata: function MetadataCollector_getMetadata(callback) {
    let self = this;
    self.getTestPilotVersion(function(tpVersion) {
      self.getExtensions(function(extensions) {
        callback({ extensions: extensions,
                   accessibilities: self.getAccessibilities(),
	           location: self.getLocation(),
	           fxVersion: self.getVersion(),
                   operatingSystem: self.getOperatingSystem(),
                   tpVersion: tpVersion,
                   surveyAnswers: self.getSurveyAnswers(),
                   updateChannel: self.getUpdateChannel()}
                 );
      });
    });
  }
  
};
