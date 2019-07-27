



"use strict";

let Cc = Components.classes;
let Ci = Components.interfaces;
let Cu = Components.utils;

this.EXPORTED_SYMBOLS = [ "TabCrashReporter" ];

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "CrashSubmit",
  "resource://gre/modules/CrashSubmit.jsm");

this.TabCrashReporter = {
  init: function () {
    if (this.initialized)
      return;
    this.initialized = true;

    Services.obs.addObserver(this, "ipc:content-shutdown", false);
    Services.obs.addObserver(this, "oop-frameloader-crashed", false);

    this.childMap = new Map();
    this.browserMap = new WeakMap();
  },

  observe: function (aSubject, aTopic, aData) {
    switch (aTopic) {
      case "ipc:content-shutdown":
        aSubject.QueryInterface(Ci.nsIPropertyBag2);

        if (!aSubject.get("abnormal"))
          return;

        this.childMap.set(aSubject.get("childID"), aSubject.get("dumpID"));
        break;

      case "oop-frameloader-crashed":
        aSubject.QueryInterface(Ci.nsIFrameLoader);

        let browser = aSubject.ownerElement;
        if (!browser)
          return;

        this.browserMap.set(browser, aSubject.childID);
        break;
    }
  },

  submitCrashReport: function (aBrowser) {
    let childID = this.browserMap.get(aBrowser);
    let dumpID = this.childMap.get(childID);
    if (!dumpID)
      return

    if (CrashSubmit.submit(dumpID, { recordSubmission: true })) {
      this.childMap.set(childID, null); 
      this.removeSubmitCheckboxesForSameCrash(childID);
    }
  },

  removeSubmitCheckboxesForSameCrash: function(childID) {
    let enumerator = Services.wm.getEnumerator("navigator:browser");
    while (enumerator.hasMoreElements()) {
      let window = enumerator.getNext();
      if (!window.gMultiProcessBrowser)
        continue;

      for (let browser of window.gBrowser.browsers) {
        if (browser.isRemoteBrowser)
          continue;

        let doc = browser.contentDocument;
        if (!doc.documentURI.startsWith("about:tabcrashed"))
          continue;

        if (this.browserMap.get(browser) == childID) {
          this.browserMap.delete(browser);
          browser.contentDocument.documentElement.classList.remove("crashDumpAvailable");
        }
      }
    }
  },

  reloadCrashedTab: function (browser) {
    if (browser.isRemoteBrowser)
      return;

    let doc = browser.contentDocument;
    if (!doc.documentURI.startsWith("about:tabcrashed"))
      return;

    let url = browser.currentURI.spec;
    browser.loadURIWithFlags(url, Ci.nsIWebNavigation.LOAD_FLAGS_NONE, null, null, null);
  },

  onAboutTabCrashedLoad: function (aBrowser) {
    if (!this.childMap)
      return;

    let dumpID = this.childMap.get(this.browserMap.get(aBrowser));
    if (!dumpID)
      return;

    aBrowser.contentDocument.documentElement.classList.add("crashDumpAvailable");
  }
}
