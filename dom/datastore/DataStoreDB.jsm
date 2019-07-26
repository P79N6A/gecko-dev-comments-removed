



'use strict';

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

this.EXPORTED_SYMBOLS = ['DataStoreDB'];

function debug(s) {
  
}

const DATASTOREDB_VERSION = 1;
const DATASTOREDB_OBJECTSTORE_NAME = 'DataStoreDB';

Cu.import('resource://gre/modules/IndexedDBHelper.jsm');

this.DataStoreDB = function DataStoreDB() {}

DataStoreDB.prototype = {

  __proto__: IndexedDBHelper.prototype,

  upgradeSchema: function(aTransaction, aDb, aOldVersion, aNewVersion) {
    debug('updateSchema');
    aDb.createObjectStore(DATASTOREDB_OBJECTSTORE_NAME, { autoIncrement: true });
  },

  init: function(aOrigin, aName) {
    let dbName = aOrigin + '_' + aName;
    this.initDBHelper(dbName, DATASTOREDB_VERSION,
                      [DATASTOREDB_OBJECTSTORE_NAME]);
  },

  txn: function(aType, aCallback, aErrorCb) {
    debug('Transaction request');
    this.newTxn(
      aType,
      DATASTOREDB_OBJECTSTORE_NAME,
      aCallback,
      function() {},
      aErrorCb
    );
  },

  delete: function() {
    debug('delete');
    this.close();
    indexedDB.deleteDatabase(this.dbName);
    debug('database deleted');
  }
}
