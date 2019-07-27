









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
                                  "resource://gre/modules/Promise.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Services",
                                  "resource://gre/modules/Services.jsm");

const kStringBundleUrl =
  "chrome://mozapps/locale/downloads/downloads.properties";

const kStringsRequiringFormatting = {
  fileExecutableSecurityWarning: true,
  cancelDownloadsOKTextMultiple: true,
  quitCancelDownloadsAlertMsgMultiple: true,
  quitCancelDownloadsAlertMsgMacMultiple: true,
  offlineCancelDownloadsAlertMsgMultiple: true,
  leavePrivateBrowsingWindowsCancelDownloadsAlertMsgMultiple2: true
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
    } else {
      strings[stringName] = string.value;
    }
  }
  return strings;
});











this.DownloadPrompter = function (aParent)
{
#ifdef MOZ_B2G
  
  this._prompter = null;
#else
  this._prompter = Services.ww.getNewPrompter(aParent);
#endif
}

this.DownloadPrompter.prototype = {
  


  ON_QUIT: "prompt-on-quit",
  ON_OFFLINE: "prompt-on-offline",
  ON_LEAVE_PRIVATE_BROWSING: "prompt-on-leave-private-browsing",

  


  _prompter: null,

  











  confirmLaunchExecutable: function (aPath)
  {
    const kPrefAlertOnEXEOpen = "browser.download.manager.alertOnEXEOpen";

    try {
      
      if (!this._prompter) {
        return Promise.resolve(true);
      }

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

  











  confirmCancelDownloads: function DP_confirmCancelDownload(aDownloadsCount,
                                                            aPromptType)
  {
    
    
    if (!this._prompter || aDownloadsCount <= 0) {
      return false;
    }

    let s = DownloadUIHelper.strings;
    let buttonFlags = (Ci.nsIPrompt.BUTTON_TITLE_IS_STRING * Ci.nsIPrompt.BUTTON_POS_0) +
                      (Ci.nsIPrompt.BUTTON_TITLE_IS_STRING * Ci.nsIPrompt.BUTTON_POS_1);
    let okButton = aDownloadsCount > 1 ? s.cancelDownloadsOKTextMultiple(aDownloadsCount)
                                       : s.cancelDownloadsOKText;
    let title, message, cancelButton;

    switch (aPromptType) {
      case this.ON_QUIT:
        title = s.quitCancelDownloadsAlertTitle;
#ifndef XP_MACOSX
        message = aDownloadsCount > 1
                  ? s.quitCancelDownloadsAlertMsgMultiple(aDownloadsCount)
                  : s.quitCancelDownloadsAlertMsg;
        cancelButton = s.dontQuitButtonWin;
#else
        message = aDownloadsCount > 1
                  ? s.quitCancelDownloadsAlertMsgMacMultiple(aDownloadsCount)
                  : s.quitCancelDownloadsAlertMsgMac;
        cancelButton = s.dontQuitButtonMac;
#endif
        break;
      case this.ON_OFFLINE:
        title = s.offlineCancelDownloadsAlertTitle;
        message = aDownloadsCount > 1
                  ? s.offlineCancelDownloadsAlertMsgMultiple(aDownloadsCount)
                  : s.offlineCancelDownloadsAlertMsg;
        cancelButton = s.dontGoOfflineButton;
        break;
      case this.ON_LEAVE_PRIVATE_BROWSING:
        title = s.leavePrivateBrowsingCancelDownloadsAlertTitle;
        message = aDownloadsCount > 1
                  ? s.leavePrivateBrowsingWindowsCancelDownloadsAlertMsgMultiple2(aDownloadsCount)
                  : s.leavePrivateBrowsingWindowsCancelDownloadsAlertMsg2;
        cancelButton = s.dontLeavePrivateBrowsingButton2;
        break;
    }

    let rv = this._prompter.confirmEx(title, message, buttonFlags, okButton,
                                      cancelButton, null, null, {});
    return (rv == 1);
  }
};
