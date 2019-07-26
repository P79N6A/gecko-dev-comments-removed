



"use strict";

this.EXPORTED_SYMBOLS = ["BrowserID"];

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

Cu.import("resource://services-common/utils.js");
Cu.import("resource://services-common/log4moz.js");
Cu.import("resource://services-common/preferences.js");

const PREFS = new Preferences("services.aitc.browserid.");





function BrowserIDService() {
  this._log = Log4Moz.repository.getLogger("Service.AITC.BrowserID");
  this._log.level = Log4Moz.Level[PREFS.get("log")];
}
BrowserIDService.prototype = {
  


  get ID_URI() {
    return PREFS.get("url");
  },

  



































  getAssertion: function getAssertion(cb, options) {
    if (!cb) {
      throw new Error("getAssertion called without a callback");
    }
    if (!options) {
      throw new Error("getAssertion called without any options");
    }
    if (!options.audience) {
      throw new Error("getAssertion called without an audience");
    }
    if (options.sameEmailAs && options.requiredEmail) {
      throw new Error(
        "getAssertion sameEmailAs and requiredEmail are mutually exclusive"
      );
    }

    new Sandbox(this._getEmails.bind(this, cb, options), this.ID_URI);
  },

  






























  getAssertionWithLogin: function getAssertionWithLogin(cb, win, options) {
    if (!cb) {
      throw new Error("getAssertionWithLogin called without a callback");
    }
    if (!win) {
      throw new Error("getAssertionWithLogin called without a window");
    }
    this._getAssertionWithLogin(cb, win);
  },


  



  
  _getEmails: function _getEmails(cb, options, sandbox) {
    let self = this;

    if (!sandbox) {
      cb(new Error("Sandbox not created"), null);
      return;
    }

    function callback(res) {
      let emails = {};
      try {
        emails = JSON.parse(res);
      } catch (e) {
        self._log.error("Exception in JSON.parse for _getAssertion: " + e);
      }
      self._gotEmails(emails, sandbox, cb, options);
    }
    sandbox.box.importFunction(callback);

    let scriptText =
      "var list = window.BrowserID.User.getStoredEmailKeypairs();" +
      "callback(JSON.stringify(list));";
    Cu.evalInSandbox(scriptText, sandbox.box, "1.8", this.ID_URI, 1);
  },

  
  _gotEmails: function _gotEmails(emails, sandbox, cb, options) {
    let keys = Object.keys(emails);

    
    if (!keys.length) {
      sandbox.free();

      let err = "User is not logged in, or no emails were found";
      this._log.error(err);
      try {
        cb(new Error(err), null);
      } catch(e) {
        this._log.warn("Callback threw in _gotEmails " +
                       CommonUtils.exceptionStr(e));
      }
      return;
    }

    

    
    if (options.requiredEmail) {
      this._getAssertionWithEmail(
        sandbox, cb, options.requiredEmail, options.audience
      );
      return;
    }

    
    if (options.sameEmailAs) {
      this._getAssertionWithDomain(
        sandbox, cb, options.sameEmailAs, options.audience
      );
      return;
    }

    
    this._getAssertionWithEmail(
      sandbox, cb, keys[0], options.audience
    );
    return;
  },

  



  _getAssertionWithLogin: function _getAssertionWithLogin(cb, win) {
    
    
    let pm = Services.perms;
    let principal = win.document.nodePrincipal;

    let oldPerm = pm.testExactPermissionFromPrincipal(principal, "popup");
    try {
      pm.addFromPrincipal(principal, "popup", pm.ALLOW_ACTION);
    } catch(e) {
      this._log.warn("Setting popup blocking to false failed " + e);
    }

    
    let sandbox = new Cu.Sandbox(win, {
      wantXrays:        false,
      sandboxPrototype: win
    });

    let self = this;
    function callback(val) {
      
      try {
        pm.addFromPrincipal(principal, "popup", oldPerm);
      } catch(e) {
        this._log.warn("Setting popup blocking to original value failed " + e);
      }

      if (val) {
        self._log.info("_getAssertionWithLogin succeeded");
        try {
          cb(null, val);
        } catch(e) {
          self._log.warn("Callback threw in _getAssertionWithLogin " +
                         CommonUtils.exceptionStr(e));
        }
      } else {
        let msg = "Could not obtain assertion in _getAssertionWithLogin";
        self._log.error(msg);
        try {
          cb(new Error(msg), null);
        } catch(e) {
          self._log.warn("Callback threw in _getAssertionWithLogin " +
                         CommonUtils.exceptionStr(e));
        }
      }
    }
    sandbox.importFunction(callback);

    function doGetAssertion() {
      self._log.info("_getAssertionWithLogin Started");
      let scriptText = "window.navigator.id.get(" +
                       "  callback, {allowPersistent: true}" +
                       ");";
      Cu.evalInSandbox(scriptText, sandbox, "1.8", self.ID_URI, 1);
    }

    
    if (!win.document || (win.document.readyState != "complete")) {
      win.addEventListener("DOMContentLoaded", function _contentLoaded() {
        win.removeEventListener("DOMContentLoaded", _contentLoaded, false);
        doGetAssertion();
      }, false);
    } else {
      doGetAssertion();
    }
  },

  


  _getAssertionWithEmail: function _getAssertionWithEmail(sandbox, cb, email,
                                                          audience) {
    let self = this;

    function onSuccess(res) {
      
      sandbox.free();

      
      
      if (!res) {
        let msg = "BrowserID.User.getAssertion empty assertion for " + email;
        self._log.error(msg);
        try {
          cb(new Error(msg), null);
        } catch(e) {
          self._log.warn("Callback threw in _getAssertionWithEmail " +
                         CommonUtils.exceptionStr(e));
        }
        return;
      }

      
      self._log.info("BrowserID.User.getAssertion succeeded");
      try {
        cb(null, res);
      } catch(e) {
        self._log.warn("Callback threw in _getAssertionWithEmail " +
                       CommonUtils.exceptionStr(e));
      }
    }

    function onError(err) {
      sandbox.free();

      self._log.info("BrowserID.User.getAssertion failed");
      try {
        cb(err, null);
      } catch(e) {
        self._log.warn("Callback threw in _getAssertionWithEmail " +
                       CommonUtils.exceptionStr(e));
      }
    }
    sandbox.box.importFunction(onSuccess);
    sandbox.box.importFunction(onError);

    self._log.info("_getAssertionWithEmail Started");
    let scriptText =
      "window.BrowserID.User.getAssertion(" +
        "'" + email + "', "     +
        "'" + audience + "', "  +
        "onSuccess, "           +
        "onError"               +
      ");";
    Cu.evalInSandbox(scriptText, sandbox.box, "1.8", this.ID_URI, 1);
  },

  



  _getAssertionWithDomain: function _getAssertionWithDomain(sandbox, cb, domain,
                                                            audience) {
    let self = this;

    function onDomainSuccess(email) {
      if (email) {
        self._getAssertionWithEmail(sandbox, cb, email, audience);
      } else {
        sandbox.free();
        try {
          cb(new Error("No email found for _getAssertionWithDomain"), null);
        } catch(e) {
          self._log.warn("Callback threw in _getAssertionWithDomain " +
                         CommonUtils.exceptionStr(e));
        }
      }
    }
    sandbox.box.importFunction(onDomainSuccess);

    
    self._log.info("_getAssertionWithDomain Started");
    let scriptText =
      "onDomainSuccess(window.BrowserID.Storage.site.get(" +
        "'" + domain + "', "  +
        "'email'"             +
      "));";
    Cu.evalInSandbox(scriptText, sandbox.box, "1.8", this.ID_URI, 1);
  },
};















function Sandbox(cb, uri) {
  this._uri = uri;

  
  
  try {
    this._createFrame();
    this._createSandbox(cb, uri);
  } catch(e) {
    this._log = Log4Moz.repository.getLogger("Service.AITC.BrowserID.Sandbox");
    this._log.level = Log4Moz.Level[PREFS.get("log")];
    this._log.error("Could not create Sandbox " + e);
    cb(null);
  }
}
Sandbox.prototype = {
  


  free: function free() {
    delete this.box;
    this._container.removeChild(this._frame);
    this._frame = null;
    this._container = null;
  },

  






  _createFrame: function _createFrame() {
    let doc = Services.wm.getMostRecentWindow("navigator:browser").document;

    
    let frame = doc.createElement("iframe");
    frame.setAttribute("type", "content");
    frame.setAttribute("collapsed", "true");
    doc.documentElement.appendChild(frame);

    
    let webNav = frame.docShell.QueryInterface(Ci.nsIWebNavigation);
    webNav.stop(Ci.nsIWebNavigation.STOP_NETWORK);

    
    this._frame = frame;
    this._container = doc.documentElement;
  },

  _createSandbox: function _createSandbox(cb, uri) {
    let self = this;
    this._frame.addEventListener(
      "DOMContentLoaded",
      function _makeSandboxContentLoaded(event) {
        if (event.target.location.toString() != uri) {
          return;
        }
        event.target.removeEventListener(
          "DOMContentLoaded", _makeSandboxContentLoaded, false
        );
        let workerWindow = self._frame.contentWindow;
        self.box = new Cu.Sandbox(workerWindow, {
          wantXrays:        false,
          sandboxPrototype: workerWindow
        });
        cb(self);
      },
      true
    );

    
    this._frame.docShell.loadURI(
      uri,
      this._frame.docShell.LOAD_FLAGS_NONE,
      null, 
      null, 
      null  
    );
  },
};

XPCOMUtils.defineLazyGetter(this, "BrowserID", function() {
  return new BrowserIDService();
});
