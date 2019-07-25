



var EXPORTED_SYMBOLS = ["SafeBrowsing"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");

const phishingList = "goog-phish-shavar";
const malwareList  = "goog-malware-shavar";

var debug = false;
function log(...stuff) {
  if (!debug)
    return;

  let msg = "SafeBrowsing: " + stuff.join(" ");
  Services.console.logStringMessage(msg);
  dump(msg + "\n");
}

var SafeBrowsing = {

  init: function() {
    if (this.initialized) {
      log("Already initialized");
      return;
    }

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

  provName:              null,
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

    let basePref = "browser.safebrowsing.provider.0.";
    this.provName = Services.prefs.getCharPref(basePref + "name");

    
    this.updateURL  = this.getUrlPref(basePref + "updateURL");
    this.keyURL     = this.getUrlPref(basePref + "keyURL");
    this.reportURL  = this.getUrlPref(basePref + "reportURL");
    this.gethashURL = this.getUrlPref(basePref + "gethashURL");

    
    this.reportGenericURL      = this.getUrlPref(basePref + "reportGenericURL");
    this.reportErrorURL        = this.getUrlPref(basePref + "reportErrorURL");
    this.reportPhishURL        = this.getUrlPref(basePref + "reportPhishURL");
    this.reportMalwareURL      = this.getUrlPref(basePref + "reportMalwareURL")
    this.reportMalwareErrorURL = this.getUrlPref(basePref + "reportMalwareErrorURL")
  },


  getUrlPref: function(prefName) {
    let MOZ_OFFICIAL_BUILD = false;
#ifdef OFFICIAL_BUILD
    MOZ_OFFICIAL_BUILD = true;
#endif

    let url = Services.prefs.getCharPref(prefName);

    let clientName = MOZ_OFFICIAL_BUILD ? "navclient-auto-ffox" : Services.appinfo.name;
    let clientVersion = Services.appinfo.version;

    
    
    url = url.replace(/\{moz:locale\}/g,  this.getLocale());
    url = url.replace(/\{moz:client\}/g,  clientName);
    url = url.replace(/\{moz:buildid\}/g, Services.appinfo.appBuildID);
    url = url.replace(/\{moz:version\}/g, clientVersion);

    log(prefName, "is", url);
    return url;
  },


  getLocale: function() {
    const localePref = "general.useragent.locale";

    let locale = Services.prefs.getCharPref(localePref);
    try {
      
      locale = Services.prefs.getComplexValue(localePref, Ci.nsIPrefLocalizedString).data;
    } catch (e) { }

    return locale;
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
