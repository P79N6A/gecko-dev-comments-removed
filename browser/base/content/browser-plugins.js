# -*- Mode: javascript; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:

var gPluginHandler = {
  PREF_NOTIFY_MISSING_FLASH: "plugins.notifyMissingFlash",
  PREF_SESSION_PERSIST_MINUTES: "plugin.sessionPermissionNow.intervalInMinutes",
  PREF_PERSISTENT_DAYS: "plugin.persistentPermissionAlways.intervalInDays",

  getPluginUI: function (plugin, anonid) {
    return plugin.ownerDocument.
           getAnonymousElementByAttribute(plugin, "anonid", anonid);
  },

#ifdef MOZ_CRASHREPORTER
  get CrashSubmit() {
    delete this.CrashSubmit;
    Cu.import("resource://gre/modules/CrashSubmit.jsm", this);
    return this.CrashSubmit;
  },
#endif

  _getPluginInfo: function (pluginElement) {
    let pluginHost = Cc["@mozilla.org/plugin/host;1"].getService(Ci.nsIPluginHost);
    pluginElement.QueryInterface(Ci.nsIObjectLoadingContent);

    let tagMimetype;
    let pluginName = gNavigatorBundle.getString("pluginInfo.unknownPlugin");
    let pluginTag = null;
    let permissionString = null;
    let fallbackType = null;
    let blocklistState = null;

    if (pluginElement instanceof HTMLAppletElement) {
      tagMimetype = "application/x-java-vm";
    } else {
      tagMimetype = pluginElement.actualType;

      if (tagMimetype == "") {
        tagMimetype = pluginElement.type;
      }
    }

    if (gPluginHandler.isKnownPlugin(pluginElement)) {
      pluginTag = pluginHost.getPluginTagForType(pluginElement.actualType);
      pluginName = gPluginHandler.makeNicePluginName(pluginTag.name);

      permissionString = pluginHost.getPermissionStringForType(pluginElement.actualType);
      fallbackType = pluginElement.defaultFallbackType;
      blocklistState = pluginHost.getBlocklistStateForType(pluginElement.actualType);
      
      
      
      if (blocklistState == Ci.nsIBlocklistService.STATE_SOFTBLOCKED ||
          blocklistState == Ci.nsIBlocklistService.STATE_OUTDATED) {
        blocklistState = Ci.nsIBlocklistService.STATE_NOT_BLOCKED;
      }
    }

    return { mimetype: tagMimetype,
             pluginName: pluginName,
             pluginTag: pluginTag,
             permissionString: permissionString,
             fallbackType: fallbackType,
             blocklistState: blocklistState,
           };
  },

  
  makeNicePluginName : function (aName) {
    if (aName == "Shockwave Flash")
      return "Adobe Flash";

    
    
    
    
    
    let newName = aName.replace(/[\s\d\.\-\_\(\)]+$/, "").replace(/\bplug-?in\b/i, "").trim();
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

  
  _getBindingType : function(plugin) {
    if (!(plugin instanceof Ci.nsIObjectLoadingContent))
      return null;

    switch (plugin.pluginFallbackType) {
      case Ci.nsIObjectLoadingContent.PLUGIN_UNSUPPORTED:
        return "PluginNotFound";
      case Ci.nsIObjectLoadingContent.PLUGIN_DISABLED:
        return "PluginDisabled";
      case Ci.nsIObjectLoadingContent.PLUGIN_BLOCKLISTED:
        return "PluginBlocklisted";
      case Ci.nsIObjectLoadingContent.PLUGIN_OUTDATED:
        return "PluginOutdated";
      case Ci.nsIObjectLoadingContent.PLUGIN_CLICK_TO_PLAY:
        return "PluginClickToPlay";
      case Ci.nsIObjectLoadingContent.PLUGIN_VULNERABLE_UPDATABLE:
        return "PluginVulnerableUpdatable";
      case Ci.nsIObjectLoadingContent.PLUGIN_VULNERABLE_NO_UPDATE:
        return "PluginVulnerableNoUpdate";
      case Ci.nsIObjectLoadingContent.PLUGIN_PLAY_PREVIEW:
        return "PluginPlayPreview";
      default:
        
        return null;
    }
  },

  supportedPlugins: {
    "mimetypes": {
      "application/x-shockwave-flash": "flash",
      "application/futuresplash": "flash",
      "application/x-java-.*": "java",
      "application/x-director": "shockwave",
      "application/(sdp|x-(mpeg|rtsp|sdp))": "quicktime",
      "audio/(3gpp(2)?|AMR|aiff|basic|mid(i)?|mp4|mpeg|vnd\.qcelp|wav|x-(aiff|m4(a|b|p)|midi|mpeg|wav))": "quicktime",
      "image/(pict|png|tiff|x-(macpaint|pict|png|quicktime|sgi|targa|tiff))": "quicktime",
      "video/(3gpp(2)?|flc|mp4|mpeg|quicktime|sd-video|x-mpeg)": "quicktime",
      "application/x-unknown": "test",
    },

    "plugins": {
      "flash": {
        "displayName": "Flash",
        "installWINNT": true,
        "installDarwin": true,
        "installLinux": true,
      },
      "java": {
        "displayName": "Java",
        "installWINNT": true,
        "installDarwin": true,
        "installLinux": true,
      },
      "shockwave": {
        "displayName": "Shockwave",
        "installWINNT": true,
        "installDarwin": true,
      },
      "quicktime": {
        "displayName": "QuickTime",
        "installWINNT": true,
      },
      "test": {
        "displayName": "Test plugin",
        "installWINNT": true,
        "installLinux": true,
        "installDarwin": true,
      }
    }
  },

  nameForSupportedPlugin: function (aMimeType) {
    for (let type in this.supportedPlugins.mimetypes) {
      let re = new RegExp(type);
      if (re.test(aMimeType)) {
        return this.supportedPlugins.mimetypes[type];
      }
    }
    return null;
  },

  canInstallThisMimeType: function (aMimeType) {
    let os = Services.appinfo.OS;
    let pluginName = this.nameForSupportedPlugin(aMimeType);
    if (pluginName && "install" + os in this.supportedPlugins.plugins[pluginName]) {
      return true;
    }
    return false;
  },

  handleEvent : function(event) {
    let plugin;
    let doc;

    let eventType = event.type;
    if (eventType === "PluginRemoved") {
      doc = event.target;
    }
    else {
      plugin = event.target;
      doc = plugin.ownerDocument;

      if (!(plugin instanceof Ci.nsIObjectLoadingContent))
        return;
    }

    if (eventType == "PluginBindingAttached") {
      
      
      
      let overlay = this.getPluginUI(plugin, "main");
      if (!overlay || overlay._bindingHandled) {
        return;
      }
      overlay._bindingHandled = true;

      
      eventType = this._getBindingType(plugin);
      if (!eventType) {
        
        return;
      }
    }

    let shouldShowNotification = false;
    let browser = gBrowser.getBrowserForDocument(doc.defaultView.top.document);

    switch (eventType) {
      case "PluginCrashed":
        this.pluginInstanceCrashed(plugin, event);
        break;

      case "PluginNotFound":
        let installable = this.showInstallNotification(plugin, eventType);
        
        
        
        if (installable && !(plugin instanceof HTMLObjectElement)) {
          let installStatus = this.getPluginUI(plugin, "installStatus");
          installStatus.setAttribute("installable", "true");
          let iconStatus = this.getPluginUI(plugin, "icon");
          iconStatus.setAttribute("installable", "true");

          let installLink = this.getPluginUI(plugin, "installPluginLink");
          this.addLinkClickCallback(installLink, "installSinglePlugin", plugin);
        }
        break;

      case "PluginBlocklisted":
      case "PluginOutdated":
        shouldShowNotification = true;
        break;

      case "PluginVulnerableUpdatable":
        let updateLink = this.getPluginUI(plugin, "checkForUpdatesLink");
        this.addLinkClickCallback(updateLink, "openPluginUpdatePage");
        

      case "PluginVulnerableNoUpdate":
      case "PluginClickToPlay":
        this._handleClickToPlayEvent(plugin);
        let overlay = this.getPluginUI(plugin, "main");
        let pluginName = this._getPluginInfo(plugin).pluginName;
        let messageString = gNavigatorBundle.getFormattedString("PluginClickToActivate", [pluginName]);
        let overlayText = this.getPluginUI(plugin, "clickToPlay");
        overlayText.textContent = messageString;
        if (eventType == "PluginVulnerableUpdatable" ||
            eventType == "PluginVulnerableNoUpdate") {
          let vulnerabilityString = gNavigatorBundle.getString(eventType);
          let vulnerabilityText = this.getPluginUI(plugin, "vulnerabilityStatus");
          vulnerabilityText.textContent = vulnerabilityString;
        }
        shouldShowNotification = true;
        break;

      case "PluginPlayPreview":
        this._handlePlayPreviewEvent(plugin);
        break;

      case "PluginDisabled":
        let manageLink = this.getPluginUI(plugin, "managePluginsLink");
        this.addLinkClickCallback(manageLink, "managePlugins");
        shouldShowNotification = true;
        break;

      case "PluginInstantiated":
      case "PluginRemoved":
        shouldShowNotification = true;
        break;
    }

    
    if (eventType != "PluginCrashed" && eventType != "PluginRemoved") {
      let overlay = this.getPluginUI(plugin, "main");
      if (overlay != null) {
        if (!this.isTooSmall(plugin, overlay))
          overlay.style.visibility = "visible";

        plugin.addEventListener("overflow", function(event) {
          overlay.style.visibility = "hidden";
        });
        plugin.addEventListener("underflow", function(event) {
          
          
          if (!gPluginHandler.isTooSmall(plugin, overlay)) {
            overlay.style.visibility = "visible";
          }
        });
      }
    }

    
    
    if (shouldShowNotification) {
      this._showClickToPlayNotification(browser);
    }
  },

  isKnownPlugin: function PH_isKnownPlugin(objLoadingContent) {
    return (objLoadingContent.getContentTypeForMIMEType(objLoadingContent.actualType) ==
            Ci.nsIObjectLoadingContent.TYPE_PLUGIN);
  },

  canActivatePlugin: function PH_canActivatePlugin(objLoadingContent) {
    
    
    
    if (!gPluginHandler.isKnownPlugin(objLoadingContent))
      return false;

    let pluginHost = Cc["@mozilla.org/plugin/host;1"].getService(Ci.nsIPluginHost);
    let permissionString = pluginHost.getPermissionStringForType(objLoadingContent.actualType);
    let principal = objLoadingContent.ownerDocument.defaultView.top.document.nodePrincipal;
    let pluginPermission = Services.perms.testPermissionFromPrincipal(principal, permissionString);

    let isFallbackTypeValid =
      objLoadingContent.pluginFallbackType >= Ci.nsIObjectLoadingContent.PLUGIN_CLICK_TO_PLAY &&
      objLoadingContent.pluginFallbackType <= Ci.nsIObjectLoadingContent.PLUGIN_VULNERABLE_NO_UPDATE;

    if (objLoadingContent.pluginFallbackType == Ci.nsIObjectLoadingContent.PLUGIN_PLAY_PREVIEW) {
      
      let playPreviewInfo = pluginHost.getPlayPreviewInfo(objLoadingContent.actualType);
      isFallbackTypeValid = !playPreviewInfo.ignoreCTP;
    }

    return !objLoadingContent.activated &&
           pluginPermission != Ci.nsIPermissionManager.DENY_ACTION &&
           isFallbackTypeValid;
  },

  hideClickToPlayOverlay: function(aPlugin) {
    let overlay = this.getPluginUI(aPlugin, "main");
    if (overlay)
      overlay.style.visibility = "hidden";
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
    var missingPlugins = new Map();

    var pluginInfo = this._getPluginInfo(plugin);
    missingPlugins.set(pluginInfo.mimetype, pluginInfo);

    openDialog("chrome://mozapps/content/plugins/pluginInstallerWizard.xul",
               "PFSWindow", "chrome,centerscreen,resizable=yes",
               {plugins: missingPlugins, browser: gBrowser.selectedBrowser});
  },

  
  managePlugins: function (aEvent) {
    BrowserOpenAddonsMgr("addons://list/plugin");
  },

  
  
  openPluginUpdatePage: function (aEvent) {
    openUILinkIn(Services.urlFormatter.formatURLPref("plugins.update.url"), "tab");
  },

#ifdef MOZ_CRASHREPORTER
  submitReport: function submitReport(pluginDumpID, browserDumpID, plugin) {
    let keyVals = {};
    if (plugin) {
      let userComment = this.getPluginUI(plugin, "submitComment").value.trim();
      if (userComment)
        keyVals.PluginUserComment = userComment;
      if (this.getPluginUI(plugin, "submitURLOptIn").checked)
        keyVals.PluginContentURL = plugin.ownerDocument.URL;
    }
    this.CrashSubmit.submit(pluginDumpID, { extraExtraKeyVals: keyVals });
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

  showInstallNotification: function (aPlugin) {
    let browser = gBrowser.getBrowserForDocument(aPlugin.ownerDocument
                                                        .defaultView.top.document);
    if (!browser.missingPlugins)
      browser.missingPlugins = new Map();

    let pluginInfo = this._getPluginInfo(aPlugin);
    browser.missingPlugins.set(pluginInfo.mimetype, pluginInfo);

    
    let mimetype = pluginInfo.mimetype.split(";")[0];
    if (!this.canInstallThisMimeType(mimetype))
      return false;

    let pluginIdentifier = this.nameForSupportedPlugin(mimetype);
    if (!pluginIdentifier)
      return false;

    let displayName = this.supportedPlugins.plugins[pluginIdentifier].displayName;

    
    let notification = PopupNotifications.getNotification("plugins-not-found", browser);
    if (notification)
      return true;

    let messageString = gNavigatorBundle.getString("installPlugin.message");
    let mainAction = {
      label: gNavigatorBundle.getFormattedString("installPlugin.button.label",
                                                 [displayName]),
      accessKey: gNavigatorBundle.getString("installPlugin.button.accesskey"),
      callback: function () {
        openDialog("chrome://mozapps/content/plugins/pluginInstallerWizard.xul",
                   "PFSWindow", "chrome,centerscreen,resizable=yes",
                   {plugins: browser.missingPlugins, browser: browser});
      }
    };
    let secondaryActions = null;
    let options = { dismissed: true };

    let showForFlash = Services.prefs.getBoolPref(this.PREF_NOTIFY_MISSING_FLASH);
    if (pluginIdentifier == "flash" && showForFlash) {
      let prefNotifyMissingFlash = this.PREF_NOTIFY_MISSING_FLASH;
      secondaryActions = [{
        label: gNavigatorBundle.getString("installPlugin.ignoreButton.label"),
        accessKey: gNavigatorBundle.getString("installPlugin.ignoreButton.accesskey"),
        callback: function () {
          Services.prefs.setBoolPref(prefNotifyMissingFlash, false);
        }
      }];
      options.dismissed = false;
    }
    PopupNotifications.show(browser, "plugins-not-found",
                            messageString, "plugin-install-notification-icon",
                            mainAction, secondaryActions, options);
    return true;
  },
  
  _handleClickToPlayEvent: function PH_handleClickToPlayEvent(aPlugin) {
    let doc = aPlugin.ownerDocument;
    let browser = gBrowser.getBrowserForDocument(doc.defaultView.top.document);
    let pluginHost = Cc["@mozilla.org/plugin/host;1"].getService(Ci.nsIPluginHost);
    let objLoadingContent = aPlugin.QueryInterface(Ci.nsIObjectLoadingContent);
    
    
    if (!gPluginHandler.isKnownPlugin(objLoadingContent))
      return;
    let permissionString = pluginHost.getPermissionStringForType(objLoadingContent.actualType);
    let principal = doc.defaultView.top.document.nodePrincipal;
    let pluginPermission = Services.perms.testPermissionFromPrincipal(principal, permissionString);

    let overlay = this.getPluginUI(aPlugin, "main");

    if (pluginPermission == Ci.nsIPermissionManager.DENY_ACTION) {
      if (overlay)
        overlay.style.visibility = "hidden";
      return;
    }

    if (overlay) {
      overlay.addEventListener("click", gPluginHandler._overlayClickListener, true);
      let closeIcon = this.getPluginUI(aPlugin, "closeIcon");
      closeIcon.addEventListener("click", function(aEvent) {
        if (aEvent.button == 0 && aEvent.isTrusted)
          gPluginHandler.hideClickToPlayOverlay(aPlugin);
      }, true);
    }
  },

  _overlayClickListener: {
    handleEvent: function PH_handleOverlayClick(aEvent) {
      let plugin = document.getBindingParent(aEvent.target);
      let contentWindow = plugin.ownerDocument.defaultView.top;
      
      
      
      let browser = gBrowser.getBrowserForDocument ?
                      gBrowser.getBrowserForDocument(contentWindow.document) :
                      null;
      
      
      if (!browser) {
        aEvent.target.removeEventListener("click", gPluginHandler._overlayClickListener, true);
        return;
      }
      let objLoadingContent = plugin.QueryInterface(Ci.nsIObjectLoadingContent);
      
      if (!(aEvent.originalTarget instanceof HTMLAnchorElement) &&
          (aEvent.originalTarget.getAttribute('anonid') != 'closeIcon') &&
          aEvent.button == 0 && aEvent.isTrusted) {
        gPluginHandler._showClickToPlayNotification(browser, plugin);
        aEvent.stopPropagation();
        aEvent.preventDefault();
      }
    }
  },

  _handlePlayPreviewEvent: function PH_handlePlayPreviewEvent(aPlugin) {
    let doc = aPlugin.ownerDocument;
    let browser = gBrowser.getBrowserForDocument(doc.defaultView.top.document);
    let pluginHost = Cc["@mozilla.org/plugin/host;1"].getService(Ci.nsIPluginHost);
    let pluginInfo = this._getPluginInfo(aPlugin);
    let playPreviewInfo = pluginHost.getPlayPreviewInfo(pluginInfo.mimetype);

    let previewContent = this.getPluginUI(aPlugin, "previewPluginContent");
    let iframe = previewContent.getElementsByClassName("previewPluginContentFrame")[0];
    if (!iframe) {
      
      iframe = doc.createElementNS("http://www.w3.org/1999/xhtml", "iframe");
      iframe.className = "previewPluginContentFrame";
      previewContent.appendChild(iframe);

      
      aPlugin.clientTop;
    }
    iframe.src = playPreviewInfo.redirectURL;

    
    
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

    if (!playPreviewInfo.ignoreCTP) {
      gPluginHandler._showClickToPlayNotification(browser);
    }
  },

  reshowClickToPlayNotification: function PH_reshowClickToPlayNotification() {
    let browser = gBrowser.selectedBrowser;
    let contentWindow = browser.contentWindow;
    let cwu = contentWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                           .getInterface(Ci.nsIDOMWindowUtils);
    let plugins = cwu.plugins;
    for (let plugin of plugins) {
      let overlay = this.getPluginUI(plugin, "main");
      if (overlay)
        overlay.removeEventListener("click", gPluginHandler._overlayClickListener, true);
      let objLoadingContent = plugin.QueryInterface(Ci.nsIObjectLoadingContent);
      if (gPluginHandler.canActivatePlugin(objLoadingContent))
        gPluginHandler._handleClickToPlayEvent(plugin);
    }
    gPluginHandler._showClickToPlayNotification(browser);
  },

  _clickToPlayNotificationEventCallback: function PH_ctpEventCallback(event) {
    if (event == "showing") {
      gPluginHandler._makeCenterActions(this);
      Services.telemetry.getHistogramById("PLUGINS_NOTIFICATION_SHOWN")
        .add(!this.options.primaryPlugin);
    }
    else if (event == "dismissed") {
      
      
      this.options.primaryPlugin = null;
    }
  },

  
  _getHostFromPrincipal: function PH_getHostFromPrincipal(principal) {
    if (!principal.URI || principal.URI.schemeIs("moz-nullprincipal")) {
      return "(null)";
    }

    try {
      if (principal.URI.host)
        return principal.URI.host;
    } catch (e) {}

    return principal.origin;
  },

  _makeCenterActions: function PH_makeCenterActions(notification) {
    let contentWindow = notification.browser.contentWindow;
    let cwu = contentWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                           .getInterface(Ci.nsIDOMWindowUtils);

    let principal = contentWindow.document.nodePrincipal;
    
    let principalHost = this._getHostFromPrincipal(principal);

    let centerActions = [];
    let pluginsFound = new Set();
    for (let plugin of cwu.plugins) {
      plugin.QueryInterface(Ci.nsIObjectLoadingContent);
      if (plugin.getContentTypeForMIMEType(plugin.actualType) != Ci.nsIObjectLoadingContent.TYPE_PLUGIN) {
        continue;
      }

      let pluginInfo = this._getPluginInfo(plugin);
      if (pluginInfo.permissionString === null) {
        Components.utils.reportError("No permission string for active plugin.");
        continue;
      }
      if (pluginsFound.has(pluginInfo.permissionString)) {
        continue;
      }
      pluginsFound.add(pluginInfo.permissionString);

      
      
      
      let permissionObj = Services.perms.
        getPermissionObject(principal, pluginInfo.permissionString, false);
      if (permissionObj) {
        pluginInfo.pluginPermissionHost = permissionObj.host;
        pluginInfo.pluginPermissionType = permissionObj.expireType;
      }
      else {
        pluginInfo.pluginPermissionHost = principalHost;
        pluginInfo.pluginPermissionType = undefined;
      }

      let url;
      
      if (pluginInfo.blocklistState == Ci.nsIBlocklistService.STATE_VULNERABLE_UPDATE_AVAILABLE) {
        url = Services.urlFormatter.formatURLPref("plugins.update.url");
      }
      else if (pluginInfo.blocklistState != Ci.nsIBlocklistService.STATE_NOT_BLOCKED) {
        url = Services.blocklist.getPluginBlocklistURL(pluginInfo.pluginTag);
      }
      pluginInfo.detailsLink = url;

      centerActions.push(pluginInfo);
    }
    centerActions.sort(function(a, b) {
      return a.pluginName.localeCompare(b.pluginName);
    });

    notification.options.centerActions = centerActions;

    
    let histogramCount = centerActions.length - 1;
    if (histogramCount > 4) {
      histogramCount = 4;
    }
    Services.telemetry.getHistogramById("PLUGINS_NOTIFICATION_PLUGIN_COUNT")
      .add(histogramCount);
  },

  




  _updatePluginPermission: function PH_setPermissionForPlugins(aNotification, aPluginInfo, aNewState) {
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
        break;

      case "allowalways":
        permission = Ci.nsIPermissionManager.ALLOW_ACTION;
        expireType = Ci.nsIPermissionManager.EXPIRE_TIME;
        expireTime = Date.now() +
          Services.prefs.getIntPref(this.PREF_PERSISTENT_DAYS) * 24 * 60 * 60 * 1000;
        histogram.add(1);
        break;

      case "block":
        permission = Ci.nsIPermissionManager.PROMPT_ACTION;
        expireType = Ci.nsIPermissionManager.EXPIRE_NEVER;
        expireTime = 0;
        histogram.add(2);
        break;

      
      
      case "continue":
        break;
      default:
        Cu.reportError(Error("Unexpected plugin state: " + aNewState));
        return;
    }

    let browser = aNotification.browser;
    let contentWindow = browser.contentWindow;
    if (aNewState != "continue") {
      let principal = contentWindow.document.nodePrincipal;
      Services.perms.addFromPrincipal(principal, aPluginInfo.permissionString,
                                      permission, expireType, expireTime);

      if (aNewState == "block") {
        return;
      }
    }

    
    
    let cwu = contentWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                           .getInterface(Ci.nsIDOMWindowUtils);
    let plugins = cwu.plugins;
    let pluginHost = Cc["@mozilla.org/plugin/host;1"].getService(Ci.nsIPluginHost);

    for (let plugin of plugins) {
      plugin.QueryInterface(Ci.nsIObjectLoadingContent);
      
      
      if (gPluginHandler.canActivatePlugin(plugin) &&
          aPluginInfo.permissionString == pluginHost.getPermissionStringForType(plugin.actualType)) {
        plugin.playPlugin();
      }
    }
  },

  _showClickToPlayNotification: function PH_showClickToPlayNotification(aBrowser, aPrimaryPlugin) {
    let notification = PopupNotifications.getNotification("click-to-play-plugins", aBrowser);

    let contentWindow = aBrowser.contentWindow;
    let contentDoc = aBrowser.contentDocument;
    let cwu = contentWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                           .getInterface(Ci.nsIDOMWindowUtils);
    
    let plugins = cwu.plugins.filter((plugin) =>
      plugin.getContentTypeForMIMEType(plugin.actualType) == Ci.nsIObjectLoadingContent.TYPE_PLUGIN);
    if (plugins.length == 0) {
      if (notification) {
        PopupNotifications.remove(notification);
      }
      return;
    }

    let icon = 'plugins-notification-icon';
    for (let plugin of plugins) {
      let fallbackType = plugin.pluginFallbackType;
      if (fallbackType == plugin.PLUGIN_VULNERABLE_UPDATABLE ||
          fallbackType == plugin.PLUGIN_VULNERABLE_NO_UPDATE ||
          fallbackType == plugin.PLUGIN_BLOCKLISTED) {
        icon = 'blocked-plugins-notification-icon';
        break;
      }
      if (fallbackType == plugin.PLUGIN_CLICK_TO_PLAY) {
        let overlay = this.getPluginUI(plugin, "main");
        if (!overlay || overlay.style.visibility == 'hidden') {
          icon = 'alert-plugins-notification-icon';
        }
      }
    }

    let dismissed = notification ? notification.dismissed : true;
    if (aPrimaryPlugin)
      dismissed = false;

    let primaryPluginPermission = null;
    if (aPrimaryPlugin) {
      primaryPluginPermission = this._getPluginInfo(aPrimaryPlugin).permissionString;
    }

    let options = {
      dismissed: dismissed,
      eventCallback: this._clickToPlayNotificationEventCallback,
      primaryPlugin: primaryPluginPermission
    };
    PopupNotifications.show(aBrowser, "click-to-play-plugins",
                            "", icon,
                            null, null, options);
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
    let pluginDumpID    = aEvent.getData("pluginDumpID");
    let browserDumpID   = aEvent.getData("browserDumpID");

    
    pluginName = this.makeNicePluginName(pluginName);

    let messageString = gNavigatorBundle.getFormattedString("crashedpluginsMessage.title", [pluginName]);

    
    
    

    
    plugin.clientTop;
    let overlay = this.getPluginUI(plugin, "main");
    let statusDiv = this.getPluginUI(plugin, "submitStatus");
    let doc = plugin.ownerDocument;
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
      this.getPluginUI(plugin, "submitButton").addEventListener("click",
        function (event) {
          if (event.button != 0 || !event.isTrusted)
            return;
          this.submitReport(pluginDumpID, browserDumpID, plugin);
          pref.setBoolPref("", optInCB.checked);
        }.bind(this));
      let optInCB = this.getPluginUI(plugin, "submitURLOptIn");
      let pref = Services.prefs.getBranch("dom.ipc.plugins.reportCrashURL");
      optInCB.checked = pref.getBoolPref("");
    }

    
    
    if (!pluginDumpID) {
        status = "noReport";
    }

    statusDiv.setAttribute("status", status);

    let helpIcon = this.getPluginUI(plugin, "helpIcon");
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

    let crashText = this.getPluginUI(plugin, "crashedText");
    crashText.textContent = messageString;

    let browser = gBrowser.getBrowserForDocument(doc.defaultView.top.document);

    let link = this.getPluginUI(plugin, "reloadLink");
    this.addLinkClickCallback(link, "reloadPage", browser);

    let notificationBox = gBrowser.getNotificationBox(browser);

    let isShowing = true;

    
    if (this.isTooSmall(plugin, overlay)) {
      
      statusDiv.removeAttribute("status");

      if (this.isTooSmall(plugin, overlay)) {
        
        
        overlay.style.visibility = "hidden";
        isShowing = false;
      }
    }

    if (isShowing) {
      
      
      
      hideNotificationBar();
      doc.mozNoPluginCrashedNotification = true;
    } else {
      
      
      if (!doc.mozNoPluginCrashedNotification)
        showNotificationBar(pluginDumpID, browserDumpID);
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
