# -*- indent-tabs-mode: nil; js-indent-level: 2 -*-
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:

var gPluginHandler = {
  PREF_SESSION_PERSIST_MINUTES: "plugin.sessionPermissionNow.intervalInMinutes",
  PREF_PERSISTENT_DAYS: "plugin.persistentPermissionAlways.intervalInDays",
  MESSAGES: [
    "PluginContent:ShowClickToPlayNotification",
    "PluginContent:RemoveNotification",
    "PluginContent:UpdateHiddenPluginUI",
    "PluginContent:HideNotificationBar",
    "PluginContent:ShowInstallNotification",
    "PluginContent:InstallSinglePlugin",
    "PluginContent:ShowNPAPIPluginCrashedNotification",
    "PluginContent:ShowGMPCrashedNotification",
    "PluginContent:SubmitReport",
    "PluginContent:LinkClickCallback",
  ],

  init: function () {
    const mm = window.messageManager;
    for (let msg of this.MESSAGES) {
      mm.addMessageListener(msg, this);
    }
    window.addEventListener("unload", this);
  },

  uninit: function () {
    const mm = window.messageManager;
    for (let msg of this.MESSAGES) {
      mm.removeMessageListener(msg, this);
    }
    window.removeEventListener("unload", this);
  },

  handleEvent: function (event) {
    if (event.type == "unload") {
      this.uninit();
    }
  },

  receiveMessage: function (msg) {
    switch (msg.name) {
      case "PluginContent:ShowClickToPlayNotification":
        this.showClickToPlayNotification(msg.target, msg.data.plugins, msg.data.showNow,
                                         msg.principal, msg.data.host, msg.data.location);
        break;
      case "PluginContent:RemoveNotification":
        this.removeNotification(msg.target, msg.data.name);
        break;
      case "PluginContent:UpdateHiddenPluginUI":
        this.updateHiddenPluginUI(msg.target, msg.data.haveInsecure, msg.data.actions,
                                  msg.principal, msg.data.host, msg.data.location);
        break;
      case "PluginContent:HideNotificationBar":
        this.hideNotificationBar(msg.target, msg.data.name);
        break;
      case "PluginContent:ShowInstallNotification":
        return this.showInstallNotification(msg.target, msg.data.pluginInfo);
      case "PluginContent:InstallSinglePlugin":
        this.installSinglePlugin(msg.data.pluginInfo);
        break;
      case "PluginContent:ShowNPAPIPluginCrashedNotification":
        this.showNPAPIPluginCrashedNotification(msg.target, msg.data.message,
                                                msg.data.runID);
        break;
      case "PluginContent:ShowGMPCrashedNotification":
        this.showGMPCrashedNotification(msg.target,
                                        msg.data.messageString,
                                        msg.data.pluginDumpID,
                                        msg.data.browserDumpID);
        break;
      case "PluginContent:SubmitReport":
        if (AppConstants.MOZ_CRASHREPORTER) {
          this.submitReport(msg.data.runID, msg.data.keyVals, msg.data.submitURLOptIn);
        }
        break;
      case "PluginContent:LinkClickCallback":
        switch (msg.data.name) {
          case "managePlugins":
          case "openHelpPage":
          case "openPluginUpdatePage":
            this[msg.data.name].apply(this);
            break;
        }
        break;
      default:
        Cu.reportError("gPluginHandler did not expect to handle message " + msg.name);
        break;
    }
  },

#ifdef MOZ_CRASHREPORTER
  get CrashSubmit() {
    delete this.CrashSubmit;
    Cu.import("resource://gre/modules/CrashSubmit.jsm", this);
    return this.CrashSubmit;
  },
#endif

  
  managePlugins: function () {
    BrowserOpenAddonsMgr("addons://list/plugin");
  },

  
  
  openPluginUpdatePage: function () {
    openUILinkIn(Services.urlFormatter.formatURLPref("plugins.update.url"), "tab");
  },

  submitReport: function submitReport(runID, keyVals, submitURLOptIn) {
    if (!AppConstants.MOZ_CRASHREPORTER) {
      return;
    }
    Services.prefs.setBoolPref("dom.ipc.plugins.reportCrashURL", submitURLOptIn);
    PluginCrashReporter.submitCrashReport(runID, keyVals);
  },

  
  reloadPage: function (browser) {
    browser.reload();
  },

  
  openHelpPage: function () {
    openHelpLink("plugin-crashed", false);
  },

  _clickToPlayNotificationEventCallback: function PH_ctpEventCallback(event) {
    if (event == "showing") {
      Services.telemetry.getHistogramById("PLUGINS_NOTIFICATION_SHOWN")
        .add(!this.options.primaryPlugin);
      
      let histogramCount = this.options.pluginData.size - 1;
      if (histogramCount > 4) {
        histogramCount = 4;
      }
      Services.telemetry.getHistogramById("PLUGINS_NOTIFICATION_PLUGIN_COUNT")
        .add(histogramCount);
    }
    else if (event == "dismissed") {
      
      
      this.options.primaryPlugin = null;
    }
  },

  




  _updatePluginPermission: function (aNotification, aPluginInfo, aNewState) {
    let permission;
    let expireType;
    let expireTime;
    let histogram =
      Services.telemetry.getHistogramById("PLUGINS_NOTIFICATION_USER_ACTION");

    
    
    
    switch (aNewState) {
      case "allownow":
        permission = Ci.nsIPermissionManager.ALLOW_ACTION;
        expireType = Ci.nsIPermissionManager.EXPIRE_SESSION;
        expireTime = Date.now() + Services.prefs.getIntPref(this.PREF_SESSION_PERSIST_MINUTES) * 60 * 1000;
        histogram.add(0);
        aPluginInfo.fallbackType = Ci.nsIObjectLoadingContent.PLUGIN_ACTIVE;
        break;

      case "allowalways":
        permission = Ci.nsIPermissionManager.ALLOW_ACTION;
        expireType = Ci.nsIPermissionManager.EXPIRE_TIME;
        expireTime = Date.now() +
          Services.prefs.getIntPref(this.PREF_PERSISTENT_DAYS) * 24 * 60 * 60 * 1000;
        histogram.add(1);
        aPluginInfo.fallbackType = Ci.nsIObjectLoadingContent.PLUGIN_ACTIVE;
        break;

      case "block":
        permission = Ci.nsIPermissionManager.PROMPT_ACTION;
        expireType = Ci.nsIPermissionManager.EXPIRE_NEVER;
        expireTime = 0;
        histogram.add(2);
        switch (aPluginInfo.blocklistState) {
          case Ci.nsIBlocklistService.STATE_VULNERABLE_UPDATE_AVAILABLE:
            aPluginInfo.fallbackType = Ci.nsIObjectLoadingContent.PLUGIN_VULNERABLE_UPDATABLE;
            break;
          case Ci.nsIBlocklistService.STATE_VULNERABLE_NO_UPDATE:
            aPluginInfo.fallbackType = Ci.nsIObjectLoadingContent.PLUGIN_VULNERABLE_NO_UPDATE;
            break;
          default:
            aPluginInfo.fallbackType = Ci.nsIObjectLoadingContent.PLUGIN_CLICK_TO_PLAY;
        }
        break;

      
      
      case "continue":
        aPluginInfo.fallbackType = Ci.nsIObjectLoadingContent.PLUGIN_ACTIVE;
        break;
      default:
        Cu.reportError(Error("Unexpected plugin state: " + aNewState));
        return;
    }

    let browser = aNotification.browser;
    let contentWindow = browser.contentWindow;
    if (aNewState != "continue") {
      let principal = aNotification.options.principal;
      Services.perms.addFromPrincipal(principal, aPluginInfo.permissionString,
                                      permission, expireType, expireTime);
      aPluginInfo.pluginPermissionType = expireType;
    }

    browser.messageManager.sendAsyncMessage("BrowserPlugins:ActivatePlugins", {
      pluginInfo: aPluginInfo,
      newState: aNewState,
    });
  },

  showClickToPlayNotification: function (browser, plugins, showNow, principal,
                                         host, location) {
    
    
    
    
    
    if (!principal.equals(browser.contentPrincipal)) {
      return;
    }

    
    
    
    
    let receivedURI = BrowserUtils.makeURI(location);
    if (!browser.documentURI.equalsExceptRef(receivedURI)) {
      return;
    }

    let notification = PopupNotifications.getNotification("click-to-play-plugins", browser);

    
    let pluginData;
    if (notification) {
      pluginData = notification.options.pluginData;
    } else {
      pluginData = new Map();
    }

    for (var pluginInfo of plugins) {
      if (pluginData.has(pluginInfo.permissionString)) {
        continue;
      }

      
      
      let url = Services.blocklist.getPluginInfoURL(pluginInfo.pluginTag);

      if (pluginInfo.blocklistState == Ci.nsIBlocklistService.STATE_VULNERABLE_UPDATE_AVAILABLE) {
        if (!url) {
          url = Services.urlFormatter.formatURLPref("plugins.update.url");
        }
      }
      else if (pluginInfo.blocklistState != Ci.nsIBlocklistService.STATE_NOT_BLOCKED) {
        if (!url) {
          url = Services.blocklist.getPluginBlocklistURL(pluginInfo.pluginTag);
        }
      }
      else {
        url = Services.urlFormatter.formatURLPref("app.support.baseURL") + "clicktoplay";
      }
      pluginInfo.detailsLink = url;

      pluginData.set(pluginInfo.permissionString, pluginInfo);
    }

    let primaryPluginPermission = null;
    if (showNow) {
      primaryPluginPermission = plugins[0].permissionString;
    }

    if (notification) {
      
      
      if (showNow) {
        notification.options.primaryPlugin = primaryPluginPermission;
        notification.reshow();
        browser.messageManager.sendAsyncMessage("BrowserPlugins:NotificationShown");
      }
      return;
    }

    let options = {
      dismissed: !showNow,
      eventCallback: this._clickToPlayNotificationEventCallback,
      primaryPlugin: primaryPluginPermission,
      pluginData: pluginData,
      principal: principal,
      host: host,
    };
    PopupNotifications.show(browser, "click-to-play-plugins",
                            "", "plugins-notification-icon",
                            null, null, options);
    browser.messageManager.sendAsyncMessage("BrowserPlugins:NotificationShown");
  },

  removeNotification: function (browser, name) {
    let notification = PopupNotifications.getNotification(name, browser);
    if (notification)
      PopupNotifications.remove(notification);
  },

  hideNotificationBar: function (browser, name) {
    let notificationBox = gBrowser.getNotificationBox(browser);
    let notification = notificationBox.getNotificationWithValue(name);
    if (notification)
      notificationBox.removeNotification(notification, true);
  },

  updateHiddenPluginUI: function (browser, haveInsecure, actions, principal,
                                  host, location) {
    
    
    
    
    
    if (!principal.equals(browser.contentPrincipal)) {
      return;
    }

    
    
    
    
    let receivedURI = BrowserUtils.makeURI(location);
    if (!browser.documentURI.equalsExceptRef(receivedURI)) {
      return;
    }

    
    document.getElementById("plugins-notification-icon").classList.
      toggle("plugin-blocked", haveInsecure);

    
    let notificationBox = gBrowser.getNotificationBox(browser);

    function hideNotification() {
      let n = notificationBox.getNotificationWithValue("plugin-hidden");
      if (n) {
        notificationBox.removeNotification(n, true);
      }
    }

    
    
    
    
    
    
    
    function showNotification() {
      let n = notificationBox.getNotificationWithValue("plugin-hidden");
      if (n) {
        
        return;
      }

      Services.telemetry.getHistogramById("PLUGINS_INFOBAR_SHOWN").
        add(true);

      let message;
      
      
      let brand = document.getElementById("bundle_brand").getString("brandShortName");

      if (actions.length == 1) {
        let pluginInfo = actions[0];
        let pluginName = pluginInfo.pluginName;

        switch (pluginInfo.fallbackType) {
          case Ci.nsIObjectLoadingContent.PLUGIN_CLICK_TO_PLAY:
            message = gNavigatorBundle.getFormattedString(
              "pluginActivateNew.message",
              [pluginName, host]);
            break;
          case Ci.nsIObjectLoadingContent.PLUGIN_VULNERABLE_UPDATABLE:
            message = gNavigatorBundle.getFormattedString(
              "pluginActivateOutdated.message",
              [pluginName, host, brand]);
            break;
          case Ci.nsIObjectLoadingContent.PLUGIN_VULNERABLE_NO_UPDATE:
            message = gNavigatorBundle.getFormattedString(
              "pluginActivateVulnerable.message",
              [pluginName, host, brand]);
        }
      } else {
        
        message = gNavigatorBundle.getFormattedString(
          "pluginActivateMultiple.message", [host]);
      }

      let buttons = [
        {
          label: gNavigatorBundle.getString("pluginContinueBlocking.label"),
          accessKey: gNavigatorBundle.getString("pluginContinueBlocking.accesskey"),
          callback: function() {
            Services.telemetry.getHistogramById("PLUGINS_INFOBAR_BLOCK").
              add(true);

            Services.perms.addFromPrincipal(principal,
                                            "plugin-hidden-notification",
                                            Services.perms.DENY_ACTION);
          }
        },
        {
          label: gNavigatorBundle.getString("pluginActivateTrigger.label"),
          accessKey: gNavigatorBundle.getString("pluginActivateTrigger.accesskey"),
          callback: function() {
            Services.telemetry.getHistogramById("PLUGINS_INFOBAR_ALLOW").
              add(true);

            let curNotification =
              PopupNotifications.getNotification("click-to-play-plugins",
                                                 browser);
            if (curNotification) {
              curNotification.reshow();
            }
          }
        }
      ];
      n = notificationBox.
        appendNotification(message, "plugin-hidden", null,
                           notificationBox.PRIORITY_INFO_HIGH, buttons);
      if (haveInsecure) {
        n.classList.add('pluginVulnerable');
      }
    }

    if (actions.length == 0) {
      hideNotification();
    } else {
      let notificationPermission = Services.perms.testPermissionFromPrincipal(
        principal, "plugin-hidden-notification");
      if (notificationPermission == Ci.nsIPermissionManager.DENY_ACTION) {
        hideNotification();
      } else {
        showNotification();
      }
    }
  },

  contextMenuCommand: function (browser, plugin, command) {
    browser.messageManager.sendAsyncMessage("BrowserPlugins:ContextMenuCommand",
      { command: command }, { plugin: plugin });
  },

  
  
  NPAPIPluginCrashed : function(subject, topic, data) {
    let propertyBag = subject;
    if (!(propertyBag instanceof Ci.nsIPropertyBag2) ||
        !(propertyBag instanceof Ci.nsIWritablePropertyBag2) ||
        !propertyBag.hasKey("runID") ||
        !propertyBag.hasKey("pluginName")) {
      Cu.reportError("A NPAPI plugin crashed, but the properties of this plugin " +
                     "cannot be read.");
      return;
    }

    let runID = propertyBag.getPropertyAsUint32("runID");
    let uglyPluginName = propertyBag.getPropertyAsAString("pluginName");
    let pluginName = BrowserUtils.makeNicePluginName(uglyPluginName);
    let pluginDumpID = propertyBag.getPropertyAsAString("pluginDumpID");

    
    
    let state;
    if (!AppConstants.MOZ_CRASHREPORTER || !gCrashReporter.enabled) {
      
      
      state = "noSubmit";
    } else if (!pluginDumpID) {
      
      state = "noReport";
    } else {
      
      state = "please";
    }

    let mm = window.getGroupMessageManager("browsers");
    mm.broadcastAsyncMessage("BrowserPlugins:NPAPIPluginProcessCrashed",
                             { pluginName, runID, state });
  },

  showNPAPIPluginCrashedNotification: function (browser, messageString, runID) {
    let crashReportCallback;

    if (AppConstants.MOZ_CRASHREPORTER &&
        PluginCrashReporter.hasCrashReport(runID)) {
      crashReportCallback = () => {
        PluginCrashReporter.submitGMPCrashReport(pluginDumpID, browserDumpID);
      };
    }

    this._showPluginCrashedNotification(browser, messageString, crashReportCallback);
  },

  




  showGMPCrashedNotification: function (browser, messageString,
                                        pluginDumpID, browserDumpID) {
    let crashReportCallback;

    if (AppConstants.MOZ_CRASHREPORTER && pluginDumpID) {
      crashReportCallback = () => {
        PluginCrashReporter.submitGMPCrashReport(pluginDumpID, browserDumpID);
      };
    }

    this._showPluginCrashedNotification(browser, messageString, crashReportCallback);
  },

  











  _showPluginCrashedNotification: function (browser, messageString, crashReportCallback) {
    
    let notificationBox = gBrowser.getNotificationBox(browser);
    let notification = notificationBox.getNotificationWithValue("plugin-crashed");
    if (notification)
      return;

    
    let priority = notificationBox.PRIORITY_WARNING_MEDIUM;
    let iconURL = "chrome://mozapps/skin/plugins/notifyPluginCrashed.png";
    let reloadLabel = gNavigatorBundle.getString("crashedpluginsMessage.reloadButton.label");
    let reloadKey   = gNavigatorBundle.getString("crashedpluginsMessage.reloadButton.accesskey");
    let submitLabel = gNavigatorBundle.getString("crashedpluginsMessage.submitButton.label");
    let submitKey   = gNavigatorBundle.getString("crashedpluginsMessage.submitButton.accesskey");

    let buttons = [{
      label: reloadLabel,
      accessKey: reloadKey,
      popup: null,
      callback: function() { browser.reload(); },
    }];

    if (AppConstants.MOZ_CRASHREPORTER && crashReportCallback) {
      let submitButton = {
        label: submitLabel,
        accessKey: submitKey,
        popup: null,
        callback: crashReportCallback,
      };

      buttons.push(submitButton);
    }

    notification = notificationBox.appendNotification(messageString, "plugin-crashed",
                                                      iconURL, priority, buttons);

    
    let XULNS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
    let link = notification.ownerDocument.createElementNS(XULNS, "label");
    link.className = "text-link";
    link.setAttribute("value", gNavigatorBundle.getString("crashedpluginsMessage.learnMore"));
    let crashurl = formatURL("app.support.baseURL", true);
    crashurl += "plugin-crashed-notificationbar";
    link.href = crashurl;

    let description = notification.ownerDocument.getAnonymousElementByAttribute(notification, "anonid", "messageText");
    description.appendChild(link);
  },
};

gPluginHandler.init();

