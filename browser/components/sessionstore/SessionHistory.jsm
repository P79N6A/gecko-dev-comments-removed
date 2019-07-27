



"use strict";

this.EXPORTED_SYMBOLS = ["SessionHistory"];

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Utils",
  "resource:///modules/sessionstore/Utils.jsm");

function debug(msg) {
  Services.console.logStringMessage("SessionHistory: " + msg);
}




this.SessionHistory = Object.freeze({
  isEmpty: function (docShell) {
    return SessionHistoryInternal.isEmpty(docShell);
  },

  collect: function (docShell) {
    return SessionHistoryInternal.collect(docShell);
  },

  restore: function (docShell, tabData) {
    SessionHistoryInternal.restore(docShell, tabData);
  }
});




let SessionHistoryInternal = {
  





  isEmpty: function (docShell) {
    let webNavigation = docShell.QueryInterface(Ci.nsIWebNavigation);
    let history = webNavigation.sessionHistory;
    if (!webNavigation.currentURI) {
      return true;
    }
    let uri = webNavigation.currentURI.spec;
    return uri == "about:blank" && history.count == 0;
  },

  





  collect: function (docShell) {
    let data = {entries: []};
    let isPinned = docShell.isAppTab;
    let webNavigation = docShell.QueryInterface(Ci.nsIWebNavigation);
    let history = webNavigation.sessionHistory.QueryInterface(Ci.nsISHistoryInternal);

    if (history && history.count > 0) {
      
      
      for (let txn = history.rootTransaction; txn; txn = txn.next) {
        let entry = this.serializeEntry(txn.sHEntry, isPinned);
        entry.persist = txn.persist;
        data.entries.push(entry);
      }

      
      data.index = Math.min(history.index + 1, data.entries.length);
    }

    
    
    if (data.entries.length == 0) {
      let uri = webNavigation.currentURI.spec;
      let body = webNavigation.document.body;
      
      
      
      
      
      
      if (uri != "about:blank" || (body && body.hasChildNodes())) {
        data.entries.push({ url: uri });
        data.index = 1;
      }
    }

    return data;
  },

  






  isDynamic: function (shEntry) {
    
    
    
    
    return shEntry.parent && shEntry.isDynamicallyAdded();
  },

  








  serializeEntry: function (shEntry, isPinned) {
    let entry = { url: shEntry.URI.spec };

    
    
    if (shEntry.title && shEntry.title != entry.url) {
      entry.title = shEntry.title;
    }
    if (shEntry.isSubFrame) {
      entry.subframe = true;
    }

    let cacheKey = shEntry.cacheKey;
    if (cacheKey && cacheKey instanceof Ci.nsISupportsPRUint32 &&
        cacheKey.data != 0) {
      
      
      entry.cacheKey = cacheKey.data;
    }
    entry.ID = shEntry.ID;
    entry.docshellID = shEntry.docshellID;

    
    
    if (shEntry.referrerURI) {
      entry.referrer = shEntry.referrerURI.spec;
      entry.referrerPolicy = shEntry.referrerPolicy;
    }

    if (shEntry.srcdocData)
      entry.srcdocData = shEntry.srcdocData;

    if (shEntry.isSrcdocEntry)
      entry.isSrcdocEntry = shEntry.isSrcdocEntry;

    if (shEntry.baseURI)
      entry.baseURI = shEntry.baseURI.spec;

    if (shEntry.contentType)
      entry.contentType = shEntry.contentType;

    let x = {}, y = {};
    shEntry.getScrollPosition(x, y);
    if (x.value != 0 || y.value != 0)
      entry.scroll = x.value + "," + y.value;

    
    try {
      let owner = this.serializeOwner(shEntry);
      if (owner) {
        entry.owner_b64 = owner;
      }
    } catch (ex) {
      
      
      debug("Failed serializing owner data: " + ex);
    }

    entry.docIdentifier = shEntry.BFCacheEntry.ID;

    if (shEntry.stateData != null) {
      entry.structuredCloneState = shEntry.stateData.getDataAsBase64();
      entry.structuredCloneVersion = shEntry.stateData.formatVersion;
    }

    if (!(shEntry instanceof Ci.nsISHContainer)) {
      return entry;
    }

    if (shEntry.childCount > 0) {
      let children = [];
      for (let i = 0; i < shEntry.childCount; i++) {
        let child = shEntry.GetChildAt(i);

        if (child && !this.isDynamic(child)) {
          
          
          if (child.URI.schemeIs("wyciwyg")) {
            children.length = 0;
            break;
          }

          children.push(this.serializeEntry(child, isPinned));
        }
      }

      if (children.length) {
        entry.children = children;
      }
    }

    return entry;
  },

  






  serializeOwner: function (shEntry) {
    if (!shEntry.owner) {
      return null;
    }

    let binaryStream = Cc["@mozilla.org/binaryoutputstream;1"].
                       createInstance(Ci.nsIObjectOutputStream);
    let pipe = Cc["@mozilla.org/pipe;1"].createInstance(Ci.nsIPipe);
    pipe.init(false, false, 0, 0xffffffff, null);
    binaryStream.setOutputStream(pipe.outputStream);
    binaryStream.writeCompoundObject(shEntry.owner, Ci.nsISupports, true);
    binaryStream.close();

    
    let scriptableStream = Cc["@mozilla.org/binaryinputstream;1"].
                           createInstance(Ci.nsIBinaryInputStream);
    scriptableStream.setInputStream(pipe.inputStream);
    let ownerBytes =
      scriptableStream.readByteArray(scriptableStream.available());

    
    
    
    return btoa(String.fromCharCode.apply(null, ownerBytes));
  },

  







  restore: function (docShell, tabData) {
    let webNavigation = docShell.QueryInterface(Ci.nsIWebNavigation);
    let history = webNavigation.sessionHistory;

    if (history.count > 0) {
      history.PurgeHistory(history.count);
    }
    history.QueryInterface(Ci.nsISHistoryInternal);

    let idMap = { used: {} };
    let docIdentMap = {};
    for (let i = 0; i < tabData.entries.length; i++) {
      let entry = tabData.entries[i];
      
      if (!entry.url)
        continue;
      let persist = "persist" in entry ? entry.persist : true;
      history.addEntry(this.deserializeEntry(entry, idMap, docIdentMap), persist);
    }
  },

  










  deserializeEntry: function (entry, idMap, docIdentMap) {

    var shEntry = Cc["@mozilla.org/browser/session-history-entry;1"].
                  createInstance(Ci.nsISHEntry);

    shEntry.setURI(Utils.makeURI(entry.url));
    shEntry.setTitle(entry.title || entry.url);
    if (entry.subframe)
      shEntry.setIsSubFrame(entry.subframe || false);
    shEntry.loadType = Ci.nsIDocShellLoadInfo.loadHistory;
    if (entry.contentType)
      shEntry.contentType = entry.contentType;
    if (entry.referrer) {
      shEntry.referrerURI = Utils.makeURI(entry.referrer);
      shEntry.referrerPolicy = entry.referrerPolicy;
    }
    if (entry.isSrcdocEntry)
      shEntry.srcdocData = entry.srcdocData;
    if (entry.baseURI)
      shEntry.baseURI = Utils.makeURI(entry.baseURI);

    if (entry.cacheKey) {
      var cacheKey = Cc["@mozilla.org/supports-PRUint32;1"].
                     createInstance(Ci.nsISupportsPRUint32);
      cacheKey.data = entry.cacheKey;
      shEntry.cacheKey = cacheKey;
    }

    if (entry.ID) {
      
      
      var id = idMap[entry.ID] || 0;
      if (!id) {
        for (id = Date.now(); id in idMap.used; id++);
        idMap[entry.ID] = id;
        idMap.used[id] = true;
      }
      shEntry.ID = id;
    }

    if (entry.docshellID)
      shEntry.docshellID = entry.docshellID;

    if (entry.structuredCloneState && entry.structuredCloneVersion) {
      shEntry.stateData =
        Cc["@mozilla.org/docshell/structured-clone-container;1"].
        createInstance(Ci.nsIStructuredCloneContainer);

      shEntry.stateData.initFromBase64(entry.structuredCloneState,
                                       entry.structuredCloneVersion);
    }

    if (entry.scroll) {
      var scrollPos = (entry.scroll || "0,0").split(",");
      scrollPos = [parseInt(scrollPos[0]) || 0, parseInt(scrollPos[1]) || 0];
      shEntry.setScrollPosition(scrollPos[0], scrollPos[1]);
    }

    let childDocIdents = {};
    if (entry.docIdentifier) {
      
      
      
      
      let matchingEntry = docIdentMap[entry.docIdentifier];
      if (!matchingEntry) {
        matchingEntry = {shEntry: shEntry, childDocIdents: childDocIdents};
        docIdentMap[entry.docIdentifier] = matchingEntry;
      }
      else {
        shEntry.adoptBFCacheEntry(matchingEntry.shEntry);
        childDocIdents = matchingEntry.childDocIdents;
      }
    }

    if (entry.owner_b64) {
      var ownerInput = Cc["@mozilla.org/io/string-input-stream;1"].
                       createInstance(Ci.nsIStringInputStream);
      var binaryData = atob(entry.owner_b64);
      ownerInput.setData(binaryData, binaryData.length);
      var binaryStream = Cc["@mozilla.org/binaryinputstream;1"].
                         createInstance(Ci.nsIObjectInputStream);
      binaryStream.setInputStream(ownerInput);
      try { 
        shEntry.owner = binaryStream.readObject(true);
      } catch (ex) { debug(ex); }
    }

    if (entry.children && shEntry instanceof Ci.nsISHContainer) {
      for (var i = 0; i < entry.children.length; i++) {
        
        if (!entry.children[i].url)
          continue;

        
        
        
        
        
        
        
        
        
        
        
        
        
        

        shEntry.AddChild(this.deserializeEntry(entry.children[i], idMap,
                                               childDocIdents), i);
      }
    }

    return shEntry;
  },

};
