


"use strict";



this.EXPORTED_SYMBOLS = ["Chat"];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PrivateBrowsingUtils",
  "resource://gre/modules/PrivateBrowsingUtils.jsm");


function isWindowChromeless(win) {
  
  

  
  let docElem = win.document.documentElement;
  
  
  let chromeless = docElem.getAttribute("chromehidden").includes("extrachrome") ||
                   docElem.getAttribute('chromehidden').includes("toolbar");
  return chromeless;
}

function isWindowGoodForChats(win) {
  return !win.closed &&
         !!win.document.getElementById("pinnedchats") &&
         !isWindowChromeless(win) &&
         !PrivateBrowsingUtils.isWindowPrivate(win);
}

function getChromeWindow(contentWin) {
  return contentWin.QueryInterface(Ci.nsIInterfaceRequestor)
                   .getInterface(Ci.nsIWebNavigation)
                   .QueryInterface(Ci.nsIDocShellTreeItem)
                   .rootTreeItem
                   .QueryInterface(Ci.nsIInterfaceRequestor)
                   .getInterface(Ci.nsIDOMWindow);
}





let Chat = {

  


  get chatboxes() {
    return function*() {
      let winEnum = Services.wm.getEnumerator("navigator:browser");
      while (winEnum.hasMoreElements()) {
        let win = winEnum.getNext();
        let chatbar = win.document.getElementById("pinnedchats");
        if (!chatbar)
          continue;

        
        
        let chatboxes = [c for (c of chatbar.children)];
        for (let chatbox of chatboxes) {
          yield chatbox;
        }
      }

      
      winEnum = Services.wm.getEnumerator("Social:Chat");
      while (winEnum.hasMoreElements()) {
        let win = winEnum.getNext();
        if (win.closed)
          continue;
        yield win.document.getElementById("chatter");
      }
    }();
  },

  
























  open: function(contentWindow, origin, title, url, mode, focus, callback) {
    let chromeWindow = this.findChromeWindowForChats(contentWindow);
    if (!chromeWindow) {
      Cu.reportError("Failed to open a chat window - no host window could be found.");
      return null;
    }

    let chatbar = chromeWindow.document.getElementById("pinnedchats");
    chatbar.hidden = false;
    let chatbox = chatbar.openChat(origin, title, url, mode, callback);
    
    
    chromeWindow.getAttention();
    
    
    if (focus === undefined) {
      let dwu = chromeWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                            .getInterface(Ci.nsIDOMWindowUtils);
      focus = dwu.isHandlingUserInput;
    }
    if (focus) {
      chatbar.focus();
    }
    return chatbox;
  },

  





  closeAll: function(origin) {
    for (let chatbox of this.chatboxes) {
      if (chatbox.content.getAttribute("origin") != origin) {
        continue;
      }
      chatbox.close();
    }
  },

  




  focus: function(win) {
    let chatbar = win.document.getElementById("pinnedchats");
    if (chatbar && !chatbar.hidden) {
      chatbar.focus();
    }

  },

  
  
  findChromeWindowForChats: function(preferredWindow) {
    if (preferredWindow) {
      preferredWindow = getChromeWindow(preferredWindow);
      if (isWindowGoodForChats(preferredWindow)) {
        return preferredWindow;
      }
    }
    
    
    

    
    
    
    

    let mostRecent = Services.wm.getMostRecentWindow("navigator:browser");
    if (isWindowGoodForChats(mostRecent))
      return mostRecent;

    let topMost, enumerator;
    
    
    
    
    let os = Services.appinfo.OS;
    const BROKEN_WM_Z_ORDER = os != "WINNT" && os != "Darwin";
    if (BROKEN_WM_Z_ORDER) {
      
      enumerator = Services.wm.getEnumerator("navigator:browser");
    } else {
      
      
      enumerator = Services.wm.getZOrderDOMWindowEnumerator("navigator:browser", false);
    }
    while (enumerator.hasMoreElements()) {
      let win = enumerator.getNext();
      if (!win.closed && isWindowGoodForChats(win))
        topMost = win;
    }
    return topMost;
  },
}
