




"use strict";

let Cc = Components.classes;
let Ci = Components.interfaces;
let Cu = Components.utils;

this.EXPORTED_SYMBOLS = [ "ContentClick" ];

Cu.import("resource:///modules/PlacesUIUtils.jsm");
Cu.import("resource://gre/modules/PrivateBrowsingUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

let ContentClick = {
  init: function() {
    let mm = Cc["@mozilla.org/globalmessagemanager;1"].getService(Ci.nsIMessageListenerManager);
    mm.addMessageListener("Content:Click", this);
  },

  receiveMessage: function (message) {
    switch (message.name) {
      case "Content:Click":
        this.contentAreaClick(message.json, message.target)
        break;
    }
  },

  contentAreaClick: function (json, browser) {
    
    
    let window = browser.ownerDocument.defaultView;

    if (!json.href) {
      
      if (Services.prefs.getBoolPref("middlemouse.contentLoadURL") &&
          !Services.prefs.getBoolPref("general.autoScroll")) {
        window.middleMousePaste(json);
      }
      return;
    }

    if (json.bookmark) {
      
      
      
      PlacesUIUtils.showBookmarkDialog({ action: "add"
                                       , type: "bookmark"
                                       , uri: Services.io.newURI(json.href, null, null)
                                       , title: json.title
                                       , loadBookmarkInSidebar: true
                                       , hiddenRows: [ "description"
                                                     , "location"
                                                     , "keyword" ]
                                       }, window);
      return;
    }

    

    
    var where = window.whereToOpenLink(json);
    if (where == "current")
      return false;

    

    let params = { charset: browser.characterSet,
                   referrerURI: browser.documentURI,
                   noReferrer: json.noReferrer };
    window.openLinkIn(json.href, where, params);

    
    
    
    
    try {
      if (!PrivateBrowsingUtils.isWindowPrivate(window))
        PlacesUIUtils.markPageAsFollowedLink(href);
    } catch (ex) {  }
  }
};
