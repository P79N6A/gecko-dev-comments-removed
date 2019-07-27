



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
    let deferred = Promise.defer();
    debug("message to dispatch: " + JSON.stringify(aMessage));
    debug("aManifest url: " + aManifestURL);
    if(!aMessage) {
      return deferred.resolve(false);
    }
    let aid = this._byteAIDToHex(aMessage.aid);
    let seName = aMessage.seName;

    appsService.getManifestFor(aManifestURL)
    .then((aManifest) => this._checkAppManifest(seName, aid, aManifest))
    .then(() => {
      
      
      
      debug("dispatching message");
      deferred.resolve(true);
    })
    .catch(() => {
      
      debug("not dispatching");
      deferred.resolve(false);
    });

    return deferred.promise;
  },

  
  
  
  _checkAppManifest: function _checkAppManifest(aSeName, aAid, aManifest) {
    debug("aManifest " + JSON.stringify(aManifest));

    let hciRules = aManifest["secure_element_access"] || [];
    let matchingRule = hciRules.find((rule) => {
      rule = rule.toUpperCase();
      if(rule === "*" || rule === (aSeName + "/" + aAid)) {
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

      return isMatching(rule.split('/')[0], aSeName) &&
             isMatching(rule.split('/')[1], aAid);
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
