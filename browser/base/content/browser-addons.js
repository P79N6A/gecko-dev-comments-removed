# -*- indent-tabs-mode: nil; js-indent-level: 2 -*-
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:

const gXPInstallObserver = {
  _findChildShell: function (aDocShell, aSoughtShell)
  {
    if (aDocShell == aSoughtShell)
      return aDocShell;

    var node = aDocShell.QueryInterface(Components.interfaces.nsIDocShellTreeItem);
    for (var i = 0; i < node.childCount; ++i) {
      var docShell = node.getChildAt(i);
      docShell = this._findChildShell(docShell, aSoughtShell);
      if (docShell == aSoughtShell)
        return docShell;
    }
    return null;
  },

  _getBrowser: function (aDocShell)
  {
    for (let browser of gBrowser.browsers) {
      if (this._findChildShell(browser.docShell, aDocShell))
        return browser;
    }
    return null;
  },

  pendingInstalls: new WeakMap(),

  showInstallConfirmation: function(browser, installInfo, height = undefined) {
    
    
    if (PopupNotifications.getNotification("addon-install-confirmation", browser)) {
      let pending = this.pendingInstalls.get(browser);
      if (pending) {
        pending.push(installInfo);
      } else {
        this.pendingInstalls.set(browser, [installInfo]);
      }
      return;
    }

    const anchorID = "addons-notification-icon";

    
    var options = {
      displayURI: installInfo.originatingURI,
      timeout: Date.now() + 30000,
    };

    let cancelInstallation = () => {
      if (installInfo) {
        for (let install of installInfo.installs)
          install.cancel();
      }

      this.acceptInstallation = null;

      let tab = gBrowser.getTabForBrowser(browser);
      if (tab)
        tab.removeEventListener("TabClose", cancelInstallation);

      window.removeEventListener("unload", cancelInstallation);

      
      if (gBrowser.browsers.indexOf(browser) == -1)
        return;

      let pending = this.pendingInstalls.get(browser);
      if (pending && pending.length)
        this.showInstallConfirmation(browser, pending.shift());
    };

    let unsigned = installInfo.installs.filter(i => i.addon.signedState <= AddonManager.SIGNEDSTATE_MISSING);
    let someUnsigned = unsigned.length > 0 && unsigned.length < installInfo.installs.length;

    options.eventCallback = (aEvent) => {
      switch (aEvent) {
        case "removed":
          cancelInstallation();
          break;
        case "shown":
          let addonList = document.getElementById("addon-install-confirmation-content");
          while (addonList.firstChild)
            addonList.firstChild.remove();

          for (let install of installInfo.installs) {
            let container = document.createElement("hbox");

            let name = document.createElement("label");
            name.setAttribute("value", install.addon.name);
            name.setAttribute("class", "addon-install-confirmation-name");
            container.appendChild(name);

            if (someUnsigned && install.addon.signedState <= AddonManager.SIGNEDSTATE_MISSING) {
              let unsigned = document.createElement("label");
              unsigned.setAttribute("value", gNavigatorBundle.getString("addonInstall.unsigned"));
              unsigned.setAttribute("class", "addon-install-confirmation-unsigned");
              container.appendChild(unsigned);
            }

            addonList.appendChild(container);
          }

          this.acceptInstallation = () => {
            for (let install of installInfo.installs)
              install.install();
            installInfo = null;

            Services.telemetry
                    .getHistogramById("SECURITY_UI")
                    .add(Ci.nsISecurityUITelemetry.WARNING_CONFIRM_ADDON_INSTALL_CLICK_THROUGH);
          };
          break;
      }
    };

    options.learnMoreURL = Services.urlFormatter.formatURLPref("app.support.baseURL");

    let messageString;
    let notification = document.getElementById("addon-install-confirmation-notification");
    if (unsigned.length == installInfo.installs.length) {
      
      messageString = gNavigatorBundle.getString("addonConfirmInstallUnsigned.message");
      notification.setAttribute("warning", "true");
      options.learnMoreURL += "unsigned-addons";
    }
    else if (unsigned.length == 0) {
      
      messageString = gNavigatorBundle.getString("addonConfirmInstall.message");
      notification.removeAttribute("warning");
      options.learnMoreURL += "find-and-install-add-ons";
    }
    else {
      
      
      messageString = gNavigatorBundle.getString("addonConfirmInstallSomeUnsigned.message");
      notification.setAttribute("warning", "true");
      options.learnMoreURL += "unsigned-addons";
    }

    let brandBundle = document.getElementById("bundle_brand");
    let brandShortName = brandBundle.getString("brandShortName");

    messageString = PluralForm.get(installInfo.installs.length, messageString);
    messageString = messageString.replace("#1", brandShortName);
    messageString = messageString.replace("#2", installInfo.installs.length);

    let cancelButton = document.getElementById("addon-install-confirmation-cancel");
    cancelButton.label = gNavigatorBundle.getString("addonInstall.cancelButton.label");
    cancelButton.accessKey = gNavigatorBundle.getString("addonInstall.cancelButton.accesskey");

    let acceptButton = document.getElementById("addon-install-confirmation-accept");
    acceptButton.label = gNavigatorBundle.getString("addonInstall.acceptButton.label");
    acceptButton.accessKey = gNavigatorBundle.getString("addonInstall.acceptButton.accesskey");

    if (height) {
      let notification = document.getElementById("addon-install-confirmation-notification");
      notification.style.minHeight = height + "px";
    }

    let tab = gBrowser.getTabForBrowser(browser);
    if (tab) {
      gBrowser.selectedTab = tab;
      tab.addEventListener("TabClose", cancelInstallation);
    }

    window.addEventListener("unload", cancelInstallation);

    PopupNotifications.show(browser, "addon-install-confirmation", messageString,
                            anchorID, null, null, options);

    Services.telemetry
            .getHistogramById("SECURITY_UI")
            .add(Ci.nsISecurityUITelemetry.WARNING_CONFIRM_ADDON_INSTALL);
  },

  observe: function (aSubject, aTopic, aData)
  {
    var brandBundle = document.getElementById("bundle_brand");
    var installInfo = aSubject.QueryInterface(Components.interfaces.amIWebInstallInfo);
    var browser = installInfo.browser;

    
    if (!browser || gBrowser.browsers.indexOf(browser) == -1)
      return;

    const anchorID = "addons-notification-icon";
    var messageString, action;
    var brandShortName = brandBundle.getString("brandShortName");

    var notificationID = aTopic;
    
    var options = {
      displayURI: installInfo.originatingURI,
      timeout: Date.now() + 30000,
    };

    switch (aTopic) {
    case "addon-install-disabled": {
      notificationID = "xpinstall-disabled";

      if (gPrefService.prefIsLocked("xpinstall.enabled")) {
        messageString = gNavigatorBundle.getString("xpinstallDisabledMessageLocked");
        buttons = [];
      }
      else {
        messageString = gNavigatorBundle.getString("xpinstallDisabledMessage");

        action = {
          label: gNavigatorBundle.getString("xpinstallDisabledButton"),
          accessKey: gNavigatorBundle.getString("xpinstallDisabledButton.accesskey"),
          callback: function editPrefs() {
            gPrefService.setBoolPref("xpinstall.enabled", true);
          }
        };
      }

      PopupNotifications.show(browser, notificationID, messageString, anchorID,
                              action, null, options);
      break; }
    case "addon-install-blocked": {
      messageString = gNavigatorBundle.getFormattedString("xpinstallPromptMessage",
                        [brandShortName]);

      let secHistogram = Components.classes["@mozilla.org/base/telemetry;1"].getService(Ci.nsITelemetry).getHistogramById("SECURITY_UI");
      action = {
        label: gNavigatorBundle.getString("xpinstallPromptAllowButton"),
        accessKey: gNavigatorBundle.getString("xpinstallPromptAllowButton.accesskey"),
        callback: function() {
          secHistogram.add(Ci.nsISecurityUITelemetry.WARNING_ADDON_ASKING_PREVENTED_CLICK_THROUGH);
          installInfo.install();
        }
      };

      secHistogram.add(Ci.nsISecurityUITelemetry.WARNING_ADDON_ASKING_PREVENTED);
      PopupNotifications.show(browser, notificationID, messageString, anchorID,
                              action, null, options);
      break; }
    case "addon-install-started": {
      let needsDownload = function needsDownload(aInstall) {
        return aInstall.state != AddonManager.STATE_DOWNLOADED;
      }
      
      
      if (!installInfo.installs.some(needsDownload))
        return;
      notificationID = "addon-progress";
      messageString = gNavigatorBundle.getString("addonDownloadingAndVerifying");
      messageString = PluralForm.get(installInfo.installs.length, messageString);
      messageString = messageString.replace("#1", installInfo.installs.length);
      options.installs = installInfo.installs;
      options.contentWindow = browser.contentWindow;
      options.sourceURI = browser.currentURI;
      options.eventCallback = (aEvent) => {
        switch (aEvent) {
          case "removed":
            options.contentWindow = null;
            options.sourceURI = null;
            break;
        }
      };
      let notification = PopupNotifications.show(browser, notificationID, messageString,
                                                 anchorID, null, null, options);
      notification._startTime = Date.now();

      let cancelButton = document.getElementById("addon-progress-cancel");
      cancelButton.label = gNavigatorBundle.getString("addonInstall.cancelButton.label");
      cancelButton.accessKey = gNavigatorBundle.getString("addonInstall.cancelButton.accesskey");

      let acceptButton = document.getElementById("addon-progress-accept");
      if (Preferences.get("xpinstall.customConfirmationUI", false)) {
        acceptButton.label = gNavigatorBundle.getString("addonInstall.acceptButton.label");
        acceptButton.accessKey = gNavigatorBundle.getString("addonInstall.acceptButton.accesskey");
      } else {
        acceptButton.hidden = true;
      }
      break; }
    case "addon-install-failed": {
      
      for (let install of installInfo.installs) {
        let host;
        try {
          host  = options.displayURI.host;
        } catch (e) {
          
        }

        if (!host)
          host = (install.sourceURI instanceof Ci.nsIStandardURL) &&
                 install.sourceURI.host;

        let error = (host || install.error == 0) ? "addonInstallError" : "addonLocalInstallError";
        let args;
        if (install.error < 0) {
          error += install.error;
          args = [brandShortName, install.name];
        } else if (install.addon.blocklistState == Ci.nsIBlocklistService.STATE_BLOCKED) {
          error += "Blocklisted";
          args = [install.name];
        } else {
          error += "Incompatible";
          args = [brandShortName, Services.appinfo.version, install.name];
        }

        
        if (install.error == AddonManager.ERROR_SIGNEDSTATE_REQUIRED) {
          options.learnMoreURL = Services.urlFormatter.formatURLPref("app.support.baseURL") + "unsigned-addons";
        }

        messageString = gNavigatorBundle.getFormattedString(error, args);

        PopupNotifications.show(browser, notificationID, messageString, anchorID,
                                action, null, options);

        
        break;
      }
      this._removeProgressNotification(browser);
      break; }
    case "addon-install-confirmation": {
      let showNotification = () => {
        let height = undefined;

        if (PopupNotifications.isPanelOpen) {
          let rect = document.getElementById("addon-progress-notification").getBoundingClientRect();
          height = rect.height;
        }

        this._removeProgressNotification(browser);
        this.showInstallConfirmation(browser, installInfo, height);
      };

      let progressNotification = PopupNotifications.getNotification("addon-progress", browser);
      if (progressNotification) {
        let downloadDuration = Date.now() - progressNotification._startTime;
        let securityDelay = Services.prefs.getIntPref("security.dialog_enable_delay") - downloadDuration;
        if (securityDelay > 0) {
          setTimeout(() => {
            
            if (PopupNotifications.getNotification("addon-progress", browser))
              showNotification();
          }, securityDelay);
          break;
        }
      }
      showNotification();
      break; }
    case "addon-install-complete": {
      let needsRestart = installInfo.installs.some(function(i) {
        return i.addon.pendingOperations != AddonManager.PENDING_NONE;
      });

      if (needsRestart) {
        notificationID = "addon-install-restart";
        messageString = gNavigatorBundle.getString("addonsInstalledNeedsRestart");
        action = {
          label: gNavigatorBundle.getString("addonInstallRestartButton"),
          accessKey: gNavigatorBundle.getString("addonInstallRestartButton.accesskey"),
          callback: function() {
            BrowserUtils.restartApplication();
          }
        };
      }
      else {
        messageString = gNavigatorBundle.getString("addonsInstalled");
        action = null;
      }

      messageString = PluralForm.get(installInfo.installs.length, messageString);
      messageString = messageString.replace("#1", installInfo.installs[0].name);
      messageString = messageString.replace("#2", installInfo.installs.length);
      messageString = messageString.replace("#3", brandShortName);

      
      
      
      options.removeOnDismissal = true;

      PopupNotifications.show(browser, notificationID, messageString, anchorID,
                              action, null, options);
      break; }
    }
  },
  _removeProgressNotification(aBrowser) {
    let notification = PopupNotifications.getNotification("addon-progress", aBrowser);
    if (notification)
      notification.remove();
  }
};

var LightWeightThemeWebInstaller = {
  init: function () {
    let mm = window.messageManager;
    mm.addMessageListener("LightWeightThemeWebInstaller:Install", this);
    mm.addMessageListener("LightWeightThemeWebInstaller:Preview", this);
    mm.addMessageListener("LightWeightThemeWebInstaller:ResetPreview", this);
  },

  receiveMessage: function (message) {
    
    if (message.target != gBrowser.selectedBrowser) {
      return;
    }

    let data = message.data;

    switch (message.name) {
      case "LightWeightThemeWebInstaller:Install": {
        this._installRequest(data.themeData, data.baseURI);
        break;
      }
      case "LightWeightThemeWebInstaller:Preview": {
        this._preview(data.themeData, data.baseURI);
        break;
      }
      case "LightWeightThemeWebInstaller:ResetPreview": {
        this._resetPreview(data && data.baseURI);
        break;
      }
    }
  },

  handleEvent: function (event) {
    switch (event.type) {
      case "TabSelect": {
        this._resetPreview();
        break;
      }
    }
  },

  get _manager () {
    let temp = {};
    Cu.import("resource://gre/modules/LightweightThemeManager.jsm", temp);
    delete this._manager;
    return this._manager = temp.LightweightThemeManager;
  },

  _installRequest: function (dataString, baseURI) {
    let data = this._manager.parseTheme(dataString, baseURI);

    if (!data) {
      return;
    }

    if (this._isAllowed(baseURI)) {
      this._install(data);
      return;
    }

    let allowButtonText =
      gNavigatorBundle.getString("lwthemeInstallRequest.allowButton");
    let allowButtonAccesskey =
      gNavigatorBundle.getString("lwthemeInstallRequest.allowButton.accesskey");
    let message =
      gNavigatorBundle.getFormattedString("lwthemeInstallRequest.message",
                                          [makeURI(baseURI).host]);
    let buttons = [{
      label: allowButtonText,
      accessKey: allowButtonAccesskey,
      callback: function () {
        LightWeightThemeWebInstaller._install(data);
      }
    }];

    this._removePreviousNotifications();

    let notificationBox = gBrowser.getNotificationBox();
    let notificationBar =
      notificationBox.appendNotification(message, "lwtheme-install-request", "",
                                         notificationBox.PRIORITY_INFO_MEDIUM,
                                         buttons);
    notificationBar.persistence = 1;
  },

  _install: function (newLWTheme) {
    let previousLWTheme = this._manager.currentTheme;

    let listener = {
      onEnabling: function(aAddon, aRequiresRestart) {
        if (!aRequiresRestart) {
          return;
        }

        let messageString = gNavigatorBundle.getFormattedString("lwthemeNeedsRestart.message",
          [aAddon.name], 1);

        let action = {
          label: gNavigatorBundle.getString("lwthemeNeedsRestart.button"),
          accessKey: gNavigatorBundle.getString("lwthemeNeedsRestart.accesskey"),
          callback: function () {
            BrowserUtils.restartApplication();
          }
        };

        let options = {
          timeout: Date.now() + 30000
        };

        PopupNotifications.show(gBrowser.selectedBrowser, "addon-theme-change",
                                messageString, "addons-notification-icon",
                                action, null, options);
      },

      onEnabled: function(aAddon) {
        LightWeightThemeWebInstaller._postInstallNotification(newLWTheme, previousLWTheme);
      }
    };

    AddonManager.addAddonListener(listener);
    this._manager.currentTheme = newLWTheme;
    AddonManager.removeAddonListener(listener);
  },

  _postInstallNotification: function (newTheme, previousTheme) {
    function text(id) {
      return gNavigatorBundle.getString("lwthemePostInstallNotification." + id);
    }

    let buttons = [{
      label: text("undoButton"),
      accessKey: text("undoButton.accesskey"),
      callback: function () {
        LightWeightThemeWebInstaller._manager.forgetUsedTheme(newTheme.id);
        LightWeightThemeWebInstaller._manager.currentTheme = previousTheme;
      }
    }, {
      label: text("manageButton"),
      accessKey: text("manageButton.accesskey"),
      callback: function () {
        BrowserOpenAddonsMgr("addons://list/theme");
      }
    }];

    this._removePreviousNotifications();

    let notificationBox = gBrowser.getNotificationBox();
    let notificationBar =
      notificationBox.appendNotification(text("message"),
                                         "lwtheme-install-notification", "",
                                         notificationBox.PRIORITY_INFO_MEDIUM,
                                         buttons);
    notificationBar.persistence = 1;
    notificationBar.timeout = Date.now() + 20000; 
  },

  _removePreviousNotifications: function () {
    let box = gBrowser.getNotificationBox();

    ["lwtheme-install-request",
     "lwtheme-install-notification"].forEach(function (value) {
        let notification = box.getNotificationWithValue(value);
        if (notification)
          box.removeNotification(notification);
      });
  },

  _preview: function (dataString, baseURI) {
    if (!this._isAllowed(baseURI))
      return;

    let data = this._manager.parseTheme(dataString, baseURI);
    if (!data)
      return;

    this._resetPreview();
    gBrowser.tabContainer.addEventListener("TabSelect", this, false);
    this._manager.previewTheme(data);
  },

  _resetPreview: function (baseURI) {
    if (baseURI && !this._isAllowed(baseURI))
      return;
    gBrowser.tabContainer.removeEventListener("TabSelect", this, false);
    this._manager.resetPreview();
  },

  _isAllowed: function (srcURIString) {
    let uri;
    try {
      uri = makeURI(srcURIString);
    }
    catch(e) {
      
      return false;
    }

    if (!uri.schemeIs("https")) {
      return false;
    }

    let pm = Services.perms;
    return pm.testPermission(uri, "install") == pm.ALLOW_ACTION;
  }
};




let LightweightThemeListener = {
  _modifiedStyles: [],

  init: function () {
    XPCOMUtils.defineLazyGetter(this, "styleSheet", function() {
      for (let i = document.styleSheets.length - 1; i >= 0; i--) {
        let sheet = document.styleSheets[i];
        if (sheet.href == "chrome://browser/skin/browser-lightweightTheme.css")
          return sheet;
      }
    });

    Services.obs.addObserver(this, "lightweight-theme-styling-update", false);
    Services.obs.addObserver(this, "lightweight-theme-optimized", false);
    if (document.documentElement.hasAttribute("lwtheme"))
      this.updateStyleSheet(document.documentElement.style.backgroundImage);
  },

  uninit: function () {
    Services.obs.removeObserver(this, "lightweight-theme-styling-update");
    Services.obs.removeObserver(this, "lightweight-theme-optimized");
  },

  





  updateStyleSheet: function(headerImage) {
    if (!this.styleSheet)
      return;
    this.substituteRules(this.styleSheet.cssRules, headerImage);
  },

  substituteRules: function(ruleList, headerImage, existingStyleRulesModified = 0) {
    let styleRulesModified = 0;
    for (let i = 0; i < ruleList.length; i++) {
      let rule = ruleList[i];
      if (rule instanceof Ci.nsIDOMCSSGroupingRule) {
        
        styleRulesModified += this.substituteRules(rule.cssRules, headerImage, existingStyleRulesModified + styleRulesModified);
      } else if (rule instanceof Ci.nsIDOMCSSStyleRule) {
        if (!rule.style.backgroundImage)
          continue;
        let modifiedIndex = existingStyleRulesModified + styleRulesModified;
        if (!this._modifiedStyles[modifiedIndex])
          this._modifiedStyles[modifiedIndex] = { backgroundImage: rule.style.backgroundImage };

        rule.style.backgroundImage = this._modifiedStyles[modifiedIndex].backgroundImage + ", " + headerImage;
        styleRulesModified++;
      } else {
        Cu.reportError("Unsupported rule encountered");
      }
    }
    return styleRulesModified;
  },

  
  observe: function (aSubject, aTopic, aData) {
    if ((aTopic != "lightweight-theme-styling-update" && aTopic != "lightweight-theme-optimized") ||
          !this.styleSheet)
      return;

    if (aTopic == "lightweight-theme-optimized" && aSubject != window)
      return;

    let themeData = JSON.parse(aData);
    if (!themeData)
      return;
    this.updateStyleSheet("url(" + themeData.headerURL + ")");
  },
};
