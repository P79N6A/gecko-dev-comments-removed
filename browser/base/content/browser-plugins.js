# -*- Mode: javascript; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:

var gPluginHandler = {
  PLUGIN_SCRIPTED_STATE_NONE: 0,
  PLUGIN_SCRIPTED_STATE_FIRED: 1,
  PLUGIN_SCRIPTED_STATE_DONE: 2,

#ifdef MOZ_CRASHREPORTER
  get CrashSubmit() {
    delete this.CrashSubmit;
    Cu.import("resource://gre/modules/CrashSubmit.jsm", this);
    return this.CrashSubmit;
  },
#endif

  _getPluginInfo: function (pluginElement) {
    let tagMimetype;
    let pluginsPage;
    let pluginName = gNavigatorBundle.getString("pluginInfo.unknownPlugin");
    if (pluginElement instanceof HTMLAppletElement) {
      tagMimetype = "application/x-java-vm";
    } else {
      if (pluginElement instanceof HTMLObjectElement) {
        pluginsPage = pluginElement.getAttribute("codebase");
      } else {
        pluginsPage = pluginElement.getAttribute("pluginspage");
      }

      
      if (pluginsPage) {
        let doc = pluginElement.ownerDocument;
        let docShell = findChildShell(doc, gBrowser.docShell, null);
        try {
          pluginsPage = makeURI(pluginsPage, doc.characterSet, docShell.currentURI).spec;
        } catch (ex) {
          pluginsPage = "";
        }
      }

      tagMimetype = pluginElement.QueryInterface(Ci.nsIObjectLoadingContent)
                                 .actualType;

      if (tagMimetype == "") {
        tagMimetype = pluginElement.type;
      }
    }

    if (tagMimetype) {
      let navMimeType = navigator.mimeTypes.namedItem(tagMimetype);
      if (navMimeType && navMimeType.enabledPlugin) {
        pluginName = navMimeType.enabledPlugin.name;
        pluginName = gPluginHandler.makeNicePluginName(pluginName);
      }
    }

    return { mimetype: tagMimetype,
             pluginsPage: pluginsPage,
             pluginName: pluginName };
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

  handleEvent : function(event) {
    let plugin = event.target;
    let doc = plugin.ownerDocument;

    
    if (!(plugin instanceof Ci.nsIObjectLoadingContent))
      return;

    let eventType = event.type;
    if (eventType == "PluginBindingAttached") {
      
      
      
      let overlay = doc.getAnonymousElementByAttribute(plugin, "class", "mainBox");
      if (!overlay || overlay._bindingHandled) {
        return;
      }
      overlay._bindingHandled = true;

      
      eventType = this._getBindingType(plugin);
      if (!eventType) {
        
        return;
      }
    }

    switch (eventType) {
      case "PluginCrashed":
        this.pluginInstanceCrashed(plugin, event);
        break;

      case "PluginNotFound":
        
        
        
        if (!(plugin instanceof HTMLObjectElement)) {
          
          let installStatus = doc.getAnonymousElementByAttribute(plugin, "class", "installStatus");
          installStatus.setAttribute("status", "ready");
          let iconStatus = doc.getAnonymousElementByAttribute(plugin, "class", "icon");
          iconStatus.setAttribute("status", "ready");

          let installLink = doc.getAnonymousElementByAttribute(plugin, "class", "installPluginLink");
          this.addLinkClickCallback(installLink, "installSinglePlugin", plugin);
        }
        

      case "PluginBlocklisted":
      case "PluginOutdated":
#ifdef XP_MACOSX
      case "npapi-carbon-event-model-failure":
#endif
        this.pluginUnavailable(plugin, eventType);
        break;

      case "PluginVulnerableUpdatable":
        let updateLink = doc.getAnonymousElementByAttribute(plugin, "class", "checkForUpdatesLink");
        this.addLinkClickCallback(updateLink, "openPluginUpdatePage");
        

      case "PluginVulnerableNoUpdate":
      case "PluginClickToPlay":
        this._handleClickToPlayEvent(plugin);
        let overlay = doc.getAnonymousElementByAttribute(plugin, "class", "mainBox");
        let pluginName = this._getPluginInfo(plugin).pluginName;
        let messageString = gNavigatorBundle.getFormattedString("PluginClickToActivate", [pluginName]);
        let overlayText = doc.getAnonymousElementByAttribute(plugin, "class", "msg msgClickToPlay");
        overlayText.textContent = messageString;
        if (eventType == "PluginVulnerableUpdatable" ||
            eventType == "PluginVulnerableNoUpdate") {
          let vulnerabilityString = gNavigatorBundle.getString(eventType);
          let vulnerabilityText = doc.getAnonymousElementByAttribute(plugin, "anonid", "vulnerabilityStatus");
          vulnerabilityText.textContent = vulnerabilityString;
        }
        break;

      case "PluginPlayPreview":
        this._handlePlayPreviewEvent(plugin);
        break;

      case "PluginDisabled":
        let manageLink = doc.getAnonymousElementByAttribute(plugin, "class", "managePluginsLink");
        this.addLinkClickCallback(manageLink, "managePlugins");
        break;

      case "PluginScripted":
        let browser = gBrowser.getBrowserForDocument(doc.defaultView.top.document);
        if (browser._pluginScriptedState == this.PLUGIN_SCRIPTED_STATE_NONE) {
          browser._pluginScriptedState = this.PLUGIN_SCRIPTED_STATE_FIRED;
          setTimeout(function() {
            gPluginHandler.handlePluginScripted(this);
          }.bind(browser), 500);
        }
        break;
    }

    
    if (eventType != "PluginCrashed") {
      let overlay = doc.getAnonymousElementByAttribute(plugin, "class", "mainBox");
      if (overlay != null && this.isTooSmall(plugin, overlay))
        overlay.style.visibility = "hidden";
    }
  },

  _notificationDisplayedOnce: false,
  handlePluginScripted: function PH_handlePluginScripted(aBrowser) {
    let contentWindow = aBrowser.contentWindow;
    if (!contentWindow)
      return;

    let cwu = contentWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                            .getInterface(Ci.nsIDOMWindowUtils);
    let plugins = cwu.plugins.filter(function(plugin) {
      let objLoadingContent = plugin.QueryInterface(Ci.nsIObjectLoadingContent);
      return gPluginHandler.canActivatePlugin(objLoadingContent);
    });

    let haveVisibleCTPPlugin = plugins.some(function(plugin) {
      let doc = plugin.ownerDocument;
      let overlay = doc.getAnonymousElementByAttribute(plugin, "class", "mainBox");
      if (!overlay)
        return false;

      
      
      
      
      let computedStyle = contentWindow.getComputedStyle(plugin);
      let isInvisible = ((computedStyle.width == "240px" &&
                          computedStyle.height == "200px") ||
                         gPluginHandler.isTooSmall(plugin, overlay));
      return !isInvisible;
    });

    let notification = PopupNotifications.getNotification("click-to-play-plugins", aBrowser);
    if (notification && plugins.length > 0 && !haveVisibleCTPPlugin && !this._notificationDisplayedOnce) {
      notification.reshow();
      this._notificationDisplayedOnce = true;
    }

    aBrowser._pluginScriptedState = this.PLUGIN_SCRIPTED_STATE_DONE;
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
    let browser = gBrowser.getBrowserForDocument(objLoadingContent.ownerDocument.defaultView.top.document);
    let pluginPermission = Services.perms.testPermission(browser.currentURI, permissionString);

    return !objLoadingContent.activated &&
           pluginPermission != Ci.nsIPermissionManager.DENY_ACTION &&
           objLoadingContent.pluginFallbackType >= Ci.nsIObjectLoadingContent.PLUGIN_CLICK_TO_PLAY &&
           objLoadingContent.pluginFallbackType <= Ci.nsIObjectLoadingContent.PLUGIN_VULNERABLE_NO_UPDATE;
  },

  activatePlugins: function PH_activatePlugins(aContentWindow) {
    let browser = gBrowser.getBrowserForDocument(aContentWindow.document);
    browser._clickToPlayAllPluginsActivated = true;
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
    let pluginNeedsActivation = gPluginHandler._pluginNeedsActivationExceptThese([aPlugin]);
    let browser = gBrowser.getBrowserForDocument(aContentWindow.document);
    let notification = PopupNotifications.getNotification("click-to-play-plugins", browser);
    if (notification) {
      notification.remove();
    }
    if (pluginNeedsActivation) {
      gPluginHandler._showClickToPlayNotification(browser);
    }
  },

  hideClickToPlayOverlay: function(aPlugin) {
    let overlay = aPlugin.ownerDocument.getAnonymousElementByAttribute(aPlugin, "class", "mainBox");
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
    let pluginHost = Cc["@mozilla.org/plugin/host;1"].getService(Ci.nsIPluginHost);
    let objLoadingContent = aPlugin.QueryInterface(Ci.nsIObjectLoadingContent);
    
    
    if (!gPluginHandler.isKnownPlugin(objLoadingContent))
      return;
    let permissionString = pluginHost.getPermissionStringForType(objLoadingContent.actualType);
    let pluginPermission = Services.perms.testPermission(browser.currentURI, permissionString);

    let overlay = doc.getAnonymousElementByAttribute(aPlugin, "class", "mainBox");

    if (pluginPermission == Ci.nsIPermissionManager.DENY_ACTION) {
      if (overlay)
        overlay.style.visibility = "hidden";
      return;
    }

    let pluginInfo = this._getPluginInfo(aPlugin);
    if (browser._clickToPlayAllPluginsActivated ||
        browser._clickToPlayPluginsActivated.get(pluginInfo.pluginName)) {
      objLoadingContent.playPlugin();
      return;
    }

    if (overlay) {
      overlay.addEventListener("click", function(aEvent) {
        
        if (!(aEvent.originalTarget instanceof HTMLAnchorElement) &&
            (aEvent.originalTarget.getAttribute('anonid') != 'closeIcon') &&
            aEvent.button == 0 && aEvent.isTrusted) {
          gPluginHandler.activateSinglePlugin(aEvent.target.ownerDocument.defaultView.top, aPlugin);
          aEvent.stopPropagation();
          aEvent.preventDefault();
        }
      }, true);

      let closeIcon = doc.getAnonymousElementByAttribute(aPlugin, "anonid", "closeIcon");
      closeIcon.addEventListener("click", function(aEvent) {
        if (aEvent.button == 0 && aEvent.isTrusted)
          gPluginHandler.hideClickToPlayOverlay(aPlugin);
      }, true);
    }

    gPluginHandler._showClickToPlayNotification(browser);
  },

  _handlePlayPreviewEvent: function PH_handlePlayPreviewEvent(aPlugin) {
    let doc = aPlugin.ownerDocument;
    let previewContent = doc.getAnonymousElementByAttribute(aPlugin, "class", "previewPluginContent");
    let iframe = previewContent.getElementsByClassName("previewPluginContentFrame")[0];
    if (!iframe) {
      
      iframe = doc.createElementNS("http://www.w3.org/1999/xhtml", "iframe");
      iframe.className = "previewPluginContentFrame";
      previewContent.appendChild(iframe);

      
      aPlugin.clientTop;
    }
    let pluginInfo = this._getPluginInfo(aPlugin);
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
    let browser = gBrowser.selectedBrowser;
    if (gPluginHandler._pluginNeedsActivationExceptThese([]))
      gPluginHandler._showClickToPlayNotification(browser);
  },

  
  
  _pluginNeedsActivationExceptThese: function PH_pluginNeedsActivationExceptThese(aExceptThese) {
    let contentWindow = gBrowser.selectedBrowser.contentWindow;
    let cwu = contentWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                           .getInterface(Ci.nsIDOMWindowUtils);
    let pluginNeedsActivation = cwu.plugins.some(function(plugin) {
      let objLoadingContent = plugin.QueryInterface(Ci.nsIObjectLoadingContent);
      return (gPluginHandler.canActivatePlugin(objLoadingContent) &&
              aExceptThese.indexOf(plugin) < 0);
    });

    return pluginNeedsActivation;
  },

  
  _getPluginsByName: function PH_getPluginsByName(aDOMWindowUtils, aName) {
    let plugins = [];
    for (let plugin of aDOMWindowUtils.plugins) {
      let objLoadingContent = plugin.QueryInterface(Ci.nsIObjectLoadingContent);
      if (gPluginHandler.canActivatePlugin(objLoadingContent)) {
        let pluginName = this._getPluginInfo(plugin).pluginName;
        if (aName == pluginName) {
          plugins.push(objLoadingContent);
        }
      }
    }
    return plugins;
  },

  _makeCenterActions: function PH_makeCenterActions(aBrowser) {
    let contentWindow = aBrowser.contentWindow;
    let cwu = contentWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                           .getInterface(Ci.nsIDOMWindowUtils);
    let pluginsDictionary = new Map();
    for (let plugin of cwu.plugins) {
      let objLoadingContent = plugin.QueryInterface(Ci.nsIObjectLoadingContent);
      if (gPluginHandler.canActivatePlugin(objLoadingContent)) {
        let pluginName = this._getPluginInfo(plugin).pluginName;
        if (!pluginsDictionary.has(pluginName))
          pluginsDictionary.set(pluginName, []);
        pluginsDictionary.get(pluginName).push(objLoadingContent);
      }
    }

    let centerActions = [];
    for (let [pluginName, namedPluginArray] of pluginsDictionary) {
      let plugin = namedPluginArray[0];
      let warn = false;
      let warningText = "";
      let updateLink = Services.urlFormatter.formatURLPref("plugins.update.url");
      if (plugin.pluginFallbackType) {
        if (plugin.pluginFallbackType ==
              Ci.nsIObjectLoadingContent.PLUGIN_VULNERABLE_UPDATABLE) {
          warn = true;
          warningText = gNavigatorBundle.getString("vulnerableUpdatablePluginWarning");
        }
        else if (plugin.pluginFallbackType ==
                   Ci.nsIObjectLoadingContent.PLUGIN_VULNERABLE_NO_UPDATE) {
          warn = true;
          warningText = gNavigatorBundle.getString("vulnerableNoUpdatePluginWarning");
          updateLink = "";
        }
      }

      let action = {
        message: pluginName,
        warn: warn,
        warningText: warningText,
        updateLink: updateLink,
        label: gNavigatorBundle.getString("activateSinglePlugin"),
        callback: function() {
          let plugins = gPluginHandler._getPluginsByName(cwu, this.message);
          for (let objLoadingContent of plugins) {
            objLoadingContent.playPlugin();
          }
          aBrowser._clickToPlayPluginsActivated.set(this.message, true);

          let notification = PopupNotifications.getNotification("click-to-play-plugins", aBrowser);
          if (notification &&
              !gPluginHandler._pluginNeedsActivationExceptThese(plugins)) {
            notification.remove();
          }
        }
      };
      centerActions.push(action);
    }

    return centerActions;
  },

  _setPermissionForPlugins: function PH_setPermissionForPlugins(aBrowser, aPermission, aPluginList) {
    let pluginHost = Cc["@mozilla.org/plugin/host;1"].getService(Ci.nsIPluginHost);
    for (let plugin of aPluginList) {
      let objLoadingContent = plugin.QueryInterface(Ci.nsIObjectLoadingContent);
      
      
      if (gPluginHandler.canActivatePlugin(objLoadingContent)) {
        let permissionString = pluginHost.getPermissionStringForType(objLoadingContent.actualType);
        Services.perms.add(aBrowser.currentURI, permissionString, aPermission);
      }
    }
  },

  _showClickToPlayNotification: function PH_showClickToPlayNotification(aBrowser) {
    let contentWindow = aBrowser.contentWindow;
    let messageString = gNavigatorBundle.getString("activatePluginsMessage.message");
    let mainAction = {
      label: gNavigatorBundle.getString("activateAllPluginsMessage.label"),
      accessKey: gNavigatorBundle.getString("activatePluginsMessage.accesskey"),
      callback: function() { gPluginHandler.activatePlugins(contentWindow); }
    };
    let centerActions = gPluginHandler._makeCenterActions(aBrowser);
    let cwu = contentWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                           .getInterface(Ci.nsIDOMWindowUtils);
    let haveVulnerablePlugin = cwu.plugins.some(function(plugin) {
      let objLoadingContent = plugin.QueryInterface(Ci.nsIObjectLoadingContent);
      return (gPluginHandler.canActivatePlugin(objLoadingContent) &&
              (objLoadingContent.pluginFallbackType == Ci.nsIObjectLoadingContent.PLUGIN_VULNERABLE_UPDATABLE ||
               objLoadingContent.pluginFallbackType == Ci.nsIObjectLoadingContent.PLUGIN_VULNERABLE_NO_UPDATE));
    });
    if (haveVulnerablePlugin) {
      messageString = gNavigatorBundle.getString("vulnerablePluginsMessage");
    }
    let secondaryActions = [{
      label: gNavigatorBundle.getString("activatePluginsMessage.always"),
      accessKey: gNavigatorBundle.getString("activatePluginsMessage.always.accesskey"),
      callback: function () {
        gPluginHandler._setPermissionForPlugins(aBrowser, Ci.nsIPermissionManager.ALLOW_ACTION, cwu.plugins);
        gPluginHandler.activatePlugins(contentWindow);
      }
    },{
      label: gNavigatorBundle.getString("activatePluginsMessage.never"),
      accessKey: gNavigatorBundle.getString("activatePluginsMessage.never.accesskey"),
      callback: function () {
        gPluginHandler._setPermissionForPlugins(aBrowser, Ci.nsIPermissionManager.DENY_ACTION, cwu.plugins);
        let notification = PopupNotifications.getNotification("click-to-play-plugins", aBrowser);
        if (notification)
          notification.remove();
        gPluginHandler._removeClickToPlayOverlays(contentWindow);
      }
    }];
    let notification = PopupNotifications.getNotification("click-to-play-plugins", aBrowser);
    let dismissed = notification ? notification.dismissed : true;
    
    if (!isElementVisible(gURLBar))
      dismissed = false;
    let options = { dismissed: dismissed, centerActions: centerActions };
    let icon = haveVulnerablePlugin ? "blocked-plugins-notification-icon" : "plugins-notification-icon"
    PopupNotifications.show(aBrowser, "click-to-play-plugins",
                            messageString, icon,
                            mainAction, secondaryActions, options);
  },

  _removeClickToPlayOverlays: function PH_removeClickToPlayOverlays(aContentWindow) {
    let doc = aContentWindow.document;
    let cwu = aContentWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                            .getInterface(Ci.nsIDOMWindowUtils);
    for (let plugin of cwu.plugins) {
      let overlay = doc.getAnonymousElementByAttribute(plugin, "class", "mainBox");
      
      if (overlay)
        overlay.style.visibility = "hidden";
    }
  },

  
  pluginUnavailable: function (plugin, eventType) {
    let browser = gBrowser.getBrowserForDocument(plugin.ownerDocument
                                                       .defaultView.top.document);
    if (!browser.missingPlugins)
      browser.missingPlugins = new Map();

    var pluginInfo = this._getPluginInfo(plugin);
    browser.missingPlugins.set(pluginInfo.mimetype, pluginInfo);

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
      
      var missingPlugins = gBrowser.selectedBrowser.missingPlugins;
      if (missingPlugins) {
        openDialog("chrome://mozapps/content/plugins/pluginInstallerWizard.xul",
                   "PFSWindow", "chrome,centerscreen,resizable=yes",
                   {plugins: missingPlugins, browser: gBrowser.selectedBrowser});
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
    let pluginDumpID    = aEvent.getData("pluginDumpID");
    let browserDumpID   = aEvent.getData("browserDumpID");

    
    pluginName = this.makeNicePluginName(pluginName);

    let messageString = gNavigatorBundle.getFormattedString("crashedpluginsMessage.title", [pluginName]);

    
    
    

    
    plugin.clientTop;
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
