 


'use strict';

const { Trait } = require("../deprecated/traits");
const { EventEmitter } = require("../deprecated/events");
const { defer } = require("../lang/functional");
const { has } = require("../util/array");
const { EVENTS } = require("./events");
const { getThumbnailURIForWindow } = require("../content/thumbnail");
const { getFaviconURIForLocation } = require("../io/data");
const { activateTab, getOwnerWindow, getBrowserForTab, getTabTitle, setTabTitle,
        getTabURL, setTabURL, getTabContentType, getTabId } = require('./utils');
const { isPrivate } = require('../private-browsing/utils');
const { isWindowPrivate } = require('../window/utils');
const viewNS = require('../core/namespace').ns();
const { deprecateUsage } = require('../util/deprecate');
const { getURL } = require('../url/utils');
const { viewFor } = require('../view/core');
const { observer } = require('./observer');


const TABS = [];




const TabTrait = Trait.compose(EventEmitter, {
  on: Trait.required,
  _emit: Trait.required,
  


  _tab: null,
  


  window: null,
  constructor: function Tab(options) {
    this._onReady = this._onReady.bind(this);
    this._onLoad = this._onLoad.bind(this);
    this._onPageShow = this._onPageShow.bind(this);
    this._tab = options.tab;
    
    let window = this.window = options.window || require('../windows').BrowserWindow({ window: getOwnerWindow(this._tab) });

    
    for each (let type in EVENTS) {
      let listener = options[type.listener];
      if (listener) {
        this.on(type.name, options[type.listener]);
      }
      
      if (!has(['ready', 'load', 'pageshow'], (type.name)))
        window.tabs.on(type.name, this._onEvent.bind(this, type.name));
    }

    this.on(EVENTS.close.name, this.destroy.bind(this));

    this._browser.addEventListener(EVENTS.ready.dom, this._onReady, true);
    this._browser.addEventListener(EVENTS.load.dom, this._onLoad, true);
    this._browser.addEventListener(EVENTS.pageshow.dom, this._onPageShow, true);

    if (options.isPinned)
      this.pin();

    viewNS(this._public).tab = this._tab;
    viewFor.implement(this._public, getTabView);
    isPrivate.implement(this._public, tab => isWindowPrivate(getChromeTab(tab)));

    
    getURL.implement(this._public, (function (obj) this._public.url).bind(this));

    
    
    
    return this;
  },
  destroy: function destroy() {
    this._removeAllListeners();
    if (this._tab) {
      let browser = this._browser;
      
      if (browser) {
        browser.removeEventListener(EVENTS.ready.dom, this._onReady, true);
        browser.removeEventListener(EVENTS.load.dom, this._onLoad, true);
        browser.removeEventListener(EVENTS.pageshow.dom, this._onPageShow, true);
      }
      this._tab = null;
      TABS.splice(TABS.indexOf(this), 1);
    }
  },

  



  _onReady: function _onReady(event) {
    
    if (event.target == this._contentDocument)
      this._emit(EVENTS.ready.name, this._public);
  },

  



  _onLoad: function _onLoad(event) {
    
    if (event.target == this._contentDocument) {
      this._emit(EVENTS.load.name, this._public);
    }
  },

  



  _onPageShow: function _onPageShow(event) {
    
    if (event.target == this._contentDocument) {
      this._emit(EVENTS.pageshow.name, this._public, event.persisted);
    }
  },
  




  _onEvent: function _onEvent(type, tab) {
    if (viewNS(tab).tab == this._tab)
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
  



  get favicon() {
    deprecateUsage(
      'tab.favicon is deprecated, ' +
      'please use require("sdk/places/favicon").getFavicon instead.'
    );
    return this._tab ? getFaviconURIForLocation(this.url) : undefined
  },
  


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
    
    if (!this._tab || !this._tab.parentNode) {
      if (callback)
        callback();
      return;
    }
    if (callback) {
      if (this.window.tabs.activeTab && (this.window.tabs.activeTab.id == this.id))
        observer.once('select', callback);
      else
        this.once(EVENTS.close.name, callback);
    }
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



const getTabView = tab => viewNS(tab).tab;

function Tab(options, existingOnly) {
  let chromeTab = options.tab;
  for each (let tab in TABS) {
    if (chromeTab == tab._tab)
      return tab._public;
  }
  
  
  if (existingOnly)
    return null;

  let tab = TabTrait(options);
  TABS.push(tab);
  return tab._public;
}
Tab.prototype = TabTrait.prototype;
exports.Tab = Tab;
