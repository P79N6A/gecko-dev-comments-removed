





'use strict'



function debug(s) {
  
}

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import('resource://gre/modules/XPCOMUtils.jsm');
Cu.import('resource://gre/modules/Services.jsm');
Cu.import('resource://gre/modules/DataStore.jsm');



const DATASTORESERVICE_CID = Components.ID('{d193d0e2-c677-4a7b-bb0a-19155b470f2e}');

function DataStoreService() {
  debug('DataStoreService Constructor');

  let obs = Services.obs;
  if (!obs) {
    debug("DataStore Error: observer-service is null!");
    return;
  }

  obs.addObserver(this, 'webapps-clear-data', false);
}

DataStoreService.prototype = {
  
  stores: {},

  installDataStore: function(aAppId, aName, aOwner, aReadOnly) {
    debug('installDataStore - appId: ' + aAppId + ', aName: ' +
          aName + ', aOwner:' + aOwner + ', aReadOnly: ' +
          aReadOnly);

    if (aName in this.stores && aAppId in this.stores[aName]) {
      debug('This should not happen');
      return;
    }

    let store = new DataStore(aAppId, aName, aOwner, aReadOnly);

    if (!(aName in this.stores)) {
      this.stores[aName] = {};
    }

    this.stores[aName][aAppId] = store;
  },

  getDataStores: function(aWindow, aName) {
    debug('getDataStores - aName: ' + aName);
    let self = this;
    return new aWindow.Promise(function(resolve, reject) {
      let results = [];

      if (aName in self.stores) {
        for (let appId in self.stores[aName]) {
          let obj = self.stores[aName][appId].exposeObject(aWindow);
          results.push(obj);
        }
      }

      resolve(results);
    });
  },

  observe: function observe(aSubject, aTopic, aData) {
    debug('getDataStores - aTopic: ' + aTopic);
    if (aTopic != 'webapps-clear-data') {
      return;
    }

    let params =
      aSubject.QueryInterface(Ci.mozIApplicationClearPrivateDataParams);

    
    if (params.browserOnly) {
      return;
    }

    for (let key in this.stores) {
      if (params.appId in this.stores[key]) {
        this.stores[key][params.appId].delete();
        delete this.stores[key][params.appId];
      }

      if (!this.stores[key].length) {
        delete this.stores[key];
      }
    }
  },

  classID : DATASTORESERVICE_CID,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDataStoreService,
                                         Ci.nsIObserver]),
  classInfo: XPCOMUtils.generateCI({
    classID: DATASTORESERVICE_CID,
    contractID: '@mozilla.org/datastore-service;1',
    interfaces: [Ci.nsIDataStoreService, Ci.nsIObserver],
    flags: Ci.nsIClassInfo.SINGLETON
  })
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([DataStoreService]);
