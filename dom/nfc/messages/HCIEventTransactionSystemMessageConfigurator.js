



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Promise.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "appsService",
                                   "@mozilla.org/AppsService;1",
                                   "nsIAppsService");

let DEBUG = false;
function debug(aMsg) {
  if (DEBUG) {
    dump("-- HCIEventTransactionSystemMessageConfigurator.js " + Date.now() + " : " + aMsg + "\n");
  }
}




function HCIEventTransactionSystemMessageConfigurator() {
  debug("HCIEventTransactionSystemMessageConfigurator");
}

HCIEventTransactionSystemMessageConfigurator.prototype = {
  get mustShowRunningApp() {
    debug("mustShowRunningApp returning true");
    return true;
  },

  shouldDispatch: function shouldDispatch(aManifestURL, aPageURL, aType, aMessage, aExtra) {
    DEBUG && debug("message to dispatch: " + JSON.stringify(aMessage));
    debug("aManifest url: " + aManifestURL);

    if (!aMessage) {
      return Promise.resolve(false);
    }

    return new Promise((resolve, reject) => {
      appsService.getManifestFor(aManifestURL)
      .then((aManifest) => this._checkAppManifest(aMessage.seName, aMessage.aid, aManifest))
      .then(() => {
        
        
        
        debug("dispatching message");
        resolve(true);
      })
      .catch(() => {
        
        debug("not dispatching");
        resolve(false);
      });
    });
  },

  
  
  
  _checkAppManifest: function _checkAppManifest(aSeName, aAid, aManifest) {
    DEBUG && debug("aManifest " + JSON.stringify(aManifest));

    
    
    let aid = this._byteAIDToHex(aAid);
    let seName = aSeName.toUpperCase();

    let hciRules = aManifest["secure_element_access"] || [];
    let matchingRule = hciRules.find((rule) => {
      rule = rule.toUpperCase();
      if(rule === "*" || rule === (seName + "/" + aid)) {
        return true;
      }

      let isMatching = (match, element) => {
        if(match === "*") {
          return true;
        }
        if(match.charAt(match.length - 1) === '*') {
          return element.indexOf(match.substr(0,match.length - 1)) === 0;
        }

        return match === element;
      };

      return isMatching(rule.split('/')[0], seName) &&
             isMatching(rule.split('/')[1], aid);
    });

    return (matchingRule) ? Promise.resolve() : Promise.reject();
  },

  
  _byteAIDToHex: function _byteAIDToHex(uint8arr) {
    if (!uint8arr) {
      return "";
    }

    var hexStr = "";
    for (var i = 0; i < uint8arr.length; i++) {
      var hex = (uint8arr[i] & 0xff).toString(16);
      hex = (hex.length === 1) ? '0' + hex : hex;
      hexStr += hex;
    }
    return hexStr.toUpperCase();
  },

  classID: Components.ID("{b501edd0-28bd-11e4-8c21-0800200c9a66}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISystemMessagesConfigurator])
}

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([HCIEventTransactionSystemMessageConfigurator]);
