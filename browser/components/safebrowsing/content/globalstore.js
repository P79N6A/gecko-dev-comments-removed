# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:

















const kDataProviderIdPref = 'browser.safebrowsing.dataProvider';
const kProviderBasePref = 'browser.safebrowsing.provider.';

#ifdef OFFICIAL_BUILD
const MOZ_OFFICIAL_BUILD = true;
#else
const MOZ_OFFICIAL_BUILD = false;
#endif

const MOZ_PARAM_LOCALE = /\{moz:locale\}/g;
const MOZ_PARAM_CLIENT = /\{moz:client\}/g;
const MOZ_PARAM_BUILDID = /\{moz:buildid\}/g;
const MOZ_PARAM_VERSION = /\{moz:version\}/g;




function PROT_DataProvider() {
  this.prefs_ = new G_Preferences();

  this.loadDataProviderPrefs_();
  
  
  this.prefs_.addObserver(kDataProviderIdPref,
                          BindToObject(this.loadDataProviderPrefs_, this));

  
  this.prefs_.addObserver(kPhishWardenEnabledPref,
                          BindToObject(this.loadDataProviderPrefs_, this));
}





PROT_DataProvider.prototype.loadDataProviderPrefs_ = function() {
  
  
  this.updateURL_ = this.getUrlPref_(
        'browser.safebrowsing.provider.0.updateURL');

  var id = this.prefs_.getPref(kDataProviderIdPref, null);

  
  if (null == id)
    id = 0;
  
  var basePref = kProviderBasePref + id + '.';

  this.name_ = this.prefs_.getPref(basePref + "name", "");

  
  this.keyURL_ = this.getUrlPref_(basePref + "keyURL");
  this.reportURL_ = this.getUrlPref_(basePref + "reportURL");
  this.gethashURL_ = this.getUrlPref_(basePref + "gethashURL");

  
  this.reportGenericURL_ = this.getUrlPref_(basePref + "reportGenericURL");
  this.reportErrorURL_ = this.getUrlPref_(basePref + "reportErrorURL");
  this.reportPhishURL_ = this.getUrlPref_(basePref + "reportPhishURL");
  this.reportMalwareURL_ = this.getUrlPref_(basePref + "reportMalwareURL")
  this.reportMalwareErrorURL_ = this.getUrlPref_(basePref + "reportMalwareErrorURL")

  
  this.updateListManager_();
}





PROT_DataProvider.prototype.updateListManager_ = function() {
  var listManager = Cc["@mozilla.org/url-classifier/listmanager;1"]
                      .getService(Ci.nsIUrlListManager);

  
  
  listManager.setUpdateUrl(this.getUpdateURL());

  
  
  var isEnabled = this.prefs_.getPref(kPhishWardenEnabledPref, false) ||
                  this.prefs_.getPref(kMalwareWardenEnabledPref, false);
  if (isEnabled) {
    listManager.setKeyUrl(this.keyURL_);
  }

  listManager.setGethashUrl(this.getGethashURL());
}




PROT_DataProvider.prototype.getUrlPref_ = function(prefName) {
  var url = this.prefs_.getPref(prefName);

  var appInfo = Components.classes["@mozilla.org/xre/app-info;1"]
                          .getService(Components.interfaces.nsIXULAppInfo);

  var mozClientStr = this.prefs_.getPref("browser.safebrowsing.clientid",
                                         MOZ_OFFICIAL_BUILD ? 'navclient-auto-ffox' : appInfo.name);

  var versionStr = this.prefs_.getPref("browser.safebrowsing.clientver",
                                       appInfo.version);

  
  url = url.replace(MOZ_PARAM_LOCALE, this.getLocale_());
  url = url.replace(MOZ_PARAM_CLIENT, mozClientStr);
  url = url.replace(MOZ_PARAM_BUILDID, appInfo.appBuildID);
  url = url.replace(MOZ_PARAM_VERSION, versionStr);
  return url;
}




PROT_DataProvider.prototype.getLocale_ = function() {
  const localePref = "general.useragent.locale";
  var locale = this.getLocalizedPref_(localePref);
  if (locale)
    return locale;

  
  var prefs = new G_Preferences();
  return prefs.getPref(localePref, "");
}




PROT_DataProvider.prototype.getLocalizedPref_ = function(aPrefName) {
  
  
  var prefs = Cc["@mozilla.org/preferences-service;1"]
              .getService(Ci.nsIPrefBranch);
  try {
    return prefs.getComplexValue(aPrefName, Ci.nsIPrefLocalizedString).data;
  } catch (ex) {
  }
  return "";
}



PROT_DataProvider.prototype.getName = function() {
  return this.name_;
}

PROT_DataProvider.prototype.getUpdateURL = function() {
  return this.updateURL_;
}

PROT_DataProvider.prototype.getGethashURL = function() {
  return this.gethashURL_;
}

PROT_DataProvider.prototype.getReportGenericURL = function() {
  return this.reportGenericURL_;
}
PROT_DataProvider.prototype.getReportErrorURL = function() {
  return this.reportErrorURL_;
}
PROT_DataProvider.prototype.getReportPhishURL = function() {
  return this.reportPhishURL_;
}
PROT_DataProvider.prototype.getReportMalwareURL = function() {
  return this.reportMalwareURL_;
}
PROT_DataProvider.prototype.getReportMalwareErrorURL = function() {
  return this.reportMalwareErrorURL_;
}
