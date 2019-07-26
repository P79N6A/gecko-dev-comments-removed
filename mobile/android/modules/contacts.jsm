



let EXPORTED_SYMBOLS = ["Contacts"];

const Cu = Components.utils;

let Contacts = {
  _providers: [],
  _contacts: [],

  _load: function _load() {
    this._contacts = [];

    this._providers.forEach(function(provider) {
      this._contacts = this._contacts.concat(provider.getContacts());
    }, this)
  },

  init: function init() {
    
    this._load();
  },

  refresh: function refresh() {
    
    this._load();
  },

  addProvider: function(aProvider) {
    this._providers.push(aProvider);
    this.refresh();
  },

  find: function find(aMatch) {
    let results = [];

    if (!this._contacts.length)
      return results;

    for (let field in aMatch) {
      
      let match = aMatch[field];
      this._contacts.forEach(function(aContact) {
        if (field in aContact && aContact[field].indexOf(match) != -1)
          results.push(aContact);
      });
    }
    return results;
  }
};
