# -*- indent-tabs-mode: nil; js-indent-level: 2 -*-
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:






let CustomizationHandler = {
  handleEvent: function(aEvent) {
    switch(aEvent.type) {
      case "customizationstarting":
        this._customizationStarting();
        break;
      case "customizationchange":
        this._customizationChange();
        break;
      case "customizationending":
        this._customizationEnding(aEvent.detail);
        break;
    }
  },

  isCustomizing: function() {
    return document.documentElement.hasAttribute("customizing");
  },

  _customizationStarting: function() {
    
    let menubar = document.getElementById("main-menubar");
    for (let childNode of menubar.childNodes)
      childNode.setAttribute("disabled", true);

    let cmd = document.getElementById("cmd_CustomizeToolbars");
    cmd.setAttribute("disabled", "true");

    UpdateUrlbarSearchSplitterState();

    CombinedStopReload.uninit();
    PlacesToolbarHelper.customizeStart();
    DownloadsButton.customizeStart();

    
    
    let tabContainer = gBrowser.tabContainer;
    if (tabContainer.getAttribute("overflow") == "true") {
      let tabstrip = tabContainer.mTabstrip;
      tabstrip.ensureElementIsVisible(gBrowser.selectedTab, true);
    }
  },

  _customizationChange: function() {
    gHomeButton.updatePersonalToolbarStyle();
    PlacesToolbarHelper.customizeChange();
  },

  _customizationEnding: function(aDetails) {
    
    if (aDetails.changed) {
      gURLBar = document.getElementById("urlbar");

      gProxyFavIcon = document.getElementById("page-proxy-favicon");
      gHomeButton.updateTooltip();
      gIdentityHandler._cacheElements();
      XULBrowserWindow.init();

#ifndef XP_MACOSX
      updateEditUIVisibility();
#endif

      
      
      
      if (!window.__lookupGetter__("PopupNotifications")) {
        PopupNotifications.iconBox =
          document.getElementById("notification-popup-box");
      }

    }

    PlacesToolbarHelper.customizeDone();
    DownloadsButton.customizeDone();

    
    
    CombinedStopReload.init();
    UpdateUrlbarSearchSplitterState();

    
    if (gURLBar) {
      URLBarSetURI();
      XULBrowserWindow.asyncUpdateUI();
    }

    
    let menubar = document.getElementById("main-menubar");
    for (let childNode of menubar.childNodes)
      childNode.setAttribute("disabled", false);
    let cmd = document.getElementById("cmd_CustomizeToolbars");
    cmd.removeAttribute("disabled");

    gBrowser.selectedBrowser.focus();
  }
}
