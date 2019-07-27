



'use strict';

this.EXPORTED_SYMBOLS = ['RequestSyncApp'];

function debug(s) {
  
}

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import('resource://gre/modules/XPCOMUtils.jsm');

this.RequestSyncApp = function(aData) {
  debug('created');

  let keys = [ 'origin', 'manifestURL', 'isInBrowserElement' ];
  for (let i = 0; i < keys.length; ++i) {
    if (!(keys[i] in aData)) {
      dump("ERROR - RequestSyncApp must receive a full app object: " + keys[i] + " missing.");
      throw "ERROR!";
    }

    this["_" + keys[i]] = aData[keys[i]];
  }
}

this.RequestSyncApp.prototype = {
  classDescription: 'RequestSyncApp XPCOM Component',
  classID: Components.ID('{5a0b64db-a2be-4f08-a6c5-8bf2e3ae0c57}'),
  contractID: '@mozilla.org/dom/request-sync-manager;1',
  QueryInterface: XPCOMUtils.generateQI([]),

  get origin() {
    return this._origin;
  },

  get manifestURL() {
    return this._manifestURL;
  },

  get isInBrowserElement() {
    return this._isInBrowserElement;
  }
};
