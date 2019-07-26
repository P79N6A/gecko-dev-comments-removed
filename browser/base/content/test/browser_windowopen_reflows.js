


"use strict";

const EXPECTED_REFLOWS = [
  
  "handleEvent@chrome://browser/content/tabbrowser.xml|",

  
  "loadTabs@chrome://browser/content/tabbrowser.xml|" +
    "loadOneOrMoreURIs@chrome://browser/content/browser.js|" +
    "gBrowserInit._delayedStartup@chrome://browser/content/browser.js|",

  
  "select@chrome://global/content/bindings/textbox.xml|" +
    "focusAndSelectUrlBar@chrome://browser/content/browser.js|" +
    "gBrowserInit._delayedStartup@chrome://browser/content/browser.js|",

  
  "gBrowserInit._delayedStartup@chrome://browser/content/browser.js|",
];

if (Services.appinfo.OS == "Darwin") {
  
  
  
  EXPECTED_REFLOWS.push("rect@chrome://browser/content/browser.js|" +
                          "TabsInTitlebar._update@chrome://browser/content/browser.js|" +
                          "updateAppearance@chrome://browser/content/browser.js|" +
                          "handleEvent@chrome://browser/content/tabbrowser.xml|");

  
  EXPECTED_REFLOWS.push("OverflowableToolbar.prototype._onOverflow@resource:///modules/CustomizableUI.jsm|" +
                        "OverflowableToolbar.prototype.init@resource:///modules/CustomizableUI.jsm|" +
                        "OverflowableToolbar.prototype.observe@resource:///modules/CustomizableUI.jsm|" +
                        "gBrowserInit._delayedStartup@chrome://browser/content/browser.js|");
  
  EXPECTED_REFLOWS.push("@resource://app/modules/CustomizableUI.jsm|" +
                          "@resource://app/modules/CustomizableUI.jsm|" +
                          "@resource://app/modules/CustomizableUI.jsm|" +
                          "gBrowserInit._delayedStartup@chrome://browser/content/browser.js|");
}





function test() {
  waitForExplicitFinish();

  
  let win = OpenBrowserWindow();
  let docShell = win.QueryInterface(Ci.nsIInterfaceRequestor)
                    .getInterface(Ci.nsIWebNavigation)
                    .QueryInterface(Ci.nsIDocShell);
  docShell.addWeakReflowObserver(observer);

  
  waitForMozAfterPaint(win, function paintListener() {
    
    docShell.removeWeakReflowObserver(observer);
    win.close();

    finish();
  });
}

let observer = {
  reflow: function (start, end) {
    
    let stack = new Error().stack;
    let path = stack.split("\n").slice(1).map(line => {
      return line.replace(/:\d+$/, "");
    }).join("|");
    let pathWithLineNumbers = (new Error().stack).split("\n").slice(1).join("|");

    
    if (path === "") {
      return;
    }

    
    for (let expectedStack of EXPECTED_REFLOWS) {
      if (path.startsWith(expectedStack) ||
          
          path.startsWith(expectedStack.replace(/(^|\|)(gBrowserInit\._delayedStartup|TabsInTitlebar\._update)@/, "$1@"))) {
        ok(true, "expected uninterruptible reflow '" + expectedStack + "'");
        return;
      }
    }

    ok(false, "unexpected uninterruptible reflow '" + pathWithLineNumbers + "'");
  },

  reflowInterruptible: function (start, end) {
    
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIReflowObserver,
                                         Ci.nsISupportsWeakReference])
};

function waitForMozAfterPaint(win, callback) {
  win.addEventListener("MozAfterPaint", function onEnd(event) {
    if (event.target != win)
      return;
    win.removeEventListener("MozAfterPaint", onEnd);
    executeSoon(callback);
  });
}
