



const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://webapprt/modules/WebappRT.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Task.jsm");

XPCOMUtils.defineLazyGetter(this, "gAppBrowser",
                            function() document.getElementById("content"));

#ifdef MOZ_CRASHREPORTER
XPCOMUtils.defineLazyServiceGetter(this, "gCrashReporter",
                                   "@mozilla.org/toolkit/crash-reporter;1",
                                   "nsICrashReporter");
#endif

function isSameOrigin(url) {
  let origin = Services.io.newURI(url, null, null).prePath;
  return (origin == WebappRT.config.app.origin);
}

let progressListener = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIWebProgressListener,
                                         Ci.nsISupportsWeakReference]),
  onLocationChange: function onLocationChange(progress, request, location,
                                              flags) {

    
    let pageTooltip = document.getElementById("contentAreaTooltip");
    let tooltipNode = pageTooltip.triggerNode;
    if (tooltipNode) {
      
      if (progress.isTopLevel) {
        pageTooltip.hidePopup();
      }
      else {
        for (let tooltipWindow = tooltipNode.ownerDocument.defaultView;
             tooltipWindow != tooltipWindow.parent;
             tooltipWindow = tooltipWindow.parent) {
          if (tooltipWindow == progress.DOMWindow) {
            pageTooltip.hidePopup();
            break;
          }
        }
      }
    }

    
    
    
    
    let title = WebappRT.localeManifest.name;
    if (!isSameOrigin(location.spec)) {
      title = location.prePath + " - " + title;
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

function onOpenWindow(event) {
  let name = event.detail.name;

  if (name == "_blank") {
    let uri = Services.io.newURI(event.detail.url, null, null);

    
    Cc["@mozilla.org/uriloader/external-protocol-service;1"].
    getService(Ci.nsIExternalProtocolService).
    getProtocolHandlerInfo(uri.scheme).
    launchWithURI(uri);
  } else {
    let win = window.openDialog("chrome://webapprt/content/webapp.xul",
                                name,
                                "chrome,dialog=no,resizable," + event.detail.features);

    win.addEventListener("load", function onLoad() {
      win.removeEventListener("load", onLoad, false);

#ifndef XP_WIN
#ifndef XP_MACOSX
      if (isSameOrigin(event.detail.url)) {
        
        
        
        if (document.mozFullScreenElement) {
          win.document.getElementById("main-menubar").style.display = "none";
        }
      }
#endif
#endif

      win.document.getElementById("content").docShell.setIsApp(WebappRT.appID);
      win.document.getElementById("content").setAttribute("src", event.detail.url);
    }, false);
  }
}

function onLoad() {
  window.removeEventListener("load", onLoad, false);

  gAppBrowser.addProgressListener(progressListener,
                                  Ci.nsIWebProgress.NOTIFY_LOCATION |
                                  Ci.nsIWebProgress.NOTIFY_STATE_DOCUMENT);

  updateMenuItems();

  gAppBrowser.addEventListener("mozbrowseropenwindow", onOpenWindow);
}
window.addEventListener("load", onLoad, false);

function onUnload() {
  gAppBrowser.removeProgressListener(progressListener);
  gAppBrowser.removeEventListener("mozbrowseropenwindow", onOpenWindow);
}
window.addEventListener("unload", onUnload, false);



#ifndef XP_MACOSX
document.addEventListener('mozfullscreenchange', function() {
  if (document.mozFullScreenElement) {
    document.getElementById("main-menubar").style.display = "none";
  } else {
    document.getElementById("main-menubar").style.display = "";
  }
}, false);
#endif



let updateMenuItems = Task.async(function*() {
#ifdef XP_MACOSX
  yield WebappRT.configPromise;

  let manifest = WebappRT.localeManifest;
  let bundle =
    Services.strings.createBundle("chrome://webapprt/locale/webapp.properties");
  let quitLabel = bundle.formatStringFromName("quitApplicationCmdMac.label",
                                              [manifest.name], 1);
  let hideLabel = bundle.formatStringFromName("hideApplicationCmdMac.label",
                                              [manifest.name], 1);
  document.getElementById("menu_FileQuitItem").setAttribute("label", quitLabel);
  document.getElementById("menu_mac_hide_app").setAttribute("label", hideLabel);
#endif
});

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
