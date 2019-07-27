


"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Messaging.jsm");
Cu.import("resource://gre/modules/UITelemetry.jsm");

this.EXPORTED_SYMBOLS = ["NetErrorHelper"];














let handlers = {};

function NetErrorHelper(browser) {
  browser.addEventListener("click", this.handleClick, true);

  let listener = () => {
    browser.removeEventListener("click", this.handleClick, true);
    browser.removeEventListener("pagehide", listener, true);
  };
  browser.addEventListener("pagehide", listener, true);

  
  for (let id in handlers) {
    if (handlers[id].onPageShown) {
      handlers[id].onPageShown(browser);
    }
  }
}

NetErrorHelper.attachToBrowser = function(browser) {
  return new NetErrorHelper(browser);
}

NetErrorHelper.prototype = {
  handleClick: function(event) {
    let node = event.target;

    while(node) {
      if (node.id in handlers && handlers[node.id].handleClick) {
        handlers[node.id].handleClick(event);
        return;
      }

      node = node.parentNode;
    }
  },
}

handlers.wifi = {
  
  
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference]),

  GetWeakReference: function() {
    return Cu.getWeakReference(this);
  },

  onPageShown: function(browser) {
      
      let network = Cc["@mozilla.org/network/network-link-service;1"].getService(Ci.nsINetworkLinkService);
      if (network.isLinkUp && network.linkStatusKnown) {
        let nodes = browser.contentDocument.querySelectorAll("#wifi");
        for (let i = 0; i < nodes.length; i++) {
          nodes[i].style.display = "none";
        }
      }
  },

  handleClick: function(event) {
    let node = event.target;
    while(node && node.id !== "wifi") {
      node = node.parentNode;
    }

    if (!node) {
      return;
    }

    UITelemetry.addEvent("neterror.1", "button", "wifitoggle");
    
    node.disabled = true;
    node.classList.add("inProgress");

    this.node = Cu.getWeakReference(node);
    Services.obs.addObserver(this, "network:link-status-changed", true);

    Messaging.sendRequest({
      type: "Wifi:Enable"
    });
  },

  observe: function(subject, topic, data) {
    let node = this.node.get();
    if (!node) {
      return;
    }

    
    node.disabled = false;
    node.classList.remove("inProgress");

    let network = Cc["@mozilla.org/network/network-link-service;1"].getService(Ci.nsINetworkLinkService);
    if (network.isLinkUp && network.linkStatusKnown) {
      
      UITelemetry.addEvent("neterror.1", "button", "wifitoggle.reload");
      Services.obs.removeObserver(this, "network:link-status-changed");

      
      
      node.ownerDocument.defaultView.setTimeout(function() {
        node.ownerDocument.location.reload(false);
      }, 500);
    }
  }
}

