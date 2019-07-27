



"use strict";

const { classes: Cc, interfaces: Ci, results: Cr, utils: Cu } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Timer.jsm");
Cu.import("resource://gre/modules/LoginManagerContent.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/Promise.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "BrowserUtils",
                                  "resource://gre/modules/BrowserUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "LoginHelper",
                                  "resource://gre/modules/LoginHelper.jsm");

XPCOMUtils.defineLazyGetter(this, "log", () => {
  let logger = LoginHelper.createLogger("nsLoginManager");
  return logger.log.bind(logger);
});

const MS_PER_DAY = 24 * 60 * 60 * 1000;

function LoginManager() {
  this.init();
}

LoginManager.prototype = {

  classID: Components.ID("{cb9e0de8-3598-4ed7-857b-827f011ad5d8}"),
  QueryInterface : XPCOMUtils.generateQI([Ci.nsILoginManager,
                                          Ci.nsISupportsWeakReference,
                                          Ci.nsIInterfaceRequestor]),
  getInterface : function(aIID) {
    if (aIID.equals(Ci.mozIStorageConnection) && this._storage) {
      let ir = this._storage.QueryInterface(Ci.nsIInterfaceRequestor);
      return ir.getInterface(aIID);
    }

    if (aIID.equals(Ci.nsIVariant)) {
      
      return this;
    }

    throw new Components.Exception("Interface not available", Cr.NS_ERROR_NO_INTERFACE);
  },


  


  __formFillService : null, 
  get _formFillService() {
    if (!this.__formFillService)
      this.__formFillService =
                      Cc["@mozilla.org/satchel/form-fill-controller;1"].
                      getService(Ci.nsIFormFillController);
    return this.__formFillService;
  },


  _storage : null, 
  _prefBranch  : null, 
  _remember : true,  


  








  init : function () {

    
    this._observer._pwmgr            = this;

    
    this._prefBranch = Services.prefs.getBranch("signon.");
    this._prefBranch.addObserver("rememberSignons", this._observer, false);

    this._remember = this._prefBranch.getBoolPref("rememberSignons");

    
    Services.obs.addObserver(this._observer, "xpcom-shutdown", false);

    if (Services.appinfo.processType ===
        Services.appinfo.PROCESS_TYPE_DEFAULT) {
      Services.obs.addObserver(this._observer, "passwordmgr-storage-replace",
                               false);

      
      this._initStorage();
    }

    Services.obs.addObserver(this._observer, "gather-telemetry", false);
  },


  _initStorage : function () {
#ifdef ANDROID
    var contractID = "@mozilla.org/login-manager/storage/mozStorage;1";
#else
    var contractID = "@mozilla.org/login-manager/storage/json;1";
#endif
    try {
      var catMan = Cc["@mozilla.org/categorymanager;1"].
                   getService(Ci.nsICategoryManager);
      contractID = catMan.getCategoryEntry("login-manager-storage",
                                           "nsILoginManagerStorage");
      log("Found alternate nsILoginManagerStorage with contract ID:", contractID);
    } catch (e) {
      log("No alternate nsILoginManagerStorage registered");
    }

    this._storage = Cc[contractID].
                    createInstance(Ci.nsILoginManagerStorage);
    this.initializationPromise = this._storage.initialize();
  },


  


  





  _observer : {
    _pwmgr : null,

    QueryInterface : XPCOMUtils.generateQI([Ci.nsIObserver,
                                            Ci.nsISupportsWeakReference]),

    
    observe : function (subject, topic, data) {

      if (topic == "nsPref:changed") {
        var prefName = data;
        log("got change to", prefName, "preference");

        if (prefName == "rememberSignons") {
          this._pwmgr._remember =
              this._pwmgr._prefBranch.getBoolPref("rememberSignons");
        } else {
          log("Oops! Pref not handled, change ignored.");
        }
      } else if (topic == "xpcom-shutdown") {
        delete this._pwmgr.__formFillService;
        delete this._pwmgr._storage;
        delete this._pwmgr._prefBranch;
        this._pwmgr = null;
      } else if (topic == "passwordmgr-storage-replace") {
        Task.spawn(function () {
          yield this._pwmgr._storage.terminate();
          this._pwmgr._initStorage();
          yield this._pwmgr.initializationPromise;
          Services.obs.notifyObservers(null,
                       "passwordmgr-storage-replace-complete", null);
        }.bind(this));
      } else if (topic == "gather-telemetry") {
        
        
        this._pwmgr._gatherTelemetry(data ? parseInt(data)
                                          : new Date().getTime());
      } else {
        log("Oops! Unexpected notification:", topic);
      }
    }
  },

  











  _gatherTelemetry : function (referenceTimeMs) {
    function clearAndGetHistogram(histogramId) {
      let histogram = Services.telemetry.getHistogramById(histogramId);
      histogram.clear();
      return histogram;
    }

    clearAndGetHistogram("PWMGR_BLOCKLIST_NUM_SITES").add(
      this.getAllDisabledHosts({}).length
    );
    clearAndGetHistogram("PWMGR_NUM_SAVED_PASSWORDS").add(
      this.countLogins("", "", "")
    );
    clearAndGetHistogram("PWMGR_NUM_HTTPAUTH_PASSWORDS").add(
      this.countLogins("", null, "")
    );

    
    
    clearAndGetHistogram("PWMGR_SAVING_ENABLED").add(this._remember);

    
    if (!this.isLoggedIn) {
      return;
    }

    let logins = this.getAllLogins({});

    let usernamePresentHistogram = clearAndGetHistogram("PWMGR_USERNAME_PRESENT");
    let loginLastUsedDaysHistogram = clearAndGetHistogram("PWMGR_LOGIN_LAST_USED_DAYS");

    let hostnameCount = new Map();
    for (let login of logins) {
      usernamePresentHistogram.add(!!login.username);

      let hostname = login.hostname;
      hostnameCount.set(hostname, (hostnameCount.get(hostname) || 0 ) + 1);

      login.QueryInterface(Ci.nsILoginMetaInfo);
      let timeLastUsedAgeMs = referenceTimeMs - login.timeLastUsed;
      if (timeLastUsedAgeMs > 0) {
        loginLastUsedDaysHistogram.add(
          Math.floor(timeLastUsedAgeMs / MS_PER_DAY)
        );
      }
    }

    let passwordsCountHistogram = clearAndGetHistogram("PWMGR_NUM_PASSWORDS_PER_HOSTNAME");
    for (let count of hostnameCount.values()) {
      passwordsCountHistogram.add(count);
    }
  },





  




  





  initializationPromise : null,


  




  addLogin : function (login) {
    
    if (login.hostname == null || login.hostname.length == 0)
      throw new Error("Can't add a login with a null or empty hostname.");

    
    if (login.username == null)
      throw new Error("Can't add a login with a null username.");

    if (login.password == null || login.password.length == 0)
      throw new Error("Can't add a login with a null or empty password.");

    if (login.formSubmitURL || login.formSubmitURL == "") {
      
      if (login.httpRealm != null)
        throw new Error("Can't add a login with both a httpRealm and formSubmitURL.");
    } else if (login.httpRealm) {
      
      if (login.formSubmitURL != null)
        throw new Error("Can't add a login with both a httpRealm and formSubmitURL.");
    } else {
      
      throw new Error("Can't add a login without a httpRealm or formSubmitURL.");
    }


    
    var logins = this.findLogins({}, login.hostname, login.formSubmitURL,
                                 login.httpRealm);

    if (logins.some(function(l) login.matches(l, true)))
      throw new Error("This login already exists.");

    log("Adding login");
    return this._storage.addLogin(login);
  },

  




  removeLogin : function (login) {
    log("Removing login");
    return this._storage.removeLogin(login);
  },


  




  modifyLogin : function (oldLogin, newLogin) {
    log("Modifying login");
    return this._storage.modifyLogin(oldLogin, newLogin);
  },


  








  getAllLogins : function (count) {
    log("Getting a list of all logins");
    return this._storage.getAllLogins(count);
  },


  




  removeAllLogins : function () {
    log("Removing all logins");
    this._storage.removeAllLogins();
  },

  









  getAllDisabledHosts : function (count) {
    log("Getting a list of all disabled hosts");
    return this._storage.getAllDisabledHosts(count);
  },


  




  findLogins : function (count, hostname, formSubmitURL, httpRealm) {
    log("Searching for logins matching host:", hostname,
        "formSubmitURL:", formSubmitURL, "httpRealm:", httpRealm);

    return this._storage.findLogins(count, hostname, formSubmitURL,
                                    httpRealm);
  },


  







  searchLogins : function(count, matchData) {
   log("Searching for logins");

    return this._storage.searchLogins(count, matchData);
  },


  





  countLogins : function (hostname, formSubmitURL, httpRealm) {
    log("Counting logins matching host:", hostname,
        "formSubmitURL:", formSubmitURL, "httpRealm:", httpRealm);

    return this._storage.countLogins(hostname, formSubmitURL, httpRealm);
  },


  


  get uiBusy() {
    return this._storage.uiBusy;
  },


  


  get isLoggedIn() {
    return this._storage.isLoggedIn;
  },


  




  getLoginSavingEnabled : function (host) {
    log("Checking if logins to", host, "can be saved.");
    if (!this._remember)
      return false;

    return this._storage.getLoginSavingEnabled(host);
  },


  




  setLoginSavingEnabled : function (hostname, enabled) {
    
    if (hostname.indexOf("\0") != -1)
      throw new Error("Invalid hostname");

    log("Login saving for", hostname, "now enabled?", enabled);
    return this._storage.setLoginSavingEnabled(hostname, enabled);
  },

  









  autoCompleteSearchAsync : function (aSearchString, aPreviousResult,
                                      aElement, aCallback) {
    
    

    if (!this._remember) {
      setTimeout(function() {
        aCallback.onSearchCompletion(new UserAutoCompleteResult(aSearchString, []));
      }, 0);
      return;
    }

    log("AutoCompleteSearch invoked. Search is:", aSearchString);

    var previousResult;
    if (aPreviousResult) {
      previousResult = { searchString: aPreviousResult.searchString,
                         logins: aPreviousResult.wrappedJSObject.logins };
    } else {
      previousResult = null;
    }

    let rect = BrowserUtils.getElementBoundingScreenRect(aElement);
    LoginManagerContent._autoCompleteSearchAsync(aSearchString, previousResult,
                                                 aElement, rect)
                       .then(function(logins) {
                         let results =
                             new UserAutoCompleteResult(aSearchString, logins);
                         aCallback.onSearchCompletion(results);
                       })
                       .then(null, Cu.reportError);
  },
}; 

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([LoginManager]);
