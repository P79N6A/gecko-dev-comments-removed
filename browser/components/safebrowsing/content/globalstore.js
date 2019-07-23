# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http:
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is Google Safe Browsing.
#
# The Initial Developer of the Original Code is Google Inc.
# Portions created by the Initial Developer are Copyright (C) 2006
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Fritz Schneider <fritz@google.com> (original author)
#   J. Paul Reed <preed@mozilla.com>
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****

















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

  
  this.prefs_.addObserver(kPhishWardenRemoteLookups,
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

  
  this.lookupURL_ = this.getUrlPref_(basePref + "lookupURL");
  this.keyURL_ = this.getUrlPref_(basePref + "keyURL");
  this.reportURL_ = this.getUrlPref_(basePref + "reportURL");

  
  this.reportGenericURL_ = this.getUrlPref_(basePref + "reportGenericURL");
  this.reportErrorURL_ = this.getUrlPref_(basePref + "reportErrorURL");
  this.reportPhishURL_ = this.getUrlPref_(basePref + "reportPhishURL");

  
  this.updateListManager_();
}





PROT_DataProvider.prototype.updateListManager_ = function() {
  var listManager = Cc["@mozilla.org/url-classifier/listmanager;1"]
                      .getService(Ci.nsIUrlListManager);

  
  
  listManager.setUpdateUrl(this.getUpdateURL());

  
  
  
  var isEnabled = this.prefs_.getPref(kPhishWardenEnabledPref, false);
  var remoteLookups = this.prefs_.getPref(kPhishWardenRemoteLookups, false);
  if (isEnabled && remoteLookups) {
    listManager.setKeyUrl(this.getKeyURL());
  } else {
    
    listManager.setKeyUrl("");
  }
}




PROT_DataProvider.prototype.getUrlPref_ = function(prefName) {
  var url = this.prefs_.getPref(prefName);

  var appInfo = Components.classes["@mozilla.org/xre/app-info;1"]
                          .getService(Components.interfaces.nsIXULAppInfo);

  var mozClientStr = MOZ_OFFICIAL_BUILD ? 'navclient-auto-ffox' : appInfo.name;

  
  url = url.replace(MOZ_PARAM_LOCALE, this.getLocale_());
  url = url.replace(MOZ_PARAM_CLIENT, mozClientStr);
  url = url.replace(MOZ_PARAM_BUILDID, appInfo.appBuildID);
  url = url.replace(MOZ_PARAM_VERSION, appInfo.version);
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

PROT_DataProvider.prototype.getLookupURL = function() {
  return this.lookupURL_;
}
PROT_DataProvider.prototype.getKeyURL = function() {
  return this.keyURL_;
}
PROT_DataProvider.prototype.getReportURL = function() {
  return this.reportURL_;
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
