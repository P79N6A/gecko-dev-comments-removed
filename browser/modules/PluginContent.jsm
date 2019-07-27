



"use strict";

let Cc = Components.classes;
let Ci = Components.interfaces;
let Cu = Components.utils;

this.EXPORTED_SYMBOLS = [ "PluginContent" ];

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Timer.jsm");
Cu.import("resource://gre/modules/BrowserUtils.jsm");

XPCOMUtils.defineLazyGetter(this, "gNavigatorBundle", function() {
  const url = "chrome://browser/locale/browser.properties";
  return Services.strings.createBundle(url);
});

XPCOMUtils.defineLazyModuleGetter(this, "AppConstants",
  "resource://gre/modules/AppConstants.jsm");

this.PluginContent = function (global) {
  this.init(global);
}

PluginContent.prototype = {
  init: function (global) {
    this.global = global;
    
    this.content = this.global.content;
    
    this.pluginData = new Map();

    
    global.addEventListener("PluginBindingAttached", this, true, true);
    global.addEventListener("PluginCrashed",         this, true);
    global.addEventListener("PluginOutdated",        this, true);
    global.addEventListener("PluginInstantiated",    this, true);
    global.addEventListener("PluginRemoved",         this, true);
    global.addEventListener("pagehide",              this, true);
    global.addEventListener("pageshow",              this, true);
    global.addEventListener("unload",                this);

    global.addMessageListener("BrowserPlugins:ActivatePlugins", this);
    global.addMessageListener("BrowserPlugins:NotificationShown", this);
    global.addMessageListener("BrowserPlugins:ContextMenuCommand", this);
  },

  uninit: function() {
    delete this.global;
    delete this.content;
  },

  receiveMessage: function (msg) {
    switch (msg.name) {
      case "BrowserPlugins:ActivatePlugins":
        this.activatePlugins(msg.data.pluginInfo, msg.data.newState);
        break;
      case "BrowserPlugins:NotificationShown":
        setTimeout(() => this.updateNotificationUI(), 0);
        break;
      case "BrowserPlugins:ContextMenuCommand":
        switch (msg.data.command) {
          case "play":
            this._showClickToPlayNotification(msg.objects.plugin, true);
            break;
          case "hide":
            this.hideClickToPlayOverlay(msg.objects.plugin);
            break;
        }
        break;
    }
  },

  onPageShow: function (event) {
    
    if (!this.content || event.target != this.content.document) {
      return;
    }

    
    
    
    if (event.persisted) {
      this.reshowClickToPlayNotification();
    }
  },

  onPageHide: function (event) {
    
    if (!this.content || event.target != this.content.document) {
      return;
    }

    this._finishRecordingFlashPluginTelemetry();
    this.clearPluginDataCache();
  },

  getPluginUI: function (plugin, anonid) {
    return plugin.ownerDocument.
           getAnonymousElementByAttribute(plugin, "anonid", anonid);
  },

  _getPluginInfo: function (pluginElement) {
    let pluginHost = Cc["@mozilla.org/plugin/host;1"].getService(Ci.nsIPluginHost);
    pluginElement.QueryInterface(Ci.nsIObjectLoadingContent);

    let tagMimetype;
    let pluginName = gNavigatorBundle.GetStringFromName("pluginInfo.unknownPlugin");
    let pluginTag = null;
    let permissionString = null;
    let fallbackType = null;
    let blocklistState = null;

    tagMimetype = pluginElement.actualType;
    if (tagMimetype == "") {
      tagMimetype = pluginElement.type;
    }

    if (this.isKnownPlugin(pluginElement)) {
      pluginTag = pluginHost.getPluginTagForType(pluginElement.actualType);
      pluginName = BrowserUtils.makeNicePluginName(pluginTag.name);

      
      let properties = ["name", "description", "filename", "version", "enabledState"];
      let pluginTagCopy = {};
      for (let prop of properties) {
        pluginTagCopy[prop] = pluginTag[prop];
      }
      pluginTag = pluginTagCopy;

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

  


  setVisibility : function (plugin, overlay, shouldShow) {
    overlay.classList.toggle("visible", shouldShow);
  },

  







  shouldShowOverlay : function (plugin, overlay) {
    
    
    if (overlay.scrollWidth == 0) {
      return true;
    }

    
    let pluginRect = plugin.getBoundingClientRect();
    
    
    let overflows = (overlay.scrollWidth > Math.ceil(pluginRect.width)) ||
                    (overlay.scrollHeight - 5 > Math.ceil(pluginRect.height));
    if (overflows) {
      return false;
    }

    
    
    let left = pluginRect.left + 2;
    let right = pluginRect.right - 2;
    let top = pluginRect.top + 2;
    let bottom = pluginRect.bottom - 2;
    let centerX = left + (right - left) / 2;
    let centerY = top + (bottom - top) / 2;
    let points = [[left, top],
                   [left, bottom],
                   [right, top],
                   [right, bottom],
                   [centerX, centerY]];

    if (right <= 0 || top <= 0) {
      return false;
    }

    let contentWindow = plugin.ownerDocument.defaultView;
    let cwu = contentWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                           .getInterface(Ci.nsIDOMWindowUtils);

    for (let [x, y] of points) {
      let el = cwu.elementFromPoint(x, y, true, true);
      if (el !== plugin) {
        return false;
      }
    }

    return true;
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

  handleEvent: function (event) {
    let eventType = event.type;

    if (eventType == "unload") {
      this.uninit();
      return;
    }

    if (eventType == "pagehide") {
      this.onPageHide(event);
      return;
    }

    if (eventType == "pageshow") {
      this.onPageShow(event);
      return;
    }

    if (eventType == "PluginRemoved") {
      this.updateNotificationUI(event.target);
      return;
    }

    if (eventType == "click") {
      this.onOverlayClick(event);
      return;
    }

    if (eventType == "PluginCrashed" &&
        !(event.target instanceof Ci.nsIObjectLoadingContent)) {
      
      
      this.pluginInstanceCrashed(event.target, event);
      return;
    }

    let plugin = event.target;
    let doc = plugin.ownerDocument;

    if (!(plugin instanceof Ci.nsIObjectLoadingContent))
      return;

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
    switch (eventType) {
      case "PluginCrashed":
        this.pluginInstanceCrashed(plugin, event);
        break;

      case "PluginNotFound": {
        
        break;
      }

      case "PluginBlocklisted":
      case "PluginOutdated":
        shouldShowNotification = true;
        break;

      case "PluginVulnerableUpdatable":
        let updateLink = this.getPluginUI(plugin, "checkForUpdatesLink");
        this.addLinkClickCallback(updateLink, "forwardCallback", "openPluginUpdatePage");
        

      case "PluginVulnerableNoUpdate":
      case "PluginClickToPlay":
        this._handleClickToPlayEvent(plugin);
        let overlay = this.getPluginUI(plugin, "main");
        let pluginName = this._getPluginInfo(plugin).pluginName;
        let messageString = gNavigatorBundle.formatStringFromName("PluginClickToActivate", [pluginName], 1);
        let overlayText = this.getPluginUI(plugin, "clickToPlay");
        overlayText.textContent = messageString;
        if (eventType == "PluginVulnerableUpdatable" ||
            eventType == "PluginVulnerableNoUpdate") {
          let vulnerabilityString = gNavigatorBundle.GetStringFromName(eventType);
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
        this.addLinkClickCallback(manageLink, "forwardCallback", "managePlugins");
        shouldShowNotification = true;
        break;

      case "PluginInstantiated":
        shouldShowNotification = true;
        break;
    }

    if (this._getPluginInfo(plugin).mimetype === "application/x-shockwave-flash") {
      this._recordFlashPluginTelemetry(eventType, plugin);
    }

    
    if (eventType != "PluginCrashed") {
      let overlay = this.getPluginUI(plugin, "main");
      if (overlay != null) {
        this.setVisibility(plugin, overlay,
                           this.shouldShowOverlay(plugin, overlay));
        let resizeListener = (event) => {
          this.setVisibility(plugin, overlay,
            this.shouldShowOverlay(plugin, overlay));
          this.updateNotificationUI();
        };
        plugin.addEventListener("overflow", resizeListener);
        plugin.addEventListener("underflow", resizeListener);
      }
    }

    let closeIcon = this.getPluginUI(plugin, "closeIcon");
    if (closeIcon) {
      closeIcon.addEventListener("click", event => {
        if (event.button == 0 && event.isTrusted)
          this.hideClickToPlayOverlay(plugin);
      }, true);
    }

    if (shouldShowNotification) {
      this._showClickToPlayNotification(plugin, false);
    }
  },

  _recordFlashPluginTelemetry: function (eventType, plugin) {
    if (!Services.telemetry.canRecordExtended) {
      return;
    }

    if (!this.flashPluginStats) {
      this.flashPluginStats = {
        instancesCount: 0,
        plugins: new WeakSet()
      };
    }

    if (!this.flashPluginStats.plugins.has(plugin)) {
      
      this.flashPluginStats.plugins.add(plugin);

      this.flashPluginStats.instancesCount++;

      let pluginRect = plugin.getBoundingClientRect();
      Services.telemetry.getHistogramById('FLASH_PLUGIN_WIDTH')
                       .add(pluginRect.width);
      Services.telemetry.getHistogramById('FLASH_PLUGIN_HEIGHT')
                       .add(pluginRect.height);
      Services.telemetry.getHistogramById('FLASH_PLUGIN_AREA')
                       .add(pluginRect.width * pluginRect.height);

      let state = this._getPluginInfo(plugin).fallbackType;
      if (state === null) {
        state = Ci.nsIObjectLoadingContent.PLUGIN_UNSUPPORTED;
      }
      Services.telemetry.getHistogramById('FLASH_PLUGIN_STATES')
                       .add(state);
    }
  },

  _finishRecordingFlashPluginTelemetry: function () {
    if (this.flashPluginStats) {
      Services.telemetry.getHistogramById('FLASH_PLUGIN_INSTANCES_ON_PAGE')
                        .add(this.flashPluginStats.instancesCount);
    delete this.flashPluginStats;
    }
  },

  isKnownPlugin: function (objLoadingContent) {
    return (objLoadingContent.getContentTypeForMIMEType(objLoadingContent.actualType) ==
            Ci.nsIObjectLoadingContent.TYPE_PLUGIN);
  },

  canActivatePlugin: function (objLoadingContent) {
    
    
    
    if (!this.isKnownPlugin(objLoadingContent))
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

  hideClickToPlayOverlay: function (plugin) {
    let overlay = this.getPluginUI(plugin, "main");
    if (overlay) {
      overlay.classList.remove("visible");
    }
  },

  stopPlayPreview: function (plugin, playPlugin) {
    let objLoadingContent = plugin.QueryInterface(Ci.nsIObjectLoadingContent);
    if (objLoadingContent.activated)
      return;

    if (playPlugin)
      objLoadingContent.playPlugin();
    else
      objLoadingContent.cancelPlayPreview();
  },

  
  forwardCallback: function (name) {
    this.global.sendAsyncMessage("PluginContent:LinkClickCallback", { name: name });
  },

  submitReport: function submitReport(pluginDumpID, browserDumpID, plugin) {
    if (!AppConstants.MOZ_CRASHREPORTER) {
      return;
    }
    let keyVals = {};
    if (plugin) {
      let userComment = this.getPluginUI(plugin, "submitComment").value.trim();
      if (userComment)
        keyVals.PluginUserComment = userComment;
      if (this.getPluginUI(plugin, "submitURLOptIn").checked)
        keyVals.PluginContentURL = plugin.ownerDocument.URL;
    }

    this.global.sendAsyncMessage("PluginContent:SubmitReport", {
      pluginDumpID: pluginDumpID,
      browserDumpID: browserDumpID,
      keyVals: keyVals,
    });
  },

  reloadPage: function () {
    this.global.content.location.reload();
  },

  
  _handleClickToPlayEvent: function (plugin) {
    let doc = plugin.ownerDocument;
    let pluginHost = Cc["@mozilla.org/plugin/host;1"].getService(Ci.nsIPluginHost);
    let objLoadingContent = plugin.QueryInterface(Ci.nsIObjectLoadingContent);
    
    
    if (!this.isKnownPlugin(objLoadingContent))
      return;
    let permissionString = pluginHost.getPermissionStringForType(objLoadingContent.actualType);
    let principal = doc.defaultView.top.document.nodePrincipal;
    let pluginPermission = Services.perms.testPermissionFromPrincipal(principal, permissionString);

    let overlay = this.getPluginUI(plugin, "main");

    if (pluginPermission == Ci.nsIPermissionManager.DENY_ACTION) {
      if (overlay) {
        overlay.classList.remove("visible");
      }
      return;
    }

    if (overlay) {
      overlay.addEventListener("click", this, true);
    }
  },

  onOverlayClick: function (event) {
    let document = event.target.ownerDocument;
    let plugin = document.getBindingParent(event.target);
    let contentWindow = plugin.ownerDocument.defaultView.top;
    let objLoadingContent = plugin.QueryInterface(Ci.nsIObjectLoadingContent);
    
    if (!(event.originalTarget instanceof contentWindow.HTMLAnchorElement) &&
        (event.originalTarget.getAttribute('anonid') != 'closeIcon') &&
          event.button == 0 && event.isTrusted) {
      this._showClickToPlayNotification(plugin, true);
    event.stopPropagation();
    event.preventDefault();
    }
  },

  _handlePlayPreviewEvent: function (plugin) {
    let doc = plugin.ownerDocument;
    let pluginHost = Cc["@mozilla.org/plugin/host;1"].getService(Ci.nsIPluginHost);
    let pluginInfo = this._getPluginInfo(plugin);
    let playPreviewInfo = pluginHost.getPlayPreviewInfo(pluginInfo.mimetype);

    let previewContent = this.getPluginUI(plugin, "previewPluginContent");
    let iframe = previewContent.getElementsByClassName("previewPluginContentFrame")[0];
    if (!iframe) {
      
      iframe = doc.createElementNS("http://www.w3.org/1999/xhtml", "iframe");
      iframe.className = "previewPluginContentFrame";
      previewContent.appendChild(iframe);

      
      plugin.clientTop;
    }
    iframe.src = playPreviewInfo.redirectURL;

    
    
    let playPluginHandler = (event) => {
      if (!event.isTrusted)
        return;

      previewContent.removeEventListener("MozPlayPlugin", playPluginHandler, true);

      let playPlugin = !event.detail;
      this.stopPlayPreview(plugin, playPlugin);

      
      let iframe = previewContent.getElementsByClassName("previewPluginContentFrame")[0];
      if (iframe)
        previewContent.removeChild(iframe);
    };

    previewContent.addEventListener("MozPlayPlugin", playPluginHandler, true);

    if (!playPreviewInfo.ignoreCTP) {
      this._showClickToPlayNotification(plugin, false);
    }
  },

  reshowClickToPlayNotification: function () {
    let contentWindow = this.global.content;
    let cwu = contentWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                           .getInterface(Ci.nsIDOMWindowUtils);
    let plugins = cwu.plugins;
    for (let plugin of plugins) {
      let overlay = this.getPluginUI(plugin, "main");
      if (overlay)
        overlay.removeEventListener("click", this, true);
      let objLoadingContent = plugin.QueryInterface(Ci.nsIObjectLoadingContent);
      if (this.canActivatePlugin(objLoadingContent))
        this._handleClickToPlayEvent(plugin);
    }
    this._showClickToPlayNotification(null, false);
  },

  
  _getHostFromPrincipal: function (principal) {
    if (!principal.URI || principal.URI.schemeIs("moz-nullprincipal")) {
      return "(null)";
    }

    try {
      if (principal.URI.host)
        return principal.URI.host;
    } catch (e) {}

    return principal.origin;
  },

  


  activatePlugins: function (pluginInfo, newState) {
    let contentWindow = this.global.content;
    let cwu = contentWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                           .getInterface(Ci.nsIDOMWindowUtils);
    let plugins = cwu.plugins;
    let pluginHost = Cc["@mozilla.org/plugin/host;1"].getService(Ci.nsIPluginHost);

    let pluginFound = false;
    for (let plugin of plugins) {
      plugin.QueryInterface(Ci.nsIObjectLoadingContent);
      if (!this.isKnownPlugin(plugin)) {
        continue;
      }
      if (pluginInfo.permissionString == pluginHost.getPermissionStringForType(plugin.actualType)) {
        pluginFound = true;
        if (newState == "block") {
          plugin.reload(true);
        } else {
          if (this.canActivatePlugin(plugin)) {
            let overlay = this.getPluginUI(plugin, "main");
            if (overlay) {
              overlay.removeEventListener("click", this, true);
            }
            plugin.playPlugin();
          }
        }
      }
    }

    
    
    if (newState != "block" && !pluginFound) {
      this.reloadPage();
    }
    this.updateNotificationUI();
  },

  _showClickToPlayNotification: function (plugin, showNow) {
    let plugins = [];

    
    
    if (plugin === null) {
      let contentWindow = this.content;
      let cwu = contentWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                             .getInterface(Ci.nsIDOMWindowUtils);
      
      plugins = cwu.plugins.filter((plugin) =>
        plugin.getContentTypeForMIMEType(plugin.actualType) == Ci.nsIObjectLoadingContent.TYPE_PLUGIN);

      if (plugins.length == 0) {
        this.removeNotification("click-to-play-plugins");
        return;
      }
    } else {
      plugins = [plugin];
    }

    let pluginData = this.pluginData;

    let principal = this.content.document.nodePrincipal;
    let principalHost = this._getHostFromPrincipal(principal);
    let location = this.content.document.location.href;

    for (let p of plugins) {
      let pluginInfo = this._getPluginInfo(p);
      if (pluginInfo.permissionString === null) {
        Cu.reportError("No permission string for active plugin.");
        continue;
      }
      if (pluginData.has(pluginInfo.permissionString)) {
        continue;
      }

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

      this.pluginData.set(pluginInfo.permissionString, pluginInfo);
    }

    this.global.sendAsyncMessage("PluginContent:ShowClickToPlayNotification", {
      plugins: [... this.pluginData.values()],
      showNow: showNow,
      host: principalHost,
      location: location,
    }, null, principal);
  },

  










  updateNotificationUI: function (document) {
    document = document || this.content.document;

    
    
    
    let principal = document.defaultView.top.document.nodePrincipal;
    let location = document.location.href;

    
    let haveInsecure = false;
    let actions = new Map();
    for (let action of this.pluginData.values()) {
      switch (action.fallbackType) {
        
        
        case Ci.nsIObjectLoadingContent.PLUGIN_VULNERABLE_UPDATABLE:
        case Ci.nsIObjectLoadingContent.PLUGIN_VULNERABLE_NO_UPDATE:
          haveInsecure = true;
          

        case Ci.nsIObjectLoadingContent.PLUGIN_CLICK_TO_PLAY:
          actions.set(action.permissionString, action);
          continue;
      }
    }

    
    let cwu = this.content.QueryInterface(Ci.nsIInterfaceRequestor)
                          .getInterface(Ci.nsIDOMWindowUtils);
    for (let plugin of cwu.plugins) {
      let info = this._getPluginInfo(plugin);
      if (!actions.has(info.permissionString)) {
        continue;
      }
      let fallbackType = info.fallbackType;
      if (fallbackType == Ci.nsIObjectLoadingContent.PLUGIN_ACTIVE) {
        actions.delete(info.permissionString);
        if (actions.size == 0) {
          break;
        }
        continue;
      }
      if (fallbackType != Ci.nsIObjectLoadingContent.PLUGIN_CLICK_TO_PLAY &&
          fallbackType != Ci.nsIObjectLoadingContent.PLUGIN_VULNERABLE_UPDATABLE &&
          fallbackType != Ci.nsIObjectLoadingContent.PLUGIN_VULNERABLE_NO_UPDATE) {
        continue;
      }
      let overlay = this.getPluginUI(plugin, "main");
      if (!overlay) {
        continue;
      }
      let shouldShow = this.shouldShowOverlay(plugin, overlay);
      this.setVisibility(plugin, overlay, shouldShow);
      if (shouldShow) {
        actions.delete(info.permissionString);
        if (actions.size == 0) {
          break;
        }
      }
    }

    
    
    this.global.sendAsyncMessage("PluginContent:UpdateHiddenPluginUI", {
      haveInsecure: haveInsecure,
      actions: [... actions.values()],
      host: this._getHostFromPrincipal(principal),
      location: location,
    }, null, principal);
  },

  removeNotification: function (name) {
    this.global.sendAsyncMessage("PluginContent:RemoveNotification", { name: name });
  },

  clearPluginDataCache: function () {
    this.pluginData.clear();
  },

  hideNotificationBar: function (name) {
    this.global.sendAsyncMessage("PluginContent:HideNotificationBar", { name: name });
  },

  
  
  pluginInstanceCrashed: function (target, aEvent) {
    if (!(aEvent instanceof this.content.PluginCrashedEvent))
      return;

    let submittedReport = aEvent.submittedCrashReport;
    let doPrompt        = true; 
    let submitReports   = true; 
    let pluginName      = aEvent.pluginName;
    let pluginDumpID    = aEvent.pluginDumpID;
    let browserDumpID   = aEvent.browserDumpID;
    let gmpPlugin       = aEvent.gmpPlugin;

    
    if (!gmpPlugin) {
      pluginName = BrowserUtils.makeNicePluginName(pluginName);
    }

    let messageString = gNavigatorBundle.formatStringFromName("crashedpluginsMessage.title", [pluginName], 1);

    let plugin = null, doc;
    if (target instanceof Ci.nsIObjectLoadingContent) {
      plugin = target;
      doc = plugin.ownerDocument;
    } else {
      doc = target.document;
      if (!doc) {
        return;
      }
      
      
      doPrompt = false;
    }

    let status;
    
    if (submittedReport) { 
      status = "submitted";
    }
    else if (!submitReports && !doPrompt) {
      status = "noSubmit";
    }
    else if (!pluginDumpID) {
      
      
      status = "noReport";
    }
    else {
      status = "please";
    }

    
    
    if (!pluginDumpID) {
        status = "noReport";
    }

    
    
    
    if (AppConstants.MOZ_CRASHREPORTER && doPrompt) {
      let observer = {
        QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                               Ci.nsISupportsWeakReference]),
        observe : (subject, topic, data) => {
          let propertyBag = subject;
          if (!(propertyBag instanceof Ci.nsIPropertyBag2))
            return;
          
          if (propertyBag.get("minidumpID") != pluginDumpID)
            return;
          let statusDiv = this.getPluginUI(plugin, "submitStatus");
          statusDiv.setAttribute("status", data);
        },

        handleEvent : function(event) {
            
        }
      }

      
      Services.obs.addObserver(observer, "crash-report-status", true);
      
      
      
      
      
      doc.addEventListener("mozCleverClosureHack", observer, false);
    }

    let isShowing = false;

    if (plugin) {
      
      
      isShowing = _setUpPluginOverlay.call(this, plugin, doPrompt);
    }

    if (isShowing) {
      
      
      
      this.hideNotificationBar("plugin-crashed");
      doc.mozNoPluginCrashedNotification = true;
    } else {
      
      
      if (!doc.mozNoPluginCrashedNotification) {
        this.global.sendAsyncMessage("PluginContent:ShowPluginCrashedNotification", {
          messageString: messageString,
          pluginDumpID: pluginDumpID,
          browserDumpID: browserDumpID,
        });
        
        doc.defaultView.top.addEventListener("unload", event => {
          this.hideNotificationBar("plugin-crashed");
        }, false);
      }
    }

    
    
    function _setUpPluginOverlay(plugin, doPromptSubmit) {
      if (!plugin) {
        return false;
      }

      
      plugin.clientTop;
      let overlay = this.getPluginUI(plugin, "main");
      let statusDiv = this.getPluginUI(plugin, "submitStatus");

      if (doPromptSubmit) {
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

      statusDiv.setAttribute("status", status);

      let helpIcon = this.getPluginUI(plugin, "helpIcon");
      this.addLinkClickCallback(helpIcon, "openHelpPage");

      let crashText = this.getPluginUI(plugin, "crashedText");
      crashText.textContent = messageString;

      let link = this.getPluginUI(plugin, "reloadLink");
      this.addLinkClickCallback(link, "reloadPage");

      let isShowing = this.shouldShowOverlay(plugin, overlay);

      
      if (!isShowing) {
        
        statusDiv.removeAttribute("status");

        isShowing = this.shouldShowOverlay(plugin, overlay);
      }
      this.setVisibility(plugin, overlay, isShowing);

      return isShowing;
    }
  }
};
