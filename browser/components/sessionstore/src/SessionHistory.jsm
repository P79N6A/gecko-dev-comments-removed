



"use strict";

this.EXPORTED_SYMBOLS = ["SessionHistory"];

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PrivacyLevel",
  "resource:///modules/sessionstore/PrivacyLevel.jsm");

function debug(msg) {
  Services.console.logStringMessage("SessionHistory: " + msg);
}


XPCOMUtils.defineLazyGetter(this, "gPostData", function () {
  const PREF = "browser.sessionstore.postdata";

  
  Services.prefs.addObserver(PREF, () => {
    this.gPostData = Services.prefs.getIntPref(PREF);
  }, false);

  return Services.prefs.getIntPref(PREF);
});




this.SessionHistory = Object.freeze({
  read: function (docShell, includePrivateData) {
    return SessionHistoryInternal.read(docShell, includePrivateData);
  }
});




let SessionHistoryInternal = {
  







  read: function (docShell, includePrivateData = false) {
    let data = {entries: []};
    let isPinned = docShell.isAppTab;
    let webNavigation = docShell.QueryInterface(Ci.nsIWebNavigation);
    let history = webNavigation.sessionHistory;

    if (history && history.count > 0) {
      try {
        for (let i = 0; i < history.count; i++) {
          let shEntry = history.getEntryAtIndex(i, false);
          let entry = this._serializeEntry(shEntry, includePrivateData, isPinned);
          data.entries.push(entry);
        }
      } catch (ex) {
        
        
        
        
        debug("SessionStore failed gathering complete history " +
              "for the focused window/tab. See bug 669196.");
      }
      data.index = history.index + 1;
    } else {
      let uri = webNavigation.currentURI.spec;
      
      
      
      
      
      
      if (uri != "about:blank" || webNavigation.document.body.hasChildNodes()) {
        data.entries.push({ url: uri });
        data.index = 1;
      }
    }

    return data;
  },

  










  _serializeEntry: function (shEntry, includePrivateData, isPinned) {
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

    
    
    if (shEntry.referrerURI)
      entry.referrer = shEntry.referrerURI.spec;

    if (shEntry.srcdocData)
      entry.srcdocData = shEntry.srcdocData;

    if (shEntry.isSrcdocEntry)
      entry.isSrcdocEntry = shEntry.isSrcdocEntry;

    if (shEntry.contentType)
      entry.contentType = shEntry.contentType;

    let x = {}, y = {};
    shEntry.getScrollPosition(x, y);
    if (x.value != 0 || y.value != 0)
      entry.scroll = x.value + "," + y.value;

    
    try {
      let postdata = this._serializePostData(shEntry, isPinned);
      if (postdata) {
        entry.postdata_b64 = postdata;
      }
    } catch (ex) {
      
      debug("Failed serializing post data: " + ex);
    }

    
    try {
      let owner = this._serializeOwner(shEntry);
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

        if (child) {
          
          
          if (child.URI.schemeIs("wyciwyg")) {
            children.length = 0;
            break;
          }

          children.push(this._serializeEntry(child, includePrivateData, isPinned));
        }
      }

      if (children.length) {
        entry.children = children;
      }
    }

    return entry;
  },

  








  _serializePostData: function (shEntry, isPinned) {
    let isHttps = shEntry.URI.schemeIs("https");
    if (!shEntry.postData || !gPostData ||
        !PrivacyLevel.canSave({isHttps: isHttps, isPinned: isPinned})) {
      return null;
    }

    shEntry.postData.QueryInterface(Ci.nsISeekableStream)
                    .seek(Ci.nsISeekableStream.NS_SEEK_SET, 0);
    let stream = Cc["@mozilla.org/binaryinputstream;1"]
                   .createInstance(Ci.nsIBinaryInputStream);
    stream.setInputStream(shEntry.postData);
    let postBytes = stream.readByteArray(stream.available());
    let postdata = String.fromCharCode.apply(null, postBytes);
    if (gPostData != -1 &&
        postdata.replace(/^(Content-.*\r\n)+(\r\n)*/, "").length > gPostData) {
      return null;
    }

    
    
    
    return btoa(postdata);
  },

  






  _serializeOwner: function (shEntry) {
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
  }
};
