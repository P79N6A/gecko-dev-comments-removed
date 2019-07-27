



"use strict";

const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/PrivateBrowsingUtils.jsm");

XPCOMUtils.defineLazyGetter(window, "gChromeWin", function()
  window.QueryInterface(Ci.nsIInterfaceRequestor)
    .getInterface(Ci.nsIWebNavigation)
    .QueryInterface(Ci.nsIDocShellTreeItem)
    .rootTreeItem
    .QueryInterface(Ci.nsIInterfaceRequestor)
    .getInterface(Ci.nsIDOMWindow)
    .QueryInterface(Ci.nsIDOMChromeWindow));

document.addEventListener("DOMContentLoaded", function() {
    let BrowserApp = gChromeWin.BrowserApp;

    if (!PrivateBrowsingUtils.isContentWindowPrivate(window)) {
      document.body.setAttribute("class", "normal");
      document.getElementById("newPrivateTabLink").addEventListener("click", function() {
        BrowserApp.addTab("about:privatebrowsing", { selected: true, parentId: BrowserApp.selectedTab.id, isPrivate: true });
      }, false);
    }
  }, false);
