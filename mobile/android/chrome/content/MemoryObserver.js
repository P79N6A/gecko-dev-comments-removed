


"use strict";

var MemoryObserver = {
  observe: function mo_observe(aSubject, aTopic, aData) {
    if (aTopic == "memory-pressure") {
      if (aData != "heap-minimize") {
        this.handleLowMemory();
      }
      
      
      
      this.gc();
    } else if (aTopic == "Memory:Dump") {
      this.dumpMemoryStats(aData);
    }
  },

  handleLowMemory: function() {
    
    let tabs = BrowserApp.tabs;
    let selected = BrowserApp.selectedTab;
    for (let i = 0; i < tabs.length; i++) {
      if (tabs[i] != selected) {
        this.zombify(tabs[i]);
      }
    }

    
    let defaults = Services.prefs.getDefaultBranch(null);

    
    defaults.setIntPref("image.mem.max_decoded_image_kb", 0);
  },

  zombify: function(tab) {
    let browser = tab.browser;
    let data = browser.__SS_data;
    let extra = browser.__SS_extdata;

    
    
    let currentURL = browser.__SS_restore ? data.entries[0].url : browser.currentURI.spec;
    let sibling = browser.nextSibling;
    let isPrivate = PrivateBrowsingUtils.isBrowserPrivate(browser);

    tab.destroy();
    tab.create(currentURL, { sibling: sibling, zombifying: true, delayLoad: true, isPrivate: isPrivate });

    
    browser = tab.browser;
    browser.__SS_data = data;
    browser.__SS_extdata = extra;
    browser.__SS_restore = true;
    browser.setAttribute("pending", "true");
  },

  gc: function() {
    window.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils).garbageCollect();
    Cu.forceGC();
  },

  dumpMemoryStats: function(aLabel) {
    let memDumper = Cc["@mozilla.org/memory-info-dumper;1"].getService(Ci.nsIMemoryInfoDumper);
    memDumper.dumpMemoryInfoToTempDir(aLabel,  false,
                                       false);
  },
};
