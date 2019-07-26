


"use strict";

var PluginHelper = {
  showDoorHanger: function(aTab) {
    if (!aTab.browser)
      return;

    
    
    aTab.shouldShowPluginDoorhanger = false;

    let uri = aTab.browser.currentURI;

    
    
    let permValue = Services.perms.testPermission(uri, "plugins");
    if (permValue != Services.perms.UNKNOWN_ACTION) {
      if (permValue == Services.perms.ALLOW_ACTION)
        PluginHelper.playAllPlugins(aTab.browser.contentWindow);

      return;
    }

    let message = Strings.browser.formatStringFromName("clickToPlayPlugins.message2",
                                                       [uri.host], 1);
    let buttons = [
      {
        label: Strings.browser.GetStringFromName("clickToPlayPlugins.activate"),
        callback: function(aChecked) {
          
          if (aChecked)
            Services.perms.add(uri, "plugins", Ci.nsIPermissionManager.ALLOW_ACTION);

          PluginHelper.playAllPlugins(aTab.browser.contentWindow);
        }
      },
      {
        label: Strings.browser.GetStringFromName("clickToPlayPlugins.dontActivate"),
        callback: function(aChecked) {
          
          if (aChecked)
            Services.perms.add(uri, "plugins", Ci.nsIPermissionManager.DENY_ACTION);

          
        }
      }
    ];

    
    
    let options = uri.host ? { checkbox: Strings.browser.GetStringFromName("clickToPlayPlugins.dontAskAgain") } : {};

    NativeWindow.doorhanger.show(message, "ask-to-play-plugins", buttons, aTab.id, options);
  },

  delayAndShowDoorHanger: function(aTab) {
    
    
    
    if (!aTab.pluginDoorhangerTimeout) {
      aTab.pluginDoorhangerTimeout = setTimeout(function() {
        if (this.shouldShowPluginDoorhanger) {
          PluginHelper.showDoorHanger(this);
        }
      }.bind(aTab), 500);
    }
  },

  playAllPlugins: function(aContentWindow) {
    let cwu = aContentWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                            .getInterface(Ci.nsIDOMWindowUtils);
    
    let plugins = cwu.plugins;
    if (!plugins || !plugins.length)
      return;

    plugins.forEach(this.playPlugin);
  },

  playPlugin: function(plugin) {
    let objLoadingContent = plugin.QueryInterface(Ci.nsIObjectLoadingContent);
    if (!objLoadingContent.activated)
      objLoadingContent.playPlugin();
  },

  stopPlayPreview: function(plugin, playPlugin) {
    let objLoadingContent = plugin.QueryInterface(Ci.nsIObjectLoadingContent);
    if (objLoadingContent.activated)
      return;

    if (playPlugin)
      objLoadingContent.playPlugin();
    else
      objLoadingContent.cancelPlayPreview();
  },

  getPluginPreference: function getPluginPreference() {
    let pluginDisable = Services.prefs.getBoolPref("plugin.disable");
    if (pluginDisable)
      return "0";

    let clickToPlay = Services.prefs.getBoolPref("plugins.click_to_play");
    return clickToPlay ? "2" : "1";
  },

  setPluginPreference: function setPluginPreference(aValue) {
    switch (aValue) {
      case "0": 
        Services.prefs.setBoolPref("plugin.disable", true);
        Services.prefs.clearUserPref("plugins.click_to_play");
        break;
      case "1": 
        Services.prefs.clearUserPref("plugin.disable");
        Services.prefs.setBoolPref("plugins.click_to_play", false);
        break;
      case "2": 
        Services.prefs.clearUserPref("plugin.disable");
        Services.prefs.clearUserPref("plugins.click_to_play");
        break;
    }
  },

  
  isTooSmall : function (plugin, overlay) {
    
    let pluginRect = plugin.getBoundingClientRect();
    
    
    let overflows = (overlay.scrollWidth > pluginRect.width) ||
                    (overlay.scrollHeight - 5 > pluginRect.height);

    return overflows;
  },

  getPluginMimeType: function (plugin) {
    var tagMimetype;
    if (plugin instanceof HTMLAppletElement) {
      tagMimetype = "application/x-java-vm";
    } else {
      tagMimetype = plugin.QueryInterface(Components.interfaces.nsIObjectLoadingContent)
                          .actualType;

      if (tagMimetype == "") {
        tagMimetype = plugin.type;
      }
    }

    return tagMimetype;
  },

  handlePluginBindingAttached: function (aTab, aEvent) {
    let plugin = aEvent.target;
    let doc = plugin.ownerDocument;
    let overlay = doc.getAnonymousElementByAttribute(plugin, "class", "mainBox");
    if (!overlay || overlay._bindingHandled) {
      return;
    }
    overlay._bindingHandled = true;

    let eventType = PluginHelper._getBindingType(plugin);
    if (!eventType) {
      
      return;
    }

    switch  (eventType) {
      case "PluginClickToPlay": {
        
        
        if (aTab.clickToPlayPluginsActivated ||
            Services.perms.testPermission(aTab.browser.currentURI, "plugins") ==
            Services.perms.ALLOW_ACTION) {
          PluginHelper.playPlugin(plugin);
          return;
        }

        
        
        if (PluginHelper.isTooSmall(plugin, overlay)) {
          PluginHelper.delayAndShowDoorHanger(aTab);
        } else {
          
          
          aTab.shouldShowPluginDoorhanger = false;
        }

        
        overlay.addEventListener("click", function(e) {
          if (!e.isTrusted)
            return;
          e.preventDefault();
          let win = e.target.ownerDocument.defaultView.top;
          let tab = BrowserApp.getTabForWindow(win);
          tab.clickToPlayPluginsActivated = true;
          PluginHelper.playAllPlugins(win);

          NativeWindow.doorhanger.hide("ask-to-play-plugins", tab.id);
        }, true);
        break;
      }

      case "PluginPlayPreview": {
        let previewContent = doc.getAnonymousElementByAttribute(plugin, "class", "previewPluginContent");
        let pluginHost = Cc["@mozilla.org/plugin/host;1"].getService(Ci.nsIPluginHost);
        let mimeType = PluginHelper.getPluginMimeType(plugin);
        let playPreviewInfo = pluginHost.getPlayPreviewInfo(mimeType);

        if (!playPreviewInfo.ignoreCTP) {
          
          
          if (aTab.clickToPlayPluginsActivated ||
              Services.perms.testPermission(aTab.browser.currentURI, "plugins") ==
              Services.perms.ALLOW_ACTION) {
            PluginHelper.playPlugin(plugin);
            return;
          }

          
          PluginHelper.delayAndShowDoorHanger(aTab);
        }

        let iframe = previewContent.getElementsByClassName("previewPluginContentFrame")[0];
        if (!iframe) {
          
          iframe = doc.createElementNS("http://www.w3.org/1999/xhtml", "iframe");
          iframe.className = "previewPluginContentFrame";
          previewContent.appendChild(iframe);
        }
        iframe.src = playPreviewInfo.redirectURL;

        
        
        previewContent.addEventListener("MozPlayPlugin", function playPluginHandler(e) {
          if (!e.isTrusted)
            return;

          previewContent.removeEventListener("MozPlayPlugin", playPluginHandler, true);

          let playPlugin = !aEvent.detail;
          PluginHelper.stopPlayPreview(plugin, playPlugin);

          
          let iframe = previewContent.getElementsByClassName("previewPluginContentFrame")[0];
          if (iframe)
            previewContent.removeChild(iframe);
        }, true);
        break;
      }

      case "PluginNotFound": {
        
        
        let learnMoreLink = doc.getAnonymousElementByAttribute(plugin, "class", "unsupportedLearnMoreLink");
        let learnMoreUrl = Services.urlFormatter.formatURLPref("app.support.baseURL");
        learnMoreUrl += "why-cant-firefox-mobile-play-flash-on-my-device";
        learnMoreLink.href = learnMoreUrl;
        break;
      }
    }
  },

  
  _getBindingType: function(plugin) {
    if (!(plugin instanceof Ci.nsIObjectLoadingContent))
      return null;

    switch (plugin.pluginFallbackType) {
      case Ci.nsIObjectLoadingContent.PLUGIN_UNSUPPORTED:
        return "PluginNotFound";
      case Ci.nsIObjectLoadingContent.PLUGIN_CLICK_TO_PLAY:
        return "PluginClickToPlay";
      case Ci.nsIObjectLoadingContent.PLUGIN_PLAY_PREVIEW:
        return "PluginPlayPreview";
      default:
        
        return null;
    }
  }
};
