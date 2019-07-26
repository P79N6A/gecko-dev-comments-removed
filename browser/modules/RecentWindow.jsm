



"use strict";

this.EXPORTED_SYMBOLS = ["RecentWindow"];

const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/PrivateBrowsingUtils.jsm");

#ifdef XP_UNIX
#ifndef XP_MACOSX
#define BROKEN_WM_Z_ORDER
#endif
#endif

this.RecentWindow = {
  








  getMostRecentBrowserWindow: function RW_getMostRecentBrowserWindow(aOptions) {
    let checkPrivacy = typeof aOptions == "object" &&
                       "private" in aOptions;

    let allowPopups = typeof aOptions == "object" && !!aOptions.allowPopups;

    function isSuitableBrowserWindow(win) {
      return (!win.closed &&
              (allowPopups || win.toolbar.visible) &&
              (!checkPrivacy ||
               PrivateBrowsingUtils.permanentPrivateBrowsing ||
               PrivateBrowsingUtils.isWindowPrivate(win) == aOptions.private));
    }

#ifdef BROKEN_WM_Z_ORDER
    let win = Services.wm.getMostRecentWindow("navigator:browser");

    
    if (win && !isSuitableBrowserWindow(win)) {
      win = null;
      let windowList = Services.wm.getEnumerator("navigator:browser");
      
      while (windowList.hasMoreElements()) {
        let nextWin = windowList.getNext();
        if (isSuitableBrowserWindow(nextWin))
          win = nextWin;
      }
    }
    return win;
#else
    let windowList = Services.wm.getZOrderDOMWindowEnumerator("navigator:browser", true);
    while (windowList.hasMoreElements()) {
      let win = windowList.getNext();
      if (isSuitableBrowserWindow(win))
        return win;
    }
    return null;
#endif
  }
};

