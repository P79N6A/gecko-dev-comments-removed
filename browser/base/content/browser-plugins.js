# -*- Mode: javascript; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:

function getPluginInfo(pluginElement)
{
  var tagMimetype;
  var pluginsPage;
  if (pluginElement instanceof HTMLAppletElement) {
    tagMimetype = "application/x-java-vm";
  } else {
    if (pluginElement instanceof HTMLObjectElement) {
      pluginsPage = pluginElement.getAttribute("codebase");
    } else {
      pluginsPage = pluginElement.getAttribute("pluginspage");
    }

    
    if (pluginsPage) {
      var doc = pluginElement.ownerDocument;
      var docShell = findChildShell(doc, gBrowser.docShell, null);
      try {
        pluginsPage = makeURI(pluginsPage, doc.characterSet, docShell.currentURI).spec;
      } catch (ex) {
        pluginsPage = "";
      }
    }

    tagMimetype = pluginElement.QueryInterface(Components.interfaces.nsIObjectLoadingContent)
                               .actualType;

    if (tagMimetype == "") {
      tagMimetype = pluginElement.type;
    }
  }

  return {mimetype: tagMimetype, pluginsPage: pluginsPage};
}

var gPluginHandler = {

#ifdef MOZ_CRASHREPORTER
  get CrashSubmit() {
    delete this.CrashSubmit;
    Cu.import("resource://gre/modules/CrashSubmit.jsm", this);
    return this.CrashSubmit;
  },
#endif

  
  makeNicePluginName : function (aName, aFilename) {
    if (aName == "Shockwave Flash")
      return "Adobe Flash";

    
    
    let newName = aName.replace(/\bplug-?in\b/i, "").replace(/[\s\d\.\-\_\(\)]+$/, "");
    return newName;
  },

  isTooSmall : function (plugin, overlay) {
    
    let pluginRect = plugin.getBoundingClientRect();
    
    
    let overflows = (overlay.scrollWidth > pluginRect.width) ||
                    (overlay.scrollHeight - 5 > pluginRect.height);
    return overflows;
  },

  addLinkClickCallback: function (linkNode, callbackName ) {
    
    let self = this;
    let callbackArgs = Array.prototype.slice.call(arguments).slice(2);
    linkNode.addEventListener("click",
                              function(evt) {
                                if (!evt.isTrusted)
                                  return;
                                evt.preventDefault();
                                if (callbackArgs.length == 0)
                                  callbackArgs = [ evt ];
                                (self[callbackName]).apply(self, callbackArgs);
                              },
                              true);

    linkNode.addEventListener("keydown",
                              function(evt) {
                                if (!evt.isTrusted)
                                  return;
                                if (evt.keyCode == evt.DOM_VK_RETURN) {
                                  evt.preventDefault();
                                  if (callbackArgs.length == 0)
                                    callbackArgs = [ evt ];
                                  evt.preventDefault();
                                  (self[callbackName]).apply(self, callbackArgs);
                                }
                              },
                              true);
  },

  handleEvent : function(event) {
    let self = gPluginHandler;
    let plugin = event.target;
    let doc = plugin.ownerDocument;

    
    if (!(plugin instanceof Ci.nsIObjectLoadingContent))
      return;

    
    plugin.clientTop;

    switch (event.type) {
      case "PluginCrashed":
        self.pluginInstanceCrashed(plugin, event);
        break;

      case "PluginNotFound":
        
        
        
        if (!(plugin instanceof HTMLObjectElement)) {
          
          let installStatus = doc.getAnonymousElementByAttribute(plugin, "class", "installStatus");
          installStatus.setAttribute("status", "ready");
          let iconStatus = doc.getAnonymousElementByAttribute(plugin, "class", "icon");
          iconStatus.setAttribute("status", "ready");

          let installLink = doc.getAnonymousElementByAttribute(plugin, "class", "installPluginLink");
          self.addLinkClickCallback(installLink, "installSinglePlugin", plugin);
        }
        

      case "PluginBlocklisted":
      case "PluginOutdated":
#ifdef XP_MACOSX
      case "npapi-carbon-event-model-failure":
#endif
        self.pluginUnavailable(plugin, event.type);
        break;

      case "PluginVulnerableUpdatable":
        let updateLink = doc.getAnonymousElementByAttribute(plugin, "class", "checkForUpdatesLink");
        self.addLinkClickCallback(updateLink, "openPluginUpdatePage");
        

      case "PluginVulnerableNoUpdate":
      case "PluginClickToPlay":
        self._handleClickToPlayEvent(plugin);
        break;

      case "PluginPlayPreview":
        self._handlePlayPreviewEvent(plugin);
        break;

      case "PluginDisabled":
        let manageLink = doc.getAnonymousElementByAttribute(plugin, "class", "managePluginsLink");
        self.addLinkClickCallback(manageLink, "managePlugins");
        break;
    }

    
    if (event.type != "PluginCrashed") {
      let overlay = doc.getAnonymousElementByAttribute(plugin, "class", "mainBox");
      
      if (overlay != null && self.isTooSmall(plugin, overlay))
          overlay.style.visibility = "hidden";
    }
  },

  canActivatePlugin: function PH_canActivatePlugin(objLoadingContent) {
    return !objLoadingContent.activated &&
           objLoadingContent.pluginFallbackType !== Ci.nsIObjectLoadingContent.PLUGIN_PLAY_PREVIEW;
  },

  activatePlugins: function PH_activatePlugins(aContentWindow) {
    let browser = gBrowser.getBrowserForDocument(aContentWindow.document);
    browser._clickToPlayPluginsActivated = true;
    let cwu = aContentWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                            .getInterface(Ci.nsIDOMWindowUtils);
    let plugins = cwu.plugins;
    for (let plugin of plugins) {
      let objLoadingContent = plugin.QueryInterface(Ci.nsIObjectLoadingContent);
      if (gPluginHandler.canActivatePlugin(objLoadingContent))
        objLoadingContent.playPlugin();
    }
    let notification = PopupNotifications.getNotification("click-to-play-plugins", browser);
    if (notification)
      notification.remove();
  },

  activateSinglePlugin: function PH_activateSinglePlugin(aContentWindow, aPlugin) {
    let objLoadingContent = aPlugin.QueryInterface(Ci.nsIObjectLoadingContent);
    if (gPluginHandler.canActivatePlugin(objLoadingContent))
      objLoadingContent.playPlugin();

    let cwu = aContentWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                            .getInterface(Ci.nsIDOMWindowUtils);
    let haveUnplayedPlugins = cwu.plugins.some(function(plugin) {
      let objLoadingContent = plugin.QueryInterface(Ci.nsIObjectLoadingContent);
      return (plugin != aPlugin && gPluginHandler.canActivatePlugin(objLoadingContent));
    });
    let browser = gBrowser.getBrowserForDocument(aContentWindow.document);
    let notification = PopupNotifications.getNotification("click-to-play-plugins", browser);
    if (notification && !haveUnplayedPlugins) {
      browser._clickToPlayDoorhangerShown = false;
      notification.remove();
    }
  },

  stopPlayPreview: function PH_stopPlayPreview(aPlugin, aPlayPlugin) {
    let objLoadingContent = aPlugin.QueryInterface(Ci.nsIObjectLoadingContent);
    if (objLoadingContent.activated)
      return;

    if (aPlayPlugin)
      objLoadingContent.playPlugin();
    else
      objLoadingContent.cancelPlayPreview();
  },

  newPluginInstalled : function(event) {
    
    var browser = event.originalTarget;
    
    browser.missingPlugins = null;

    var notificationBox = gBrowser.getNotificationBox(browser);
    var notification = notificationBox.getNotificationWithValue("missing-plugins");
    if (notification)
      notificationBox.removeNotification(notification);

    
    browser.reload();
  },

  
  installSinglePlugin: function (plugin) {
    var missingPluginsArray = {};

    var pluginInfo = getPluginInfo(plugin);
    missingPluginsArray[pluginInfo.mimetype] = pluginInfo;

    openDialog("chrome://mozapps/content/plugins/pluginInstallerWizard.xul",
               "PFSWindow", "chrome,centerscreen,resizable=yes",
               {plugins: missingPluginsArray, browser: gBrowser.selectedBrowser});
  },

  
  managePlugins: function (aEvent) {
    BrowserOpenAddonsMgr("addons://list/plugin");
  },

  
  
  openPluginUpdatePage: function (aEvent) {
    openURL(Services.urlFormatter.formatURLPref("plugins.update.url"));
  },

#ifdef MOZ_CRASHREPORTER
  
  submitReport : function(pluginDumpID, browserDumpID) {
    
    
    this.CrashSubmit.submit(pluginDumpID);
    if (browserDumpID)
      this.CrashSubmit.submit(browserDumpID);
  },
#endif

  
  reloadPage: function (browser) {
    browser.reload();
  },

  
  openHelpPage: function () {
    openHelpLink("plugin-crashed", false);
  },

  
  _handleClickToPlayEvent: function PH_handleClickToPlayEvent(aPlugin) {
    let doc = aPlugin.ownerDocument;
    let browser = gBrowser.getBrowserForDocument(doc.defaultView.top.document);
    let pluginsPermission = Services.perms.testPermission(browser.currentURI, "plugins");
    let overlay = doc.getAnonymousElementByAttribute(aPlugin, "class", "mainBox");

    if (browser._clickToPlayPluginsActivated) {
      let objLoadingContent = aPlugin.QueryInterface(Ci.nsIObjectLoadingContent);
      objLoadingContent.playPlugin();
      return;
    } else if (pluginsPermission == Ci.nsIPermissionManager.DENY_ACTION) {
      if (overlay)
        overlay.style.visibility = "hidden";
      return;
    }

    
    if (overlay) {
      overlay.addEventListener("click", function(aEvent) {
        
        if (!(aEvent.originalTarget instanceof HTMLAnchorElement) &&
            aEvent.button == 0 && aEvent.isTrusted)
          gPluginHandler.activateSinglePlugin(aEvent.target.ownerDocument.defaultView.top, aPlugin);
      }, true);
    }

    if (!browser._clickToPlayDoorhangerShown)
      gPluginHandler._showClickToPlayNotification(browser);
  },

  _handlePlayPreviewEvent: function PH_handlePlayPreviewEvent(aPlugin) {
    let doc = aPlugin.ownerDocument;
    let previewContent = doc.getAnonymousElementByAttribute(aPlugin, "class", "previewPluginContent");
    if (!previewContent) {
      
      gPluginHandler.stopPlayPreview(aPlugin, false);
      return;
    }
    let iframe = previewContent.getElementsByClassName("previewPluginContentFrame")[0];
    if (!iframe) {
      
      iframe = doc.createElementNS("http://www.w3.org/1999/xhtml", "iframe");
      iframe.className = "previewPluginContentFrame";
      previewContent.appendChild(iframe);

      
      aPlugin.clientTop;
    }
    let pluginInfo = getPluginInfo(aPlugin);
    let playPreviewUri = "data:application/x-moz-playpreview;," + pluginInfo.mimetype;
    iframe.src = playPreviewUri;

    
    
    previewContent.addEventListener("MozPlayPlugin", function playPluginHandler(aEvent) {
      if (!aEvent.isTrusted)
        return;

      previewContent.removeEventListener("MozPlayPlugin", playPluginHandler, true);

      let playPlugin = !aEvent.detail;
      gPluginHandler.stopPlayPreview(aPlugin, playPlugin);

      
      let iframe = previewContent.getElementsByClassName("previewPluginContentFrame")[0];
      if (iframe)
        previewContent.removeChild(iframe);
    }, true);
  },

  reshowClickToPlayNotification: function PH_reshowClickToPlayNotification() {
    if (!Services.prefs.getBoolPref("plugins.click_to_play"))
      return;

    let browser = gBrowser.selectedBrowser;

    let pluginsPermission = Services.perms.testPermission(browser.currentURI, "plugins");
    if (pluginsPermission == Ci.nsIPermissionManager.DENY_ACTION)
      return;

    let contentWindow = browser.contentWindow;
    let cwu = contentWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                           .getInterface(Ci.nsIDOMWindowUtils);
    let pluginNeedsActivation = cwu.plugins.some(function(plugin) {
      let objLoadingContent = plugin.QueryInterface(Ci.nsIObjectLoadingContent);
      return gPluginHandler.canActivatePlugin(objLoadingContent);
    });
    if (pluginNeedsActivation)
      gPluginHandler._showClickToPlayNotification(browser);
  },

  _showClickToPlayNotification: function PH_showClickToPlayNotification(aBrowser) {
    aBrowser._clickToPlayDoorhangerShown = true;
    let contentWindow = aBrowser.contentWindow;

    let messageString = gNavigatorBundle.getString("activatePluginsMessage.message");
    let mainAction = {
      label: gNavigatorBundle.getString("activatePluginsMessage.label"),
      accessKey: gNavigatorBundle.getString("activatePluginsMessage.accesskey"),
      callback: function() { gPluginHandler.activatePlugins(contentWindow); }
    };
    let secondaryActions = [{
      label: gNavigatorBundle.getString("activatePluginsMessage.always"),
      accessKey: gNavigatorBundle.getString("activatePluginsMessage.always.accesskey"),
      callback: function () {
        Services.perms.add(aBrowser.currentURI, "plugins", Ci.nsIPermissionManager.ALLOW_ACTION);
        gPluginHandler.activatePlugins(contentWindow);
      }
    },{
      label: gNavigatorBundle.getString("activatePluginsMessage.never"),
      accessKey: gNavigatorBundle.getString("activatePluginsMessage.never.accesskey"),
      callback: function () {
        Services.perms.add(aBrowser.currentURI, "plugins", Ci.nsIPermissionManager.DENY_ACTION);
        let notification = PopupNotifications.getNotification("click-to-play-plugins", aBrowser);
        if (notification)
          notification.remove();
        gPluginHandler._removeClickToPlayOverlays(contentWindow);
      }
    }];
    let options = { dismissed: true };
    PopupNotifications.show(aBrowser, "click-to-play-plugins",
                            messageString, "plugins-notification-icon",
                            mainAction, secondaryActions, options);
  },

  _removeClickToPlayOverlays: function PH_removeClickToPlayOverlays(aContentWindow) {
    let doc = aContentWindow.document;
    let cwu = aContentWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                            .getInterface(Ci.nsIDOMWindowUtils);
    for (let plugin of cwu.plugins) {
      let overlay = doc.getAnonymousElementByAttribute(plugin, "class", "mainBox");
      overlay.style.visibility = "hidden";
    }
  },

  
  pluginUnavailable: function (plugin, eventType) {
    let browser = gBrowser.getBrowserForDocument(plugin.ownerDocument
                                                       .defaultView.top.document);
    if (!browser.missingPlugins)
      browser.missingPlugins = {};

    var pluginInfo = getPluginInfo(plugin);
    browser.missingPlugins[pluginInfo.mimetype] = pluginInfo;

    var notificationBox = gBrowser.getNotificationBox(browser);

    
    
    let outdatedNotification = notificationBox.getNotificationWithValue("outdated-plugins");
    let blockedNotification  = notificationBox.getNotificationWithValue("blocked-plugins");
    let missingNotification  = notificationBox.getNotificationWithValue("missing-plugins");


    function showBlocklistInfo() {
      var url = formatURL("extensions.blocklist.detailsURL", true);
      gBrowser.loadOneTab(url, {inBackground: false});
      return true;
    }

    function showOutdatedPluginsInfo() {
      gPrefService.setBoolPref("plugins.update.notifyUser", false);
      var url = formatURL("plugins.update.url", true);
      gBrowser.loadOneTab(url, {inBackground: false});
      return true;
    }

    function showPluginsMissing() {
      
      var missingPluginsArray = gBrowser.selectedBrowser.missingPlugins;
      if (missingPluginsArray) {
        openDialog("chrome://mozapps/content/plugins/pluginInstallerWizard.xul",
                   "PFSWindow", "chrome,centerscreen,resizable=yes",
                   {plugins: missingPluginsArray, browser: gBrowser.selectedBrowser});
      }
    }

#ifdef XP_MACOSX
    function carbonFailurePluginsRestartBrowser()
    {
      
      let cancelQuit = Cc["@mozilla.org/supports-PRBool;1"].
                         createInstance(Ci.nsISupportsPRBool);
      Services.obs.notifyObservers(cancelQuit, "quit-application-requested", null);

      
      if (cancelQuit.data)
        return;

      let as = Cc["@mozilla.org/toolkit/app-startup;1"].getService(Ci.nsIAppStartup);
      as.quit(Ci.nsIAppStartup.eRestarti386 | Ci.nsIAppStartup.eRestart | Ci.nsIAppStartup.eAttemptQuit);
    }
#endif

    let notifications = {
      PluginBlocklisted : {
                            barID   : "blocked-plugins",
                            iconURL : "chrome://mozapps/skin/plugins/notifyPluginBlocked.png",
                            message : gNavigatorBundle.getString("blockedpluginsMessage.title"),
                            buttons : [{
                                         label     : gNavigatorBundle.getString("blockedpluginsMessage.infoButton.label"),
                                         accessKey : gNavigatorBundle.getString("blockedpluginsMessage.infoButton.accesskey"),
                                         popup     : null,
                                         callback  : showBlocklistInfo
                                       },
                                       {
                                         label     : gNavigatorBundle.getString("blockedpluginsMessage.searchButton.label"),
                                         accessKey : gNavigatorBundle.getString("blockedpluginsMessage.searchButton.accesskey"),
                                         popup     : null,
                                         callback  : showOutdatedPluginsInfo
                                      }],
                          },
      PluginOutdated    : {
                            barID   : "outdated-plugins",
                            iconURL : "chrome://mozapps/skin/plugins/notifyPluginOutdated.png",
                            message : gNavigatorBundle.getString("outdatedpluginsMessage.title"),
                            buttons : [{
                                         label     : gNavigatorBundle.getString("outdatedpluginsMessage.updateButton.label"),
                                         accessKey : gNavigatorBundle.getString("outdatedpluginsMessage.updateButton.accesskey"),
                                         popup     : null,
                                         callback  : showOutdatedPluginsInfo
                                      }],
                          },
      PluginNotFound    : {
                            barID   : "missing-plugins",
                            iconURL : "chrome://mozapps/skin/plugins/notifyPluginGeneric.png",
                            message : gNavigatorBundle.getString("missingpluginsMessage.title"),
                            buttons : [{
                                         label     : gNavigatorBundle.getString("missingpluginsMessage.button.label"),
                                         accessKey : gNavigatorBundle.getString("missingpluginsMessage.button.accesskey"),
                                         popup     : null,
                                         callback  : showPluginsMissing
                                      }],
                            },
#ifdef XP_MACOSX
      "npapi-carbon-event-model-failure" : {
                                             barID    : "carbon-failure-plugins",
                                             iconURL  : "chrome://mozapps/skin/plugins/notifyPluginGeneric.png",
                                             message  : gNavigatorBundle.getString("carbonFailurePluginsMessage.message"),
                                             buttons: [{
                                                         label     : gNavigatorBundle.getString("carbonFailurePluginsMessage.restartButton.label"),
                                                         accessKey : gNavigatorBundle.getString("carbonFailurePluginsMessage.restartButton.accesskey"),
                                                         popup     : null,
                                                         callback  : carbonFailurePluginsRestartBrowser
                                                      }],
                            }
#endif
    };

    
    if (outdatedNotification)
      return;

#ifdef XP_MACOSX
    if (eventType == "npapi-carbon-event-model-failure") {
      if (gPrefService.getBoolPref("plugins.hide_infobar_for_carbon_failure_plugin"))
        return;

      let carbonFailureNotification =
        notificationBox.getNotificationWithValue("carbon-failure-plugins");

      if (carbonFailureNotification)
         carbonFailureNotification.close();

      let macutils = Cc["@mozilla.org/xpcom/mac-utils;1"].getService(Ci.nsIMacUtils);
      
      if (!macutils.isUniversalBinary)
        eventType = "PluginNotFound";
    }
#endif

    if (eventType == "PluginBlocklisted") {
      if (gPrefService.getBoolPref("plugins.hide_infobar_for_missing_plugin")) 
        return;

      if (blockedNotification || missingNotification)
        return;
    }
    else if (eventType == "PluginOutdated") {
      if (gPrefService.getBoolPref("plugins.hide_infobar_for_outdated_plugin"))
        return;

      
      if (blockedNotification)
        blockedNotification.close();
      if (missingNotification)
        missingNotification.close();
    }
    else if (eventType == "PluginNotFound") {
      if (gPrefService.getBoolPref("plugins.hide_infobar_for_missing_plugin"))
        return;

      if (missingNotification)
        return;

      
      if (blockedNotification)
        blockedNotification.close();
    }

    let notify = notifications[eventType];
    notificationBox.appendNotification(notify.message, notify.barID, notify.iconURL,
                                       notificationBox.PRIORITY_WARNING_MEDIUM,
                                       notify.buttons);
  },

  
  
  pluginCrashed : function(subject, topic, data) {
    let propertyBag = subject;
    if (!(propertyBag instanceof Ci.nsIPropertyBag2) ||
        !(propertyBag instanceof Ci.nsIWritablePropertyBag2))
     return;

#ifdef MOZ_CRASHREPORTER
    let pluginDumpID = propertyBag.getPropertyAsAString("pluginDumpID");
    let browserDumpID= propertyBag.getPropertyAsAString("browserDumpID");
    let shouldSubmit = gCrashReporter.submitReports;
    let doPrompt     = true; 

    
    if (pluginDumpID && shouldSubmit && !doPrompt) {
      this.submitReport(pluginDumpID, browserDumpID);
      
      propertyBag.setPropertyAsBool("submittedCrashReport", true);
    }
#endif
  },

  
  
  pluginInstanceCrashed: function (plugin, aEvent) {
    
    if (!(aEvent instanceof Ci.nsIDOMDataContainerEvent))
      return;

    let submittedReport = aEvent.getData("submittedCrashReport");
    let doPrompt        = true; 
    let submitReports   = true; 
    let pluginName      = aEvent.getData("pluginName");
    let pluginFilename  = aEvent.getData("pluginFilename");
    let pluginDumpID    = aEvent.getData("pluginDumpID");
    let browserDumpID   = aEvent.getData("browserDumpID");

    
    pluginName = this.makeNicePluginName(pluginName, pluginFilename);

    let messageString = gNavigatorBundle.getFormattedString("crashedpluginsMessage.title", [pluginName]);

    
    
    
    let doc = plugin.ownerDocument;
    let overlay = doc.getAnonymousElementByAttribute(plugin, "class", "mainBox");
    let statusDiv = doc.getAnonymousElementByAttribute(plugin, "class", "submitStatus");
#ifdef MOZ_CRASHREPORTER
    let status;

    
    if (submittedReport) { 
      status = "submitted";
    }
    else if (!submitReports && !doPrompt) {
      status = "noSubmit";
    }
    else { 
      status = "please";
      
      let pleaseLink = doc.getAnonymousElementByAttribute(
                            plugin, "class", "pleaseSubmitLink");
      this.addLinkClickCallback(pleaseLink, "submitReport",
                                pluginDumpID, browserDumpID);
    }

    
    
    if (!pluginDumpID) {
        status = "noReport";
    }

    statusDiv.setAttribute("status", status);

    let bottomLinks = doc.getAnonymousElementByAttribute(plugin, "class", "msg msgBottomLinks");
    bottomLinks.style.display = "block";
    let helpIcon = doc.getAnonymousElementByAttribute(plugin, "class", "helpIcon");
    this.addLinkClickCallback(helpIcon, "openHelpPage");

    
    
    
    if (doPrompt) {
      let observer = {
        QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                               Ci.nsISupportsWeakReference]),
        observe : function(subject, topic, data) {
          let propertyBag = subject;
          if (!(propertyBag instanceof Ci.nsIPropertyBag2))
            return;
          
          if (propertyBag.get("minidumpID") != pluginDumpID)
            return;
          statusDiv.setAttribute("status", data);
        },

        handleEvent : function(event) {
            
        }
      }

      
      Services.obs.addObserver(observer, "crash-report-status", true);
      
      
      
      
      
      doc.addEventListener("mozCleverClosureHack", observer, false);
    }
#endif

    let crashText = doc.getAnonymousElementByAttribute(plugin, "class", "msg msgCrashed");
    crashText.textContent = messageString;

    let browser = gBrowser.getBrowserForDocument(doc.defaultView.top.document);

    let link = doc.getAnonymousElementByAttribute(plugin, "class", "reloadLink");
    this.addLinkClickCallback(link, "reloadPage", browser);

    let notificationBox = gBrowser.getNotificationBox(browser);

    
    if (this.isTooSmall(plugin, overlay)) {
        
        
        overlay.style.visibility = "hidden";
        
        
        if (!doc.mozNoPluginCrashedNotification)
          showNotificationBar(pluginDumpID, browserDumpID);
    } else {
        
        
        
        hideNotificationBar();
        doc.mozNoPluginCrashedNotification = true;
    }

    function hideNotificationBar() {
      let notification = notificationBox.getNotificationWithValue("plugin-crashed");
      if (notification)
        notificationBox.removeNotification(notification, true);
    }

    function showNotificationBar(pluginDumpID, browserDumpID) {
      
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
#ifdef MOZ_CRASHREPORTER
      let submitButton = {
        label: submitLabel,
        accessKey: submitKey,
        popup: null,
          callback: function() { gPluginHandler.submitReport(pluginDumpID, browserDumpID); },
      };
      if (pluginDumpID)
        buttons.push(submitButton);
#endif

      let notification = notificationBox.appendNotification(messageString, "plugin-crashed",
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

      
      doc.defaultView.top.addEventListener("unload", function() {
        notificationBox.removeNotification(notification);
      }, false);
    }

  }
};
