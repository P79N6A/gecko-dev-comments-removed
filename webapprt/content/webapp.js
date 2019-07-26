



const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://webapprt/modules/WebappRT.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyGetter(this, "gAppBrowser",
                            function() document.getElementById("content"));

#ifdef MOZ_CRASHREPORTER
XPCOMUtils.defineLazyServiceGetter(this, "gCrashReporter",
                                   "@mozilla.org/toolkit/crash-reporter;1",
                                   "nsICrashReporter");
#endif

let progressListener = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIWebProgressListener,
                                         Ci.nsISupportsWeakReference]),
  onLocationChange: function onLocationChange(progress, request, location,
                                              flags) {
    
    
    
    
    let title = WebappRT.config.app.manifest.name;
    let origin = location.prePath;
    if (origin != WebappRT.config.app.origin) {
      title = origin + " - " + title;

      
      
      document.mozCancelFullScreen();
    }
    document.documentElement.setAttribute("title", title);
  },

  onStateChange: function onStateChange(aProgress, aRequest, aFlags, aStatus) {
    if (aRequest instanceof Ci.nsIChannel &&
        aFlags & Ci.nsIWebProgressListener.STATE_START &&
        aFlags & Ci.nsIWebProgressListener.STATE_IS_DOCUMENT) {
      updateCrashReportURL(aRequest.URI);
    }
  }
};

function onLoad() {
  window.removeEventListener("load", onLoad, false);

  gAppBrowser.addProgressListener(progressListener,
                                  Ci.nsIWebProgress.NOTIFY_LOCATION |
                                  Ci.nsIWebProgress.NOTIFY_STATE_DOCUMENT);

  updateMenuItems();

  
  
  
  gAppBrowser.addEventListener("click", onContentClick, false, true);

  if (WebappRT.config.app.manifest.fullscreen) {
    enterFullScreen();
  }
}
window.addEventListener("load", onLoad, false);

function onUnload() {
  gAppBrowser.removeProgressListener(progressListener);
}
window.addEventListener("unload", onUnload, false);



function enterFullScreen() {
  
  
  
  gAppBrowser.mozRequestFullScreen();

  
  
  gAppBrowser.addEventListener("load", function onLoad() {
    gAppBrowser.removeEventListener("load", onLoad, true);
    gAppBrowser.contentDocument.
      documentElement.wrappedJSObject.mozRequestFullScreen();
  }, true);
}

#ifndef XP_MACOSX
document.addEventListener('mozfullscreenchange', function() {
  if (document.mozFullScreenElement) {
    document.getElementById("main-menubar").style.display = "none";
  } else {
    document.getElementById("main-menubar").style.display = "";
  }
}, false);
#endif









function onContentClick(event) {
  let target = event.target;

  if (!(target instanceof HTMLAnchorElement) ||
      target.getAttribute("target") != "_blank") {
    return;
  }

  let uri = Services.io.newURI(target.href,
                               target.ownerDocument.characterSet,
                               null);

  
  Cc["@mozilla.org/uriloader/external-protocol-service;1"].
    getService(Ci.nsIExternalProtocolService).
    getProtocolHandlerInfo(uri.scheme).
    launchWithURI(uri);

  
  
  
  event.preventDefault();
}



function updateMenuItems() {
#ifdef XP_MACOSX
  let installRecord = WebappRT.config.app;
  let manifest = WebappRT.config.app.manifest;
  let bundle =
    Services.strings.createBundle("chrome://webapprt/locale/webapp.properties");
  let quitLabel = bundle.formatStringFromName("quitApplicationCmdMac.label",
                                              [manifest.name], 1);
  let hideLabel = bundle.formatStringFromName("hideApplicationCmdMac.label",
                                              [manifest.name], 1);
  document.getElementById("menu_FileQuitItem").setAttribute("label", quitLabel);
  document.getElementById("menu_mac_hide_app").setAttribute("label", hideLabel);
#endif
}

#ifndef XP_MACOSX
let gEditUIVisible = true;
#endif

function updateEditUIVisibility() {
#ifndef XP_MACOSX
  let editMenuPopupState = document.getElementById("menu_EditPopup").state;
  let contextMenuPopupState = document.getElementById("contentAreaContextMenu").state;

  
  
  
  gEditUIVisible = editMenuPopupState == "showing" ||
                   editMenuPopupState == "open" ||
                   contextMenuPopupState == "showing" ||
                   contextMenuPopupState == "open";

  
  
  if (gEditUIVisible) {
    goUpdateGlobalEditMenuItems();
  }

  
  
  
  else {
    goSetCommandEnabled("cmd_undo", true);
    goSetCommandEnabled("cmd_redo", true);
    goSetCommandEnabled("cmd_cut", true);
    goSetCommandEnabled("cmd_copy", true);
    goSetCommandEnabled("cmd_paste", true);
    goSetCommandEnabled("cmd_selectAll", true);
    goSetCommandEnabled("cmd_delete", true);
    goSetCommandEnabled("cmd_switchTextDirection", true);
  }
#endif
}

function updateCrashReportURL(aURI) {
#ifdef MOZ_CRASHREPORTER
  if (!gCrashReporter.enabled)
    return;

  let uri = aURI.clone();
  
  
  try {
    if (uri.userPass != "") {
      uri.userPass = "";
    }
  } catch (e) {}

  gCrashReporter.annotateCrashReport("URL", uri.spec);
#endif
}





let gContextMenu = null;

XPCOMUtils.defineLazyGetter(this, "PageMenu", function() {
  let tmp = {};
  Cu.import("resource://gre/modules/PageMenu.jsm", tmp);
  return new tmp.PageMenu();
});

function showContextMenu(aEvent, aXULMenu) {
  if (aEvent.target != aXULMenu) {
    return true;
  }

  gContextMenu = new nsContextMenu(aXULMenu);
  if (gContextMenu.shouldDisplay) {
    updateEditUIVisibility();
  }

  return gContextMenu.shouldDisplay;
}

function hideContextMenu(aEvent, aXULMenu) {
  if (aEvent.target != aXULMenu) {
    return;
  }

  gContextMenu = null;

  updateEditUIVisibility();
}

function nsContextMenu(aXULMenu) {
  this.initMenu(aXULMenu);
}

nsContextMenu.prototype = {
  initMenu: function(aXULMenu) {
    this.hasPageMenu = PageMenu.maybeBuildAndAttachMenu(document.popupNode,
                                                        aXULMenu);
    this.shouldDisplay = this.hasPageMenu;
  },
};
