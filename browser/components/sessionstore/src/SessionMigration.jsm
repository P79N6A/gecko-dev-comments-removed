



"use strict";

this.EXPORTED_SYMBOLS = ["SessionMigration"];

const Cu = Components.utils;
Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);
Cu.import("resource://gre/modules/Task.jsm", this);
Cu.import("resource://gre/modules/osfile.jsm", this);


XPCOMUtils.defineLazyGetter(this, "gEncoder", function () {
  return new TextEncoder();
});


XPCOMUtils.defineLazyGetter(this, "gDecoder", function () {
  return new TextDecoder();
});

let SessionMigrationInternal = {
  














  convertState: function(aStateObj) {
    let state = {
      selectedWindow: aStateObj.selectedWindow,
      _closedWindows: []
    };
    state.windows = aStateObj.windows.map(function(oldWin) {
      var win = {extData: {}};
      win.tabs = oldWin.tabs.map(function(oldTab) {
        var tab = {};
        
        tab.entries = oldTab.entries.map(function(entry) {
          return {url: entry.url, title: entry.title};
        });
        tab.index = oldTab.index;
        tab.hidden = oldTab.hidden;
        tab.pinned = oldTab.pinned;
        
        if (oldTab.extData && "tabview-tab" in oldTab.extData) {
          tab.extData = {"tabview-tab": oldTab.extData["tabview-tab"]};
        }
        return tab;
      });
      
      
      if (oldWin.extData) {
        for (let k of Object.keys(oldWin.extData)) {
          if (k.startsWith("tabview-")) {
            win.extData[k] = oldWin.extData[k];
          }
        }
      }
      win.selected = oldWin.selected;
      win._closedTabs = [];
      return win;
    });
    let wrappedState = {
      url: "about:welcomeback",
      formdata: {
        id: {"sessionData": state},
        xpath: {}
      }
    };
    return {windows: [{tabs: [{entries: [wrappedState]}]}]};
  },
  


  readState: function(aPath) {
    return Task.spawn(function() {
      let bytes = yield OS.File.read(aPath);
      let text = gDecoder.decode(bytes);
      let state = JSON.parse(text);
      throw new Task.Result(state);
    });
  },
  


  writeState: function(aPath, aState) {
    let bytes = gEncoder.encode(JSON.stringify(aState));
    return OS.File.writeAtomic(aPath, bytes, {tmpPath: aPath + ".tmp"});
  }
}

let SessionMigration = {
  


  migrate: function(aFromPath, aToPath) {
    return Task.spawn(function() {
      let inState = yield SessionMigrationInternal.readState(aFromPath);
      let outState = SessionMigrationInternal.convertState(inState);
      
      
      
      
      yield SessionMigrationInternal.writeState(aToPath, outState);
    });
  }
};
