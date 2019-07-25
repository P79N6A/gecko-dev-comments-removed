



var EXPORTED_SYMBOLS = ["SafeBrowsing"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");

const phishingList = "goog-phish-shavar";
const malwareList  = "goog-malware-shavar";

var urlList = {
  updateURL:             "http://safebrowsing.clients.google.com/safebrowsing/downloads?client=%SAFEBROWSING_ID%&appver=%VERSION%&pver=2.2",
  keyURL:                "https://sb-ssl.google.com/safebrowsing/newkey?client=%SAFEBROWSING_ID%&appver=%VERSION%&pver=2.2",
  reportURL:             "http://safebrowsing.clients.google.com/safebrowsing/report?",
  gethashURL:            "http://safebrowsing.clients.google.com/safebrowsing/gethash?client=%SAFEBROWSING_ID%&appver=%VERSION%&pver=2.2",
  reportGenericURL:      "http://%LOCALE%.phish-generic.mozilla.com/?hl=%LOCALE%",
  reportErrorURL:        "http://%LOCALE%.phish-error.mozilla.com/?hl=%LOCALE%",
  reportPhishURL:        "http://%LOCALE%.phish-report.mozilla.com/?hl=%LOCALE%",
  reportMalwareURL:      "http://%LOCALE%.malware-report.mozilla.com/?hl=%LOCALE%",
  reportMalwareErrorURL: "http://%LOCALE%.malware-error.mozilla.com/?hl=%LOCALE%",
};

var debug = false;
function log(...stuff) {
  if (!debug)
    return;

  let msg = "SafeBrowsing: " + stuff.join(" ");
  Services.console.logStringMessage(msg);
  dump(msg + "\n");
}

var SafeBrowsing = {

  init: function(customURLs) {
    if (this.initialized) {
      log("Already initialized");
      return;
    }

    
    if (customURLs)
      urlList = customURLs;

    Services.prefs.addObserver("browser.safebrowsing", this.readPrefs, false);
    this.readPrefs();

    this.initProviderURLs();

    
    let listManager = Cc["@mozilla.org/url-classifier/listmanager;1"].
                      getService(Ci.nsIUrlListManager);

    listManager.setUpdateUrl(this.updateURL);
    
    
    if (this.phishingEnabled || this.malwareEnabled)
      listManager.setKeyUrl(this.keyURL);
    listManager.setGethashUrl(this.gethashURL);

    
    listManager.registerTable(phishingList, false);
    listManager.registerTable(malwareList, false);
    this.addMozEntries();

    this.controlUpdateChecking();
    this.initialized = true;

    log("init() finished");
  },


  initialized:     false,
  phishingEnabled: false,
  malwareEnabled:  false,

  updateURL:             null,
  keyURL:                null,
  reportURL:             null,
  gethashURL:            null,
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

    
    
    
    if (this.initialized)
      this.controlUpdateChecking();
  },


  initProviderURLs: function() {
    log("initializing provider URLs");

    
    let provID = Services.prefs.getIntPref("browser.safebrowsing.dataProvider");
    if (provID != 0) {
      Cu.reportError("unknown safebrowsing provider ID " + provID);
      return;
    }

    

    
    this.updateURL  = Services.urlFormatter.formatURL(urlList["updateURL"]);
    this.keyURL     = Services.urlFormatter.formatURL(urlList["keyURL"]);
    this.reportURL  = Services.urlFormatter.formatURL(urlList["reportURL"]);
    this.gethashURL = Services.urlFormatter.formatURL(urlList["gethashURL"]);

    
    this.reportGenericURL      = Services.urlFormatter.formatURL(urlList["reportGenericURL"]);
    this.reportErrorURL        = Services.urlFormatter.formatURL(urlList["reportErrorURL"]);
    this.reportPhishURL        = Services.urlFormatter.formatURL(urlList["reportPhishURL"]);
    this.reportMalwareURL      = Services.urlFormatter.formatURL(urlList["reportMalwareURL"]);
    this.reportMalwareErrorURL = Services.urlFormatter.formatURL(urlList["reportMalwareErrorURL"]);
  },


  controlUpdateChecking: function() {
    log("phishingEnabled:", this.phishingEnabled, "malwareEnabled:", this.malwareEnabled);

    let listManager = Cc["@mozilla.org/url-classifier/listmanager;1"].
                      getService(Ci.nsIUrlListManager);

    if (this.phishingEnabled)
      listManager.enableUpdate(phishingList);
    else
      listManager.disableUpdate(phishingList);

    if (this.malwareEnabled)
      listManager.enableUpdate(malwareList);
    else
      listManager.disableUpdate(malwareList);
  },


  addMozEntries: function() {
    
    
    const phishURL   = "mozilla.org/firefox/its-a-trap.html";
    const malwareURL = "mozilla.org/firefox/its-an-attack.html";

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
