



"use strict";

let Cc = Components.classes;
let Ci = Components.interfaces;
let Cu = Components.utils;

this.EXPORTED_SYMBOLS = [ "AboutHomeUtils", "AboutHome" ];

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PrivateBrowsingUtils",
  "resource://gre/modules/PrivateBrowsingUtils.jsm");


const SNIPPETS_URL_PREF = "browser.aboutHomeSnippets.updateUrl";


const STARTPAGE_VERSION = 4;

this.AboutHomeUtils = {
  get snippetsVersion() STARTPAGE_VERSION,

  



  get defaultSearchEngine() {
    let defaultEngine = Services.search.defaultEngine;
    let submission = defaultEngine.getSubmission("_searchTerms_", null, "homepage");

    return Object.freeze({
      name: defaultEngine.name,
      searchURL: submission.uri.spec,
      postDataString: submission.postDataString
    });
  },

  







  get showKnowYourRights() {
    
    
    try {
      return !Services.prefs.getBoolPref("browser.rights.override");
    } catch (e) { }
    
    try {
      return !Services.prefs.getBoolPref("browser.EULA.override");
    } catch (e) { }

#ifndef MOZILLA_OFFICIAL
    
    return false;
#endif

    
    var currentVersion = Services.prefs.getIntPref("browser.rights.version");
    try {
      return !Services.prefs.getBoolPref("browser.rights." + currentVersion + ".shown");
    } catch (e) { }

    
    
    try {
      return !Services.prefs.getBoolPref("browser.EULA." + currentVersion + ".accepted");
    } catch (e) { }

    
    return true;
  }
};




XPCOMUtils.defineLazyGetter(AboutHomeUtils, "snippetsURL", function() {
  let updateURL = Services.prefs
                          .getCharPref(SNIPPETS_URL_PREF)
                          .replace("%STARTPAGE_VERSION%", STARTPAGE_VERSION);
  return Services.urlFormatter.formatURL(updateURL);
});






let AboutHome = {
  MESSAGES: [
    "AboutHome:RestorePreviousSession",
    "AboutHome:Downloads",
    "AboutHome:Bookmarks",
    "AboutHome:History",
    "AboutHome:Apps",
    "AboutHome:Addons",
    "AboutHome:Sync",
    "AboutHome:Settings",
    "AboutHome:RequestUpdate",
    "AboutHome:Search",
  ],

  init: function() {
    let mm = Cc["@mozilla.org/globalmessagemanager;1"].getService(Ci.nsIMessageListenerManager);

    for (let msg of this.MESSAGES) {
      mm.addMessageListener(msg, this);
    }

    Services.obs.addObserver(this, "browser-search-engine-modified", false);
  },

  observe: function(aEngine, aTopic, aVerb) {
    switch (aTopic) {
      case "browser-search-engine-modified":
        this.sendAboutHomeData(null);
        break;
    }
  },

  receiveMessage: function(aMessage) {
    let window = aMessage.target.ownerDocument.defaultView;

    switch (aMessage.name) {
      case "AboutHome:RestorePreviousSession":
        let ss = Cc["@mozilla.org/browser/sessionstore;1"].
                 getService(Ci.nsISessionStore);
        if (ss.canRestoreLastSession) {
          ss.restoreLastSession();
        }
        break;

      case "AboutHome:Downloads":
        window.BrowserDownloadsUI();
        break;

      case "AboutHome:Bookmarks":
        window.PlacesCommandHook.showPlacesOrganizer("AllBookmarks");
        break;

      case "AboutHome:History":
        window.PlacesCommandHook.showPlacesOrganizer("History");
        break;

      case "AboutHome:Apps":
        window.openUILinkIn("https://marketplace.mozilla.org/", "tab");
        break;

      case "AboutHome:Addons":
        window.BrowserOpenAddonsMgr();
        break;

      case "AboutHome:Sync":
        window.openPreferences("paneSync");
        break;

      case "AboutHome:Settings":
        window.openPreferences();
        break;

      case "AboutHome:RequestUpdate":
        this.sendAboutHomeData(aMessage.target);
        break;

      case "AboutHome:Search":
#ifdef MOZ_SERVICES_HEALTHREPORT
        window.BrowserSearch.recordSearchInHealthReport(aMessage.data.engineName, "abouthome");
#endif
        break;
    }
  },

  
  
  sendAboutHomeData: function(target) {
    let ss = Cc["@mozilla.org/browser/sessionstore;1"].
               getService(Ci.nsISessionStore);
    let data = {
      showRestoreLastSession: ss.canRestoreLastSession,
      snippetsURL: AboutHomeUtils.snippetsURL,
      showKnowYourRights: AboutHomeUtils.showKnowYourRights,
      snippetsVersion: AboutHomeUtils.snippetsVersion,
      defaultSearchEngine: AboutHomeUtils.defaultSearchEngine
    };

    if (AboutHomeUtils.showKnowYourRights) {
      
      let currentVersion = Services.prefs.getIntPref("browser.rights.version");
      Services.prefs.setBoolPref("browser.rights." + currentVersion + ".shown", true);
    }

    if (target) {
      target.messageManager.sendAsyncMessage("AboutHome:Update", data);
    } else {
      let mm = Cc["@mozilla.org/globalmessagemanager;1"].getService(Ci.nsIMessageListenerManager);
      mm.broadcastAsyncMessage("AboutHome:Update", data);
    }
  },
};
