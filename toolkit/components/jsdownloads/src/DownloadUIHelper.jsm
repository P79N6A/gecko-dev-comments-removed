









"use strict";

this.EXPORTED_SYMBOLS = [
  "DownloadUIHelper",
];




const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "OS",
                                  "resource://gre/modules/osfile.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/commonjs/sdk/core/promise.js");
XPCOMUtils.defineLazyModuleGetter(this, "Services",
                                  "resource://gre/modules/Services.jsm");

const kStringBundleUrl =
  "chrome://browser/locale/downloads/downloads.properties";

const kStringsRequiringFormatting = {
  sizeWithUnits: true,
  shortTimeLeftSeconds: true,
  shortTimeLeftMinutes: true,
  shortTimeLeftHours: true,
  shortTimeLeftDays: true,
  statusSeparator: true,
  statusSeparatorBeforeNumber: true,
  fileExecutableSecurityWarning: true,
};

const kStringsRequiringPluralForm = {
  otherDownloads2: true,
};







this.DownloadUIHelper = {
  













  getPrompter: function (aParent)
  {
    return new DownloadPrompter(aParent || null);
  },
};






XPCOMUtils.defineLazyGetter(DownloadUIHelper, "strings", function () {
  let strings = {};
  let sb = Services.strings.createBundle(kStringBundleUrl);
  let enumerator = sb.getSimpleEnumeration();
  while (enumerator.hasMoreElements()) {
    let string = enumerator.getNext().QueryInterface(Ci.nsIPropertyElement);
    let stringName = string.key;
    if (stringName in kStringsRequiringFormatting) {
      strings[stringName] = function () {
        
        return sb.formatStringFromName(stringName,
                                       Array.slice(arguments, 0),
                                       arguments.length);
      };
    } else if (stringName in kStringsRequiringPluralForm) {
      strings[stringName] = function (aCount) {
        
        let formattedString = sb.formatStringFromName(stringName,
                                       Array.slice(arguments, 0),
                                       arguments.length);
        return PluralForm.get(aCount, formattedString);
      };
    } else {
      strings[stringName] = string.value;
    }
  }
  return strings;
});











function DownloadPrompter(aParent)
{
  this._prompter = Services.ww.getNewPrompter(aParent);
}

DownloadPrompter.prototype = {
  


  _prompter: null,

  











  confirmLaunchExecutable: function (aPath)
  {
    const kPrefAlertOnEXEOpen = "browser.download.manager.alertOnEXEOpen";

    try {
      try {
        if (!Services.prefs.getBoolPref(kPrefAlertOnEXEOpen)) {
          return Promise.resolve(true);
        }
      } catch (ex) {
        
      }

      let leafName = OS.Path.basename(aPath);

      let s = DownloadUIHelper.strings;
      let checkState = { value: false };
      let shouldLaunch = this._prompter.confirmCheck(
                           s.fileExecutableSecurityWarningTitle,
                           s.fileExecutableSecurityWarning(leafName, leafName),
                           s.fileExecutableSecurityWarningDontAsk,
                           checkState);

      if (shouldLaunch) {
        Services.prefs.setBoolPref(kPrefAlertOnEXEOpen, !checkState.value);
      }

      return Promise.resolve(shouldLaunch);
    } catch (ex) {
      return Promise.reject(ex);
    }
  },
};
