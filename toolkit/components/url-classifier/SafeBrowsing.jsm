



this.EXPORTED_SYMBOLS = ["SafeBrowsing"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");



function getLists(prefName) {
  let pref = Services.prefs.getCharPref(prefName);
  
  if (!pref) {
    return [];
  }
  return pref.split(",")
    .filter(function(value) { return value.indexOf("test-") == -1; })
    .map(function(value) { return value.trim(); });
}


const phishingLists = getLists("urlclassifier.phish_table");
const malwareLists = getLists("urlclassifier.malware_table");
const downloadBlockLists = getLists("urlclassifier.downloadBlockTable");
const downloadAllowLists = getLists("urlclassifier.downloadAllowTable");

var debug = false;
function log(...stuff) {
  if (!debug)
    return;

  let msg = "SafeBrowsing: " + stuff.join(" ");
  Services.console.logStringMessage(msg);
  dump(msg + "\n");
}

this.SafeBrowsing = {

  init: function() {
    if (this.initialized) {
      log("Already initialized");
      return;
    }

    Services.prefs.addObserver("browser.safebrowsing", this.readPrefs.bind(this), false);
    this.readPrefs();

    
    let listManager = Cc["@mozilla.org/url-classifier/listmanager;1"].
                      getService(Ci.nsIUrlListManager);
    for (let i = 0; i < phishingLists.length; ++i) {
      listManager.registerTable(phishingLists[i], this.updateURL, this.gethashURL);
    }
    for (let i = 0; i < malwareLists.length; ++i) {
      listManager.registerTable(malwareLists[i], this.updateURL, this.gethashURL);
    }
    for (let i = 0; i < downloadBlockLists.length; ++i) {
      listManager.registerTable(downloadBlockLists[i], this.updateURL, this.gethashURL);
    }
    for (let i = 0; i < downloadAllowLists.length; ++i) {
      listManager.registerTable(downloadAllowLists[i], this.updateURL, this.gethashURL);
    }
    this.addMozEntries();

    this.controlUpdateChecking();
    this.initialized = true;

    log("init() finished");
  },


  initialized:     false,
  phishingEnabled: false,
  malwareEnabled:  false,

  updateURL:             null,
  gethashURL:            null,

  reportURL:             null,
  reportGenericURL:      null,
  reportErrorURL:        null,
  reportPhishURL:        null,
  reportMalwareURL:      null,
  reportMalwareErrorURL: null,


  getReportURL: function(kind) {
    return this["report"  + kind + "URL"];
  },


  readPrefs: function() {
    log("reading prefs");

    debug = Services.prefs.getBoolPref("browser.safebrowsing.debug");
    this.phishingEnabled = Services.prefs.getBoolPref("browser.safebrowsing.enabled");
    this.malwareEnabled  = Services.prefs.getBoolPref("browser.safebrowsing.malware.enabled");
    this.updateProviderURLs();

    
    
    
    if (this.initialized)
      this.controlUpdateChecking();
  },


  updateProviderURLs: function() {
    try {
      var clientID = Services.prefs.getCharPref("browser.safebrowsing.id");
    } catch(e) {
      var clientID = Services.appinfo.name;
    }

    log("initializing safe browsing URLs, client id ", clientID);
    let basePref = "browser.safebrowsing.";

    
    this.reportURL             = Services.urlFormatter.formatURLPref(basePref + "reportURL");
    this.reportGenericURL      = Services.urlFormatter.formatURLPref(basePref + "reportGenericURL");
    this.reportErrorURL        = Services.urlFormatter.formatURLPref(basePref + "reportErrorURL");
    this.reportPhishURL        = Services.urlFormatter.formatURLPref(basePref + "reportPhishURL");
    this.reportMalwareURL      = Services.urlFormatter.formatURLPref(basePref + "reportMalwareURL");
    this.reportMalwareErrorURL = Services.urlFormatter.formatURLPref(basePref + "reportMalwareErrorURL");

    
    this.updateURL  = Services.urlFormatter.formatURLPref(basePref + "updateURL");
    this.gethashURL = Services.urlFormatter.formatURLPref(basePref + "gethashURL");

    this.updateURL  = this.updateURL.replace("SAFEBROWSING_ID", clientID);
    this.gethashURL = this.gethashURL.replace("SAFEBROWSING_ID", clientID);
  },

  controlUpdateChecking: function() {
    log("phishingEnabled:", this.phishingEnabled, "malwareEnabled:", this.malwareEnabled);

    let listManager = Cc["@mozilla.org/url-classifier/listmanager;1"].
                      getService(Ci.nsIUrlListManager);

    for (let i = 0; i < phishingLists.length; ++i) {
      if (this.phishingEnabled) {
        listManager.enableUpdate(phishingLists[i]);
      } else {
        listManager.disableUpdate(phishingLists[i]);
      }
    }
    for (let i = 0; i < malwareLists.length; ++i) {
      if (this.malwareEnabled) {
        listManager.enableUpdate(malwareLists[i]);
      } else {
        listManager.disableUpdate(malwareLists[i]);
      }
    }
    for (let i = 0; i < downloadBlockLists.length; ++i) {
      if (this.malwareEnabled) {
        listManager.enableUpdate(downloadBlockLists[i]);
      } else {
        listManager.disableUpdate(downloadBlockLists[i]);
      }
    }
    for (let i = 0; i < downloadAllowLists.length; ++i) {
      if (this.malwareEnabled) {
        listManager.enableUpdate(downloadAllowLists[i]);
      } else {
        listManager.disableUpdate(downloadAllowLists[i]);
      }
    }
  },


  addMozEntries: function() {
    
    
    const phishURL   = "itisatrap.org/firefox/its-a-trap.html";
    const malwareURL = "itisatrap.org/firefox/its-an-attack.html";

    let update = "n:1000\ni:test-malware-simple\nad:1\n" +
                 "a:1:32:" + malwareURL.length + "\n" +
                 malwareURL;
    update += "n:1000\ni:test-phish-simple\nad:1\n" +
              "a:1:32:" + phishURL.length + "\n" +
              phishURL;
    log("addMozEntries:", update);

    let db = Cc["@mozilla.org/url-classifier/dbservice;1"].
             getService(Ci.nsIUrlClassifierDBService);

    
    let dummyListener = {
      updateUrlRequested: function() { },
      streamFinished:     function() { },
      updateError:        function() { },
      updateSuccess:      function() { }
    };

    try {
      db.beginUpdate(dummyListener, "test-malware-simple,test-phish-simple", "");
      db.beginStream("", "");
      db.updateStream(update);
      db.finishStream();
      db.finishUpdate();
    } catch(ex) {
      
      log("addMozEntries failed!", ex);
    }
  },
};
