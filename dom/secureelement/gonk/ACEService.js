





"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Promise.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "DOMApplicationRegistry",
                                  "resource://gre/modules/Webapps.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "SEUtils",
                                  "resource://gre/modules/SEUtils.jsm");

XPCOMUtils.defineLazyGetter(this, "SE", function() {
  let obj = {};
  Cu.import("resource://gre/modules/se_consts.js", obj);
  return obj;
});

let DEBUG = SE.DEBUG_ACE;
function debug(msg) {
  if (DEBUG) {
    dump("ACEservice: " + msg + "\n");
  }
}







function GPAccessDecision(rules, certHash, aid) {
  this.rules = rules;
  this.certHash = certHash;
  this.aid = aid;
}

GPAccessDecision.prototype = {
  isAccessAllowed: function isAccessAllowed() {
    
    
    
    
    
    
    
    
    
    
    
    let decision = this.rules.some(this._decideAppAccess.bind(this));
    return decision;
  },

  _decideAppAccess: function _decideAppAccess(rule) {
    let appMatched, appletMatched;

    
    
    
    
    
    
    
    
    
    
    
    
    

    
    appMatched = Array.isArray(rule.application) ?
      
      this._appCertHashMatches(rule.application) :
      
      rule.application === Ci.nsIAccessRulesManager.ALLOW_ALL;

    if (!appMatched) {
      return false; 
    }

    
    appletMatched = Array.isArray(rule.applet) ?
      
      SEUtils.arraysEqual(rule.applet, this.aid) :
      
      rule.applet === Ci.nsIAccessRulesManager.ALL_APPLET;

    return appletMatched;
  },

  _appCertHashMatches: function _appCertHashMatches(hashArray) {
    if (!Array.isArray(hashArray)) {
      return false;
    }

    return !!(hashArray.find((hash) => {
      return SEUtils.arraysEqual(hash, this.certHash);
    }));
  }
};

function ACEService() {
  this._rulesManagers = new Map();

  this._rulesManagers.set(
    SE.TYPE_UICC,
    Cc["@mozilla.org/secureelement/access-control/rules-manager;1"]
      .createInstance(Ci.nsIAccessRulesManager));
}

ACEService.prototype = {
  _rulesManagers: null,

  isAccessAllowed: function isAccessAllowed(localId, seType, aid) {
    let manifestURL = DOMApplicationRegistry.getManifestURLByLocalId(localId);
    if (!manifestURL) {
      return Promise.reject(Error("Missing manifest for app: " + localId));
    }

    let rulesManager = this._rulesManagers.get(seType);
    if (!rulesManager) {
      debug("App " + manifestURL + " tried to access '" + seType + "' SE" +
            " which is not supported.");
      return Promise.reject(Error("SE type '" + seType + "' not supported"));
    }

    return new Promise((resolve, reject) => {
      debug("isAccessAllowed for " + manifestURL + " to " + aid);

      
      this._getDevCertHashForApp(manifestURL).then((certHash) => {
        if (!certHash) {
          debug("App " + manifestURL + " tried to access SE, but no developer" +
                " certificate present");
          reject(Error("No developer certificate found"));
          return;
        }

        rulesManager.getAccessRules().then((rules) => {
          let decision = new GPAccessDecision(rules,
            SEUtils.hexStringToByteArray(certHash), aid);

          resolve(decision.isAccessAllowed());
        });
      });
    });
  },

  _getDevCertHashForApp: function getDevCertHashForApp(manifestURL) {
    return DOMApplicationRegistry.getManifestFor(manifestURL)
    .then((manifest) => {
      DEBUG && debug("manifest retrieved: " + JSON.stringify(manifest));

      
      
      
      
      
      
      let certHash = manifest.dev_cert_hash || "";
      return Promise.resolve(certHash);
    })
    .catch((error) => {
      return Promise.reject(Error("Not able to retrieve certificate hash: " +
                                  error));
    });
  },

  classID: Components.ID("{882a7463-2ca7-4d61-a89a-10eb6fd70478}"),
  contractID: "@mozilla.org/secureelement/access-control/ace;1",
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIAccessControlEnforcer])
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([ACEService]);

