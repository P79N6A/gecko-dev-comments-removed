# -*- indent-tabs-mode: nil; js-indent-level: 2 -*-
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:

let gEMEHandler = {
  ensureEMEEnabled: function(browser, keySystem) {
    Services.prefs.setBoolPref("media.eme.enabled", true);
    if (keySystem) {
      if (keySystem.startsWith("com.adobe") &&
          Services.prefs.getPrefType("media.gmp-eme-adobe.enabled") &&
          !Services.prefs.getBoolPref("media.gmp-eme-adobe.enabled")) {
        Services.prefs.setBoolPref("media.gmp-eme-adobe.enabled", true);
      } else if (keySystem == "org.w3.clearkey" &&
                 Services.prefs.getPrefType("media.eme.clearkey.enabled") &&
                 !Services.prefs.getBoolPref("media.eme.clearkey.enabled")) {
        Services.prefs.setBoolPref("media.eme.clearkey.enabled", true);
      }
    }
    browser.reload();
  },
  getLearnMoreLink: function(msgId) {
    let text = gNavigatorBundle.getString("emeNotifications." + msgId + ".learnMoreLabel");
    let baseURL = Services.urlFormatter.formatURLPref("app.support.baseURL");
    return "<label class='text-link' href='" + baseURL + "drm-content'>" +
           text + "</label>";
  },
  onDontAskAgain: function(menuPopupItem) {
    let button = menuPopupItem.parentNode.anchorNode;
    let bar = button.parentNode;
    Services.prefs.setBoolPref("browser.eme.ui." + bar.value + ".disabled", true);
    bar.close();
  },
  onNotNow: function(menuPopupItem) {
    let button = menuPopupItem.parentNode.anchorNode;
    button.parentNode.close();
  },
  receiveMessage: function({target: browser, data: data}) {
    let parsedData;
    try {
      parsedData = JSON.parse(data);
    } catch (ex) {
      Cu.reportError("Malformed EME video message with data: " + data);
      return;
    }
    let {status: status, keySystem: keySystem} = parsedData;
    
    if (!Services.prefs.getBoolPref("browser.eme.ui.enabled")) {
      return;
    }

    let notificationId;
    let buttonCallback;
    let params = [];
    switch (status) {
      case "available":
      case "cdm-created":
        this.showPopupNotificationForSuccess(browser, keySystem);
        
        return;

      case "api-disabled":
      case "cdm-disabled":
        notificationId = "drmContentDisabled";
        buttonCallback = gEMEHandler.ensureEMEEnabled.bind(gEMEHandler, browser, keySystem)
        params = [this.getLearnMoreLink(notificationId)];
        break;

      case "cdm-not-supported":
        notificationId = "drmContentCDMNotSupported";
        params = [this._brandShortName, this.getLearnMoreLink(notificationId)];
        break;

      case "cdm-insufficient-version":
        notificationId = "drmContentCDMInsufficientVersion";
        params = [this._brandShortName];
        break;

      case "cdm-not-installed":
        notificationId = "drmContentCDMInstalling";
        params = [this._brandShortName];
        break;

      case "error":
        
      default:
        let typeOfIssue = status == "error" ? "error" : "message ('" + status + "')";
        Cu.reportError("Unknown " + typeOfIssue + " dealing with EME key request: " + data);
        return;
    }

    this.showNotificationBar(browser, notificationId, keySystem, params, buttonCallback);
  },
  showNotificationBar: function(browser, notificationId, keySystem, labelParams, callback) {
    let box = gBrowser.getNotificationBox(browser);
    if (box.getNotificationWithValue(notificationId)) {
      return;
    }

    
    try {
      if (Services.prefs.getBoolPref("browser.eme.ui." + notificationId + ".disabled")) {
        return;
      }
    } catch (ex) {  }

    let msgPrefix = "emeNotifications." + notificationId + ".";
    let msgId = msgPrefix + "message";

    
    if (notificationId == "drmContentCDMNotSupported" &&
        keySystem.startsWith("com.adobe")) {
      let os = Services.appinfo.OS.toLowerCase();
      if (os.startsWith("win") && Services.appinfo.XPCOMABI.startsWith("x86_64")) {
        msgId = msgPrefix + "64bit.message";
      } else if (os.startsWith("linux") || os.startsWith("darwin")) {
        msgId = msgPrefix + "unsupportedOS.message";
        labelParams.splice(1, 0, os.startsWith("linux") ? "Linux" : "Mac OS X");
      }
    }

    let message = labelParams.length ?
                  gNavigatorBundle.getFormattedString(msgId, labelParams) :
                  gNavigatorBundle.getString(msgId);

    let buttons = [];
    if (callback) {
      let btnLabelId = msgPrefix + "button.label";
      let btnAccessKeyId = msgPrefix + "button.accesskey";
      buttons.push({
        label: gNavigatorBundle.getString(btnLabelId),
        accessKey: gNavigatorBundle.getString(btnAccessKeyId),
        callback: callback
      });

      let optionsId = "emeNotifications.optionsButton";
      buttons.push({
        label: gNavigatorBundle.getString(optionsId + ".label"),
        accessKey: gNavigatorBundle.getString(optionsId + ".accesskey"),
        popup: "emeNotificationsPopup"
      });
    }

    let iconURL = "chrome://browser/skin/drm-icon.svg#chains-black";

    
    let fragment = document.createDocumentFragment();
    let descriptionContainer = document.createElement("description");
    descriptionContainer.innerHTML = message;
    while (descriptionContainer.childNodes.length) {
      fragment.appendChild(descriptionContainer.childNodes[0]);
    }

    box.appendNotification(fragment, notificationId, iconURL, box.PRIORITY_WARNING_MEDIUM,
                           buttons);
  },
  showPopupNotificationForSuccess: function(browser, keySystem) {
    
    var box = gBrowser.getNotificationBox(browser);
    ["drmContentDisabled",
     "drmContentCDMNotSupported",
     "drmContentCDMInsufficientVersion",
     "drmContentCDMInstalling"
     ].forEach(function (value) {
        var notification = box.getNotificationWithValue(value);
        if (notification)
          box.removeNotification(notification);
      });

    
    if (PopupNotifications.getNotification("drmContentPlaying", browser)) {
      return;
    }

    let msgPrefix = "emeNotifications.drmContentPlaying.";
    let msgId = msgPrefix + "message2";
    let btnLabelId = msgPrefix + "button.label";
    let btnAccessKeyId = msgPrefix + "button.accesskey";

    let message = gNavigatorBundle.getFormattedString(msgId, [this._brandShortName]);
    let anchorId = "eme-notification-icon";
    let firstPlayPref = "browser.eme.ui.firstContentShown";
    if (!Services.prefs.getPrefType(firstPlayPref) ||
        !Services.prefs.getBoolPref(firstPlayPref)) {
      document.getElementById(anchorId).setAttribute("firstplay", "true");
      Services.prefs.setBoolPref(firstPlayPref, true);
    } else {
      document.getElementById(anchorId).removeAttribute("firstplay");
    }


    let mainAction = {
      label: gNavigatorBundle.getString(btnLabelId),
      accessKey: gNavigatorBundle.getString(btnAccessKeyId),
      callback: function() { openPreferences("paneContent"); },
      dismiss: true
    };
    let options = {
      dismissed: true,
      eventCallback: aTopic => aTopic == "swapping",
    };
    PopupNotifications.show(browser, "drmContentPlaying", message, anchorId, mainAction, null, options);
  },
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIMessageListener])
};

XPCOMUtils.defineLazyGetter(gEMEHandler, "_brandShortName", function() {
  return document.getElementById("bundle_brand").getString("brandShortName");
});

window.messageManager.addMessageListener("EMEVideo:ContentMediaKeysRequest", gEMEHandler);
window.addEventListener("unload", function() {
  window.messageManager.removeMessageListener("EMEVideo:ContentMediaKeysRequest", gEMEHandler);
}, false);
