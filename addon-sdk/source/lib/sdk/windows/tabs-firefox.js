


"use strict";

module.metadata = {
  "stability": "unstable"
};

const { Trait } = require("../deprecated/traits");
const { List } = require("../deprecated/list");
const { Tab } = require("../tabs/tab");
const { EventEmitter } = require("../deprecated/events");
const { EVENTS } = require("../tabs/events");
const { getOwnerWindow, getActiveTab, getTabs,
        openTab, activateTab } = require("../tabs/utils");
const { Options } = require("../tabs/common");
const { observer: tabsObserver } = require("../tabs/observer");

const TAB_BROWSER = "tabbrowser";







const WindowTabTracker = Trait.compose({
  


  _window: Trait.required,
  


  _emit: EventEmitter.required,
  _tabOptions: Trait.required,
  


  on: EventEmitter.required,
  removeListener: EventEmitter.required,
  


  _initWindowTabTracker: function _initWindowTabTracker() {
    
    
    
    this.tabs;

    
    this._onTabReady = this._emitEvent.bind(this, "ready");
    this._onTabOpen = this._onTabEvent.bind(this, "open");
    this._onTabClose = this._onTabEvent.bind(this, "close");
    this._onTabActivate = this._onTabEvent.bind(this, "activate");
    this._onTabDeactivate = this._onTabEvent.bind(this, "deactivate");
    this._onTabPinned = this._onTabEvent.bind(this, "pinned");
    this._onTabUnpinned = this._onTabEvent.bind(this, "unpinned");

    for each (let tab in getTabs(this._window)) {
      
      
      
      this._onTabOpen(tab);
    }

    
    this._onTabActivate(getActiveTab(this._window));

    
    tabsObserver.on("open", this._onTabOpen);
    tabsObserver.on("close", this._onTabClose);
    tabsObserver.on("activate", this._onTabActivate);
    tabsObserver.on("deactivate", this._onTabDeactivate);
    tabsObserver.on("pinned", this._onTabPinned);
    tabsObserver.on("unpinned", this._onTabUnpinned);
  },
  _destroyWindowTabTracker: function _destroyWindowTabTracker() {
    
    
    for each (let tab in this.tabs)
      this._emitEvent("close", tab);

    this._tabs._clear();

    tabsObserver.removeListener("open", this._onTabOpen);
    tabsObserver.removeListener("close", this._onTabClose);
    tabsObserver.removeListener("activate", this._onTabActivate);
    tabsObserver.removeListener("deactivate", this._onTabDeactivate);
  },
  _onTabEvent: function _onTabEvent(type, tab) {
    if (this._window === getOwnerWindow(tab)) {
      let options = this._tabOptions.shift() || {};
      options.tab = tab;
      options.window = this._public;

      
      let wrappedTab = Tab(options);

      
      if (type === "open")
        wrappedTab.on("ready", this._onTabReady);

      this._emitEvent(type, wrappedTab);
    }
  },
  _emitEvent: function _emitEvent(type, tab) {
    
    tabs._emit(type, tab);
    
    this._tabs._emit(type, tab);
  }
});
exports.WindowTabTracker = WindowTabTracker;










const TabList = List.resolve({ constructor: "_init" }).compose(
  
  EventEmitter.resolve({ toString: null }),
  Trait.compose({
    on: Trait.required,
    _emit: Trait.required,
    constructor: function TabList(options) {
      this._window = options.window;
      
      this.on(EVENTS.open.name, this._add.bind(this));

      
      this.on(EVENTS.close.name, this._remove.bind(this));

      
      this.on("activate", function onTabActivate(tab) {
        this._activeTab = tab;
      }.bind(this));

      
      this._init();
      
      
      return this;
    },
    get activeTab() this._activeTab,
    _activeTab: null,

    open: function open(options) {
      options = Options(options);
      this._window._tabOptions.push(options);
      let tab = openTab(this._window._window, options.url);
      if (!options.inBackground)
        activateTab(tab);
    }
  
  }).resolve({ toString: null })
);





var tabs = TabList({ window: null });
exports.tabs = tabs._public;






const WindowTabs = Trait.compose(
  WindowTabTracker,
  Trait.compose({
    _window: Trait.required,
    


    get tabs() (this._tabs || (this._tabs = TabList({ window: this })))._public,
    _tabs: null,
  })
);
exports.WindowTabs = WindowTabs;
