# -*- Mode: javascript; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:






let CustomizationHandler = {
  handleEvent: function(aEvent) {
    switch(aEvent.type) {
      case "customizationstarting":
        this._customizationStarting();
        break;
      case "customizationending":
        this._customizationEnding(aEvent.detail);
        break;
    }
  },

  isCustomizing: function() {
    return document.documentElement.hasAttribute("customizing") ||
           document.documentElement.hasAttribute("customize-exiting");
  },

  _customizationStarting: function() {
    
    let menubar = document.getElementById("main-menubar");
    for (let childNode of menubar.childNodes)
      childNode.setAttribute("disabled", true);

    let cmd = document.getElementById("cmd_CustomizeToolbars");
    cmd.setAttribute("disabled", "true");

    let splitter = document.getElementById("urlbar-search-splitter");
    if (splitter) {
      splitter.parentNode.removeChild(splitter);
    }

    CombinedStopReload.uninit();
    PlacesToolbarHelper.customizeStart();
    BookmarkingUI.customizeStart();
    DownloadsButton.customizeStart();
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
    BookmarkingUI.customizeDone();
    DownloadsButton.customizeDone();

    
    
    CombinedStopReload.init();
    UpdateUrlbarSearchSplitterState();

    
    if (gURLBar) {
      URLBarSetURI();
      XULBrowserWindow.asyncUpdateUI();
      BookmarkingUI.updateStarState();
      SocialMark.updateMarkState();
    }

    
    let menubar = document.getElementById("main-menubar");
    for (let childNode of menubar.childNodes)
      childNode.setAttribute("disabled", false);
    let cmd = document.getElementById("cmd_CustomizeToolbars");
    cmd.removeAttribute("disabled");

    gBrowser.selectedBrowser.focus();
  }
}
