


'use strict';

const { Trait } = require("../deprecated/traits");
const { EventEmitter } = require("../deprecated/events");
const { defer } = require("../lang/functional");
const { EVENTS } = require("./events");
const { getThumbnailURIForWindow } = require("../content/thumbnail");
const { getFaviconURIForLocation } = require("../io/data");
const { activateTab, getOwnerWindow, getBrowserForTab, getTabTitle, setTabTitle,
        getTabURL, setTabURL, getTabContentType, getTabId } = require('./utils');


const TABS = [];




const TabTrait = Trait.compose(EventEmitter, {
  on: Trait.required,
  _emit: Trait.required,
  


  _tab: null,
  


  window: null,
  constructor: function Tab(options) {
    this._onReady = this._onReady.bind(this);
    this._tab = options.tab;
    
    let window = this.window = options.window || require('../windows').BrowserWindow({ window: getOwnerWindow(this._tab) });

    
    for each (let type in EVENTS) {
      let listener = options[type.listener];
      if (listener)
        this.on(type.name, options[type.listener]);
      if ('ready' != type.name) 
        window.tabs.on(type.name, this._onEvent.bind(this, type.name));
    }

    this.on(EVENTS.close.name, this.destroy.bind(this));
    this._browser.addEventListener(EVENTS.ready.dom, this._onReady, true);

    if (options.isPinned)
      this.pin();

    
    
    
    return this;
  },
  destroy: function destroy() {
    this._removeAllListeners();
    this._browser.removeEventListener(EVENTS.ready.dom, this._onReady, true);
  },

  



  _onReady: function _onReady(event) {
    
    if (event.target == this._contentDocument)
      this._emit(EVENTS.ready.name, this._public);
  },
  




  _onEvent: function _onEvent(type, tab) {
    if (tab == this._public)
      this._emit(type, tab);
  },
  


  get _browser() getBrowserForTab(this._tab),
  


  get _window() getOwnerWindow(this._tab),
  


  get _contentDocument() this._browser.contentDocument,
  


  get _contentWindow() this._browser.contentWindow,

  


  get id() getTabId(this._tab),

  




  get title() getTabTitle(this._tab),
  set title(title) setTabTitle(this._tab, title),

  




  get contentType() getTabContentType(this._tab),

  




  get url() getTabURL(this._tab),
  set url(url) setTabURL(this._tab, url),
  



  get favicon() getFaviconURIForLocation(this.url),
  


  get style() null, 
  




  get index()
    this._window.gBrowser.getBrowserIndexForDocument(this._contentDocument),
  set index(value) this._window.gBrowser.moveTabTo(this._tab, value),
  



  getThumbnail: function getThumbnail()
    getThumbnailURIForWindow(this._contentWindow),
  



  get isPinned() this._tab.pinned,
  pin: function pin() {
    this._window.gBrowser.pinTab(this._tab);
  },
  unpin: function unpin() {
    this._window.gBrowser.unpinTab(this._tab);
  },

  



  attach: function attach(options) {
    
    
    let { Worker } = require('./worker');
    return Worker(options, this._contentWindow);
  },

  





  activate: defer(function activate() {
    activateTab(this._tab);
  }),
  


  close: function close(callback) {
    if (callback)
      this.once(EVENTS.close.name, callback);
    this._window.gBrowser.removeTab(this._tab);
  },
  


  reload: function reload() {
    this._window.gBrowser.reloadTab(this._tab);
  }
});

function Tab(options) {
  let chromeTab = options.tab;
  for each (let tab in TABS) {
    if (chromeTab == tab._tab)
      return tab._public;
  }
  let tab = TabTrait(options);
  TABS.push(tab);
  return tab._public;
}
Tab.prototype = TabTrait.prototype;
exports.Tab = Tab;
