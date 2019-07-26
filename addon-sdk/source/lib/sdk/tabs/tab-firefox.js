


'use strict';

const { Trait } = require("../deprecated/traits");
const { EventEmitter } = require("../deprecated/events");
const { defer } = require("../lang/functional");
const { EVENTS } = require("./events");
const { getThumbnailURIForWindow } = require("../content/thumbnail");
const { getFaviconURIForLocation } = require("../io/data");
const { activateTab, getOwnerWindow, getBrowserForTab, getTabTitle, setTabTitle,
        getTabURL, setTabURL, getTabContentType, getTabId } = require('./utils');
const { getOwnerWindow: getPBOwnerWindow } = require('../private-browsing/window/utils');
const viewNS = require('sdk/core/namespace').ns();


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

    viewNS(this._public).tab = this._tab;
    getPBOwnerWindow.implement(this._public, getChromeTab);

    
    
    
    return this;
  },
  destroy: function destroy() {
    this._removeAllListeners();
    if (this._tab) {
      this._browser.removeEventListener(EVENTS.ready.dom, this._onReady, true);
      this._tab = null;
      TABS.splice(TABS.indexOf(this), 1);
    }
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

  


  get id() this._tab ? getTabId(this._tab) : undefined,

  




  get title() this._tab ? getTabTitle(this._tab) : undefined,
  set title(title) this._tab && setTabTitle(this._tab, title),

  




  get contentType() this._tab ? getTabContentType(this._tab) : undefined,

  




  get url() this._tab ? getTabURL(this._tab) : undefined,
  set url(url) this._tab && setTabURL(this._tab, url),
  



  get favicon() this._tab ? getFaviconURIForLocation(this.url) : undefined,
  


  get style() null, 
  




  get index()
    this._tab ?
    this._window.gBrowser.getBrowserIndexForDocument(this._contentDocument) :
    undefined,
  set index(value)
    this._tab && this._window.gBrowser.moveTabTo(this._tab, value),
  



  getThumbnail: function getThumbnail()
    this._tab ? getThumbnailURIForWindow(this._contentWindow) : undefined,
  



  get isPinned() this._tab ? this._tab.pinned : undefined,
  pin: function pin() {
    if (!this._tab)
      return;
    this._window.gBrowser.pinTab(this._tab);
  },
  unpin: function unpin() {
    if (!this._tab)
      return;
    this._window.gBrowser.unpinTab(this._tab);
  },

  



  attach: function attach(options) {
    if (!this._tab)
      return;
    
    
    let { Worker } = require('./worker');
    return Worker(options, this._contentWindow);
  },

  





  activate: defer(function activate() {
    if (!this._tab)
      return;
    activateTab(this._tab);
  }),
  


  close: function close(callback) {
    if (!this._tab)
      return;
    if (callback)
      this.once(EVENTS.close.name, callback);
    this._window.gBrowser.removeTab(this._tab);
  },
  


  reload: function reload() {
    if (!this._tab)
      return;
    this._window.gBrowser.reloadTab(this._tab);
  }
});

function getChromeTab(tab) {
  return getOwnerWindow(viewNS(tab).tab);
}

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
