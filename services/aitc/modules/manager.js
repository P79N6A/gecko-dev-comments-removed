



"use strict";

const EXPORTED_SYMBOLS = ["AitcManager"];

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/Webapps.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");

Cu.import("resource://services-aitc/client.js");
Cu.import("resource://services-aitc/browserid.js");
Cu.import("resource://services-aitc/storage.js");
Cu.import("resource://services-common/log4moz.js");
Cu.import("resource://services-common/preferences.js");
Cu.import("resource://services-common/tokenserverclient.js");
Cu.import("resource://services-common/utils.js");

const PREFS = new Preferences("services.aitc.");
const INITIAL_TOKEN_DURATION = 240000; 
const DASHBOARD_URL = PREFS.get("dashboard.url");
const MARKETPLACE_URL = PREFS.get("marketplace.url");









function AitcManager(cb, premadeClient, premadeToken) {
  this._client = null;
  this._getTimer = null;
  this._putTimer = null;

  this._lastTokenTime = 0;
  this._tokenDuration = INITIAL_TOKEN_DURATION;
  this._premadeToken = premadeToken || null;
  this._invalidTokenFlag = false;

  this._lastEmail = null;
  this._dashboardWindow = null;

  this._log = Log4Moz.repository.getLogger("Service.AITC.Manager");
  this._log.level = Log4Moz.Level[Preferences.get("manager.log.level")];
  this._log.info("Loading AitC manager module");

  
  let self = this;
  this._pending = new AitcQueue("webapps-pending.json", function _queueDone() {
    
    self._log.info("AitC manager has finished loading");
    try {
      cb(true);
    } catch (e) {
      self._log.error(new Error("AitC manager callback threw " + e));
    }

    
    if (premadeClient) {
      self._client = premadeClient;
      cb(null, true);
      return;
    }

    
    
  });
}
AitcManager.prototype = {
  



  _ACTIVE: 1,
  _PASSIVE: 2,

  


  _clientState: null,
  get _state() {
    return this._clientState;
  },
  set _state(value) {
    if (this._clientState == value) {
      return;
    }
    this._clientState = value;
    this._setPoll();
  },

  



  appEvent: function appEvent(type, app) {
    
    let self = this;
    let obj = {type: type, app: app, retries: 0, lastTime: 0};
    this._pending.enqueue(obj, function _enqueued(err, rec) {
      if (err) {
        self._log.error("Could not add " + type + " " + app + " to queue");
        return;
      }

      
      if (self._client) {
        self._processQueue();
        return;
      }

      
      self._makeClient(function(err, client) {
        if (!err && client) {
          self._client = client;
          self._processQueue();
        }
        
      });
    });
  },

  



  userActive: function userActive(win) {
    
    this._dashboardWindow = win;

    if (this._client) {
      this._state = this._ACTIVE;
      return;
    }

    
    
    
    
    let self = this;
    this._makeClient(function(err, client) {
      if (err) {
        
        self._log.error("Client not created at Dashboard");
        return;
      }
      self._client = client;
      self._state = self._ACTIVE;
    }, true, win);
  },

  




  userIdle: function userIdle() {
    this._state = this._PASSIVE;
    this._dashboardWindow = null;
  },

  












  initialSchedule: function initialSchedule(cb) {
    let self = this;

    function startProcessQueue(num) {
      self._makeClient(function(err, client) {
        if (!err && client) {
          self._client = client;
          self._processQueue();
          return;
        }
      });
      cb(num);
    }

    
    
    if (Preferences.get("services.aitc.client.lastModified", "0") != "0") {
      if (this._pending.length) {
        startProcessQueue(-1);
      } else {
        cb(-1);
      }
      return;
    }

    DOMApplicationRegistry.getAllWithoutManifests(function gotAllApps(apps) {
      let done = 0;
      let appids = Object.keys(apps);
      let total = appids.length;
      self._log.info("First run, queuing all local apps: " + total + " found");

      function appQueued(err) {
        if (err) {
          self._log.error("Error queuing app " + apps[appids[done]].origin);
        }

        if (done == total) {
          self._log.info("Finished queuing all initial local apps");
          startProcessQueue(total);
          return;
        }

        let app = apps[appids[done]];
        let obj = {type: "install", app: app, retries: 0, lastTime: 0};

        done += 1;
        self._pending.enqueue(obj, appQueued);
      }
      appQueued();
    });
  },

  






  _setPoll: function _setPoll() {
    if (this._state == this._ACTIVE && !this._client) {
      throw new Error("_setPoll(ACTIVE) called without client");
    }
    if (this._state != this._ACTIVE && this._state != this._PASSIVE) {
      throw new Error("_state is invalid " + this._state);
    }

    if (!this._client) {
      
      self._log.warn("_setPoll called but user not logged in, ignoring");
      return;
    }

    
    if (this._pending.length && !(this._putTimer)) {
      
      
      this._processQueue();
      return;
    }

    
    let getFreq;
    if (this._state == this._ACTIVE) {
      CommonUtils.nextTick(this._checkServer, this);
      getFreq = PREFS.get("manager.getActiveFreq");
    } else {
      getFreq = PREFS.get("manager.getPassiveFreq");
    }

    
    if (this._getTimer) {
      this._getTimer.cancel();
      this._getTimer = null;
    }

    
    let self = this;
    this._log.info("Starting GET timer");
    this._getTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    this._getTimer.initWithCallback({notify: this._checkServer.bind(this)},
                                    getFreq, Ci.nsITimer.TYPE_REPEATING_SLACK);

    this._log.info("GET timer set, next attempt in " + getFreq + "ms");
  },

  




  _validateToken: function _validateToken(func) {
    let timeSinceLastToken = Date.now() - this._lastTokenTime;
    if (!this._invalidTokenFlag && timeSinceLastToken < this._tokenDuration) {
      this._log.info("Current token is valid");
      func();
      return;
    }

    this._log.info("Current token is invalid");
    let win;
    if (this._state == this.ACTIVE) {
      win = this._dashboardWindow;
    }

    let self = this;
    this._refreshToken(function(err, done) {
      if (!done) {
        self._log.warn("_checkServer could not refresh token, aborting");
        return;
      }
      func(err);
    }, win);
  },

  




  _checkServer: function _checkServer() {
    if (!this._client) {
      throw new Error("_checkServer called without a client");
    }

    if (this._pending.length) {
      this._log.warn("_checkServer aborted because of pending PUTs");
      return;
    }

    let self = this;
    this._validateToken(function validation(err) {
      if (err) {
        self._log.error(err);
      } else {
        self._getApps();
      }
    });
  },

  _getApps: function _getApps() {
    
    this._log.info("Attempting to getApps");

    let self = this;
    this._client.getApps(function gotApps(err, apps) {
      if (err) {
        
        if (err.authfailure) {
          self._invalidTokenFlag = true;
          self._validateToken(function revalidated(err) {
            if (!err) {
              self._getApps();
            }
          });
        } else {
          return;
        }
      }
      if (!apps) {
        
        return;
      }
      if (!apps.length) {
        
        self._log.info("No apps found on remote server");
        return;
      }

      
      AitcStorage.processApps(apps, function processedApps() {
        self._log.info("processApps completed successfully, changes applied");
      });
    });
  },

  




  _processQueue: function _processQueue() {
    if (!this._client) {
      throw new Error("_processQueue called without a client");
    }
    if (!this._pending.length) {
      throw new Error("_processQueue called with an empty queue");
    }

    if (this._putInProgress) {
      
      
      
      return;
    }

    let self = this;
    this._validateToken(function validation(err) {
      if (err) {
        self._log.error(err);
      } else {
        self._putApps();
      }
    });
  },

  _putApps: function _putApps() {
    this._putInProgress = true;
    let record = this._pending.peek();

    this._log.info("Processing record type " + record.type);

    let self = this;
    function _clientCallback(err, done) {
      
      if (err && !err.removeFromQueue) {
        self._log.info("PUT failed, re-adding to queue");
        
        if (err.authfailure) {
          self._invalidTokenFlag = true;
          self._validateToken(function validation(err) {
            if (err) {
              self._log.error("Failed to obtain an updated token");
            }
            _reschedule();
          });
          return;
        }

        
        record.retries += 1;
        record.lastTime = new Date().getTime();

        
        self._pending.enqueue(record, function(err, done) {
          if (err) {
            self._log.error("Enqueue failed " + err);
            _reschedule();
            return;
          }
          
          self._pending.dequeue(function(err, done) {
            if (err) {
              self._log.error("Dequeue failed " + err);
            }
            _reschedule();
            return;
          });
        });
      }

      
      self._log.info("_putApp asked us to remove it from queue");
      self._pending.dequeue(function(err, done) {
        if (err) {
          self._log.error("Dequeue failed " + e);
        }
        _reschedule();
      });
    }

    function _reschedule() {
      
      self._putInProgress = false;

      
      
      if (!self._pending.length) {
        
        self._setPoll();
        return;
      }

      let obj = self._pending.peek();
      let cTime = new Date().getTime();
      let freq = PREFS.get("manager.putFreq");

      
      if (obj.lastTime && ((cTime - obj.lastTime) < freq)) {
        self._log.info("Scheduling next processQueue in " + freq);
        CommonUtils.namedTimer(self._processQueue, freq, self, "_putTimer");
        return;
      }

      
      self._log.info("Queue non-empty, processing next PUT");
      self._processQueue();
    }

    switch (record.type) {
      case "install":
        this._client.remoteInstall(record.app, _clientCallback);
        break;
      case "uninstall":
        record.app.hidden = true;
        this._client.remoteUninstall(record.app, _clientCallback);
        break;
      default:
        this._log.warn(
          "Unrecognized type " + record.type + " in queue, removing"
        );
        let self = this;
        this._pending.dequeue(function _dequeued(err) {
          if (err) {
            self._log.error("Dequeue of unrecognized app type failed");
          }
          _reschedule();
        });
    }
  },

  




  _refreshToken: function _refreshToken(cb, win) {
    if (!this._client) {
      throw new Error("_refreshToken called without an active client");
    }

    this._log.info("Token refresh requested");

    let self = this;
    function refreshedAssertion(err, assertion) {
      if (!err) {
        self._getToken(assertion, function(err, token) {
          if (err) {
            cb(err, null);
            return;
          }
          self._lastTokenTime = Date.now();
          self._client.updateToken(token);
          self._invalidTokenFlag = false;
          cb(null, true);
        });
        return;
      }

      
      if (!win) {
        cb(err, null);
        return;
      }

      
      self._makeClient(function(err, client) {
        if (err) {
          cb(err, null);
          return;
        }

        
        self._client = client;
        self._invalidTokenFlag = false;
        cb(null, true);
      }, win);
    }

    let options = { audience: DASHBOARD_URL };
    if (this._lastEmail) {
      options.requiredEmail = this._lastEmail;
    } else {
      options.sameEmailAs = MARKETPLACE_URL;
    }
    if (this._premadeToken) {
      this._client.updateToken(this._premadeToken);
      this._tokenDuration = parseInt(this._premadeToken.duration, 10);
      this._lastTokenTime = Date.now();
      this._invalidTokenFlag = false;
      cb(null, true);
    } else {
      BrowserID.getAssertion(refreshedAssertion, options);
    }
  },

  


  _getToken: function _getToken(assertion, cb) {
    let url = PREFS.get("tokenServer.url") + "/1.0/aitc/1.0";
    let client = new TokenServerClient();

    this._log.info("Obtaining token from " + url);

    let self = this;
    try {
      client.getTokenFromBrowserIDAssertion(url, assertion, function(err, tok) {
        self._gotToken(err, tok, cb);
      });
    } catch (e) {
      cb(new Error(e), null);
    }
  },

  
  _gotToken: function _gotToken(err, tok, cb) {
    if (!err) {
      this._log.info("Got token from server: " + JSON.stringify(tok));
      this._tokenDuration = parseInt(tok.duration, 10);
      cb(null, tok);
      return;
    }

    let msg = "Error in _getToken: " + err;
    this._log.error(msg);
    cb(msg, null);
  },

  
  _extractEmail: function _extractEmail(assertion) {
    
    let chain = assertion.split("~");
    let len = chain.length;
    if (len < 2) {
      return null;
    }

    try {
      
      let cert = JSON.parse(atob(
        chain[0].split(".")[1].replace("-", "+", "g").replace("_", "/", "g")
      ));
      return cert.principal.email;
    } catch (e) {
      return null;
    }
  },

  



  _makeClient: function makeClient(cb, login, win) {
    if (!cb) {
      throw new Error("makeClient called without callback");
    }
    if (login && !win) {
      throw new Error("makeClient called with login as true but no win");
    }

    let self = this;
    let ctxWin = win;
    function processAssertion(val) {
      
      self._lastEmail = self._extractEmail(val);
      self._log.info("Got assertion from BrowserID, creating token");
      self._getToken(val, function(err, token) {
        if (err) {
          cb(err, null);
          return;
        }

        
        self._lastTokenTime = Date.now();

        
        cb(null, new AitcClient(
          token, new Preferences("services.aitc.client.")
        ));
      });
    }
    function gotSilentAssertion(err, val) {
      self._log.info("gotSilentAssertion called");
      if (err) {
        
        if (login) {
          self._log.info("Could not obtain silent assertion, retrying login");
          BrowserID.getAssertionWithLogin(function gotAssertion(err, val) {
            if (err) {
              self._log.error(err);
              cb(err, false);
              return;
            }
            processAssertion(val);
          }, ctxWin);
          return;
        }
        self._log.warn("Could not obtain assertion in _makeClient");
        cb(err, false);
      } else {
        processAssertion(val);
      }
    }

    
    self._log.info("Attempting to obtain assertion silently")
    BrowserID.getAssertion(gotSilentAssertion, {
      audience: DASHBOARD_URL,
      sameEmailAs: MARKETPLACE_URL
    });
  },

};
