



const { Cu } = require("chrome");

const EventEmitter = require("devtools/toolkit/event-emitter");
const { Connection } = require("devtools/client/connection-manager");
const { Promise: promise } =
  Cu.import("resource://gre/modules/Promise.jsm", {});
const { Task } = Cu.import("resource://gre/modules/Task.jsm", {});
const { devtools } =
  Cu.import("resource://gre/modules/devtools/Loader.jsm", {});

const _knownTabStores = new WeakMap();

let TabStore;

module.exports = TabStore = function(connection) {
  
  
  if (_knownTabStores.has(connection)) {
    return _knownTabStores.get(connection);
  }

  _knownTabStores.set(connection, this);

  EventEmitter.decorate(this);

  this._resetStore();

  this.destroy = this.destroy.bind(this);
  this._onStatusChanged = this._onStatusChanged.bind(this);

  this._connection = connection;
  this._connection.once(Connection.Events.DESTROYED, this.destroy);
  this._connection.on(Connection.Events.STATUS_CHANGED, this._onStatusChanged);
  this._onTabListChanged = this._onTabListChanged.bind(this);
  this._onTabNavigated = this._onTabNavigated.bind(this);
  this._onStatusChanged();
  return this;
};

TabStore.prototype = {

  destroy: function() {
    if (this._connection) {
      
      
      
      this._connection.off(Connection.Events.DESTROYED, this.destroy);
      this._connection.off(Connection.Events.STATUS_CHANGED, this._onStatusChanged);
      _knownTabStores.delete(this._connection);
      this._connection = null;
    }
  },

  _resetStore: function() {
    this.response = null;
    this.tabs = [];
  },

  _onStatusChanged: function() {
    if (this._connection.status == Connection.Status.CONNECTED) {
      
      this._connection.client.addListener("tabListChanged",
                                          this._onTabListChanged);
      this._connection.client.addListener("tabNavigated",
                                          this._onTabNavigated);
      this.listTabs();
    } else {
      if (this._connection.client) {
        this._connection.client.removeListener("tabListChanged",
                                               this._onTabListChanged);
        this._connection.client.removeListener("tabNavigated",
                                               this._onTabNavigated);
      }
      this._resetStore();
    }
  },

  _onTabListChanged: function() {
    this.listTabs();
  },

  _onTabNavigated: function(e, { from, title, url }) {
    if (!this._selectedTab || from !== this._selectedTab.actor) {
      return;
    }
    this._selectedTab.url = url;
    this._selectedTab.title = title;
    this.emit("navigate");
  },

  listTabs: function() {
    if (!this._connection || !this._connection.client) {
      return promise.reject();
    }
    let deferred = promise.defer();
    this._connection.client.listTabs(response => {
      if (response.error) {
        this._connection.disconnect();
        deferred.reject(response.error);
        return;
      }
      this.response = response;
      this.tabs = response.tabs;
      this._checkSelectedTab();
      deferred.resolve(response);
    });
    return deferred.promise;
  },

  
  
  
  _selectedTab: null,
  get selectedTab() {
    return this._selectedTab;
  },
  set selectedTab(tab) {
    this._selectedTab = tab;
    
    if (this._selectedTab) {
      this.getTargetForTab();
    }
  },

  _checkSelectedTab: function() {
    if (!this._selectedTab) {
      return;
    }
    let alive = this.tabs.some(tab => {
      return tab.actor === this._selectedTab.actor;
    });
    if (!alive) {
      this.emit("closed");
    }
  },

  getTargetForTab: function() {
    let store = this;
    return Task.spawn(function*() {
      
      
      
      
      yield store.listTabs();
      return devtools.TargetFactory.forRemoteTab({
        form: store._selectedTab,
        client: store._connection.client,
        chrome: false
      });
    });
  },

};
