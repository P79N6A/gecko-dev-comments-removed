


"use strict";

dump("### PluginHelper.js loaded\n");









var PluginHelper = {
  init: function () {
    addEventListener("PluginBindingAttached", this, true, true);
  },

  handleEvent: function handleEvent(aEvent) {
    switch (aEvent.type) {
      case "PluginBindingAttached":
        this.handlePluginBindingAttached(aEvent);
        break;
    }
  },

  getPluginMimeType: function (plugin) {
    var tagMimetype;
    if (plugin instanceof plugin.ownerDocument.defaultView.HTMLAppletElement) {
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

  handlePluginBindingAttached: function (aEvent) {
    let plugin = aEvent.target;
    let doc = plugin.ownerDocument;
    let overlay = doc.getAnonymousElementByAttribute(plugin, "anonid", "main");
    if (!overlay || overlay._bindingHandled) {
      return;
    }
    overlay._bindingHandled = true;

    let eventType = PluginHelper._getBindingType(plugin);
    if (!eventType) {
      return;
    }

    switch  (eventType) {
      case "PluginPlayPreview": {
        
        let previewContent = doc.getAnonymousElementByAttribute(plugin, "class", "previewPluginContent");
        let pluginHost = Cc["@mozilla.org/plugin/host;1"].getService(Ci.nsIPluginHost);
        let mimeType = PluginHelper.getPluginMimeType(plugin);
        let playPreviewInfo = pluginHost.getPlayPreviewInfo(mimeType);

        let iframe = previewContent.getElementsByClassName("previewPluginContentFrame")[0];
        if (!iframe) {
          
          iframe = doc.createElementNS("http://www.w3.org/1999/xhtml", "iframe");
          iframe.className = "previewPluginContentFrame";
          previewContent.appendChild(iframe);
        }
        iframe.src = playPreviewInfo.redirectURL;
        break;
      }

      case "PluginNotFound": {
        
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
      case Ci.nsIObjectLoadingContent.PLUGIN_PLAY_PREVIEW:
        return "PluginPlayPreview";
      default:
        
        return null;
    }
  },
};

PluginHelper.init();
