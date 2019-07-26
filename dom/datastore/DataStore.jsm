





'use strict'

var EXPORTED_SYMBOLS = ["DataStore"];



function DataStore(aAppId, aName, aOwner, aReadOnly) {
  this.appId = aAppId;
  this.name = aName;
  this.owner = aOwner;
  this.readOnly = aReadOnly;
}

DataStore.prototype = {
  appId: null,
  name: null,
  owner: null,
  readOnly: null,

  exposeObject: function(aWindow) {
    let self = this;
    let chromeObject = {
      get name() {
        return self.name;
      },

      get owner() {
        return self.owner;
      },

      get readOnly() {
        return self.readOnly;
      },

      












      __exposedProps__: {
        name: 'r',
        owner: 'r',
        readOnly: 'r'
      }
    };

    return chromeObject;
  }
};
