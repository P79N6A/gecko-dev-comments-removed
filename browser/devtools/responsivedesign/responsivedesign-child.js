



const Ci = Components.interfaces;
const gDeviceSizeWasPageSize = docShell.deviceSizeIsPageSize;
const gFloatingScrollbarsStylesheet = Services.io.newURI("chrome://browser/skin/devtools/floating-scrollbars.css", null, null);
let gRequiresFloatingScrollbars;

let active = false;

addMessageListener("ResponsiveMode:Start", startResponsiveMode);
addMessageListener("ResponsiveMode:Stop", stopResponsiveMode);

function startResponsiveMode({data:data}) {
  if (active) {
    return;
  }
  addMessageListener("ResponsiveMode:RequestScreenshot", screenshot);
  addMessageListener("ResponsiveMode:NotifyOnResize", notifiyOnResize);
  let webProgress = docShell.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIWebProgress);
  webProgress.addProgressListener(WebProgressListener, Ci.nsIWebProgress.NOTIFY_ALL);
  docShell.deviceSizeIsPageSize = true;
  gRequiresFloatingScrollbars = data.requiresFloatingScrollbars;

  
  
  if (docShell.contentViewer) {
    makeScrollbarsFloating();
  }
  active = true;
  sendAsyncMessage("ResponsiveMode:Start:Done");
}

function notifiyOnResize() {
  content.addEventListener("resize", () => {
    sendAsyncMessage("ResponsiveMode:OnContentResize");
  }, false);
  sendAsyncMessage("ResponsiveMode:NotifyOnResize:Done");
}

function stopResponsiveMode() {
  if (!active) {
    return;
  }
  active = false;
  removeMessageListener("ResponsiveMode:RequestScreenshot", screenshot);
  removeMessageListener("ResponsiveMode:NotifyOnResize", notifiyOnResize);
  let webProgress = docShell.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIWebProgress);
  webProgress.removeProgressListener(WebProgressListener);
  docShell.deviceSizeIsPageSize = gDeviceSizeWasPageSize;
  restoreScrollbars();
  sendAsyncMessage("ResponsiveMode:Stop:Done");
}

function makeScrollbarsFloating() {
  if (!gRequiresFloatingScrollbars) {
    return;
  }

  let allDocShells = [docShell];

  for (let i = 0; i < docShell.childCount; i++) {
    let child = docShell.getChildAt(i).QueryInterface(Ci.nsIDocShell);
    allDocShells.push(child);
  }

  for (let d of allDocShells) {
    let win = d.contentViewer.DOMDocument.defaultView;
    let winUtils = win.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils);
    try {
      winUtils.loadSheet(gFloatingScrollbarsStylesheet, win.AGENT_SHEET);
    } catch(e) { }
  }

  flushStyle();
}

function restoreScrollbars() {
  let allDocShells = [docShell];
  for (let i = 0; i < docShell.childCount; i++) {
    allDocShells.push(docShell.getChildAt(i).QueryInterface(Ci.nsIDocShell));
  }
  for (let d of allDocShells) {
    let win = d.contentViewer.DOMDocument.defaultView;
    let winUtils = win.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils);
    try {
      winUtils.removeSheet(gFloatingScrollbarsStylesheet, win.AGENT_SHEET);
    } catch(e) { }
  }
  flushStyle();
}

function flushStyle() {
  
  let isSticky = docShell.contentViewer.sticky;
  docShell.contentViewer.sticky = false;
  docShell.contentViewer.hide();
  docShell.contentViewer.show();
  docShell.contentViewer.sticky = isSticky;
}

function screenshot() {
  let canvas = content.document.createElementNS("http://www.w3.org/1999/xhtml", "canvas");
  let width = content.innerWidth;
  let height = content.innerHeight;
  canvas.mozOpaque = true;
  canvas.width = width;
  canvas.height = height;
  let ctx = canvas.getContext("2d");
  ctx.drawWindow(content, content.scrollX, content.scrollY, width, height, "#fff");
  sendAsyncMessage("ResponsiveMode:RequestScreenshot:Done", canvas.toDataURL());
}

let WebProgressListener = {
  onLocationChange: function onLocationChange(aWebProgress) {
    makeScrollbarsFloating();
  },
  QueryInterface: function QueryInterface(aIID) {
    if (aIID.equals(Ci.nsIWebProgressListener) ||
        aIID.equals(Ci.nsISupportsWeakReference) ||
        aIID.equals(Ci.nsISupports)) {
        return this;
    }
    throw Components.results.NS_ERROR_NO_INTERFACE;
  }
};

sendAsyncMessage("ResponsiveMode:ChildScriptReady");
