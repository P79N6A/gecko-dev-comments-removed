


'use strict';

const { Cc, Ci } = require('chrome');
const { Class } = require('../core/heritage');
const { tabNS, rawTabNS } = require('./namespace');
const { EventTarget } = require('../event/target');
const { activateTab, getTabTitle, setTabTitle, closeTab, getTabURL,
        getTabContentWindow, getTabForBrowser, setTabURL, getOwnerWindow,
        getTabContentDocument, getTabContentType, getTabId } = require('./utils');
const { emit } = require('../event/core');
const { isPrivate } = require('../private-browsing/utils');
const { isWindowPrivate } = require('../window/utils');
const { when: unload } = require('../system/unload');
const { BLANK } = require('../content/thumbnail');
const { viewFor } = require('../view/core');
const { EVENTS } = require('./events');

const ERR_FENNEC_MSG = 'This method is not yet supported by Fennec';

const Tab = Class({
  extends: EventTarget,
  initialize: function initialize(options) {
    options = options.tab ? options : { tab: options };
    let tab = options.tab;

    EventTarget.prototype.initialize.call(this, options);
    let tabInternals = tabNS(this);
    rawTabNS(tab).tab = this;

    let window = tabInternals.window = options.window || getOwnerWindow(tab);
    tabInternals.tab = tab;

    
    let onReady = tabInternals.onReady = onTabReady.bind(this);
    tab.browser.addEventListener(EVENTS.ready.dom, onReady, false);

    
    let onPageShow = tabInternals.onPageShow = onTabPageShow.bind(this);
    tab.browser.addEventListener(EVENTS.pageshow.dom, onPageShow, false);

    
    let onLoad = tabInternals.onLoad = onTabLoad.bind(this);
    tab.browser.addEventListener(EVENTS.load.dom, onLoad, true);

    
    let onClose = tabInternals.onClose = onTabClose.bind(this);
    window.BrowserApp.deck.addEventListener(EVENTS.close.dom, onClose, false);

    unload(cleanupTab.bind(null, this));
  },

  




  get title() getTabTitle(tabNS(this).tab),
  set title(title) setTabTitle(tabNS(this).tab, title),

  




  get url() {
    return tabNS(this).closed ? undefined : getTabURL(tabNS(this).tab);
  },
  set url(url) setTabURL(tabNS(this).tab, url),

  



  get favicon() {
    





    console.error(
      'tab.favicon is deprecated, and currently ' +
      'favicon helpers are not yet supported by Fennec'
    );

    
    return 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAAEklEQVQ4jWNgGAWjYBSMAggAAAQQAAF/TXiOAAAAAElFTkSuQmCC';
  },

  getThumbnail: function() {
    
    console.error(ERR_FENNEC_MSG);

    
    return BLANK;
  },

  


  get readyState() {
    let doc = getTabContentDocument(tabNS(this).tab);
    return doc && doc.readyState || 'uninitialized';
  },

  get id() {
    return getTabId(tabNS(this).tab);
  },

  




  get index() {
    if (tabNS(this).closed) return undefined;

    let tabs = tabNS(this).window.BrowserApp.tabs;
    let tab = tabNS(this).tab;
    for (var i = tabs.length; i >= 0; i--) {
      if (tabs[i] === tab)
        return i;
    }
    return null;
  },
  set index(value) {
    console.error(ERR_FENNEC_MSG); 
  },

  



  get isPinned() {
    console.error(ERR_FENNEC_MSG); 
    return false; 
  },
  pin: function pin() {
    console.error(ERR_FENNEC_MSG); 
  },
  unpin: function unpin() {
    console.error(ERR_FENNEC_MSG); 
  },

  




  get contentType() getTabContentType(tabNS(this).tab),

  



  attach: function attach(options) {
    
    
    let { Worker } = require('./worker');
    return Worker(options, getTabContentWindow(tabNS(this).tab));
  },

  


  activate: function activate() {
    activateTab(tabNS(this).tab, tabNS(this).window);
  },

  


  close: function close(callback) {
    let tab = this;
    this.once(EVENTS.close.name, function () {
      tabNS(tab).closed = true;
      if (callback) callback();
    });

    closeTab(tabNS(this).tab);
  },

  


  reload: function reload() {
    tabNS(this).tab.browser.reload();
  }
});
exports.Tab = Tab;



viewFor.define(Tab, x => tabNS(x).tab);

function cleanupTab(tab) {
  let tabInternals = tabNS(tab);
  if (!tabInternals.tab)
    return;

  if (tabInternals.tab.browser) {
    tabInternals.tab.browser.removeEventListener(EVENTS.ready.dom, tabInternals.onReady, false);
    tabInternals.tab.browser.removeEventListener(EVENTS.pageshow.dom, tabInternals.onPageShow, false);
    tabInternals.tab.browser.removeEventListener(EVENTS.load.dom, tabInternals.onLoad, true);
  }
  tabInternals.onReady = null;
  tabInternals.onPageShow = null;
  tabInternals.onLoad = null;
  tabInternals.window.BrowserApp.deck.removeEventListener(EVENTS.close.dom, tabInternals.onClose, false);
  tabInternals.onClose = null;
  rawTabNS(tabInternals.tab).tab = null;
  tabInternals.tab = null;
  tabInternals.window = null;
}

function onTabReady(event) {
  let win = event.target.defaultView;

  
  if (win === win.top) {
    emit(this, 'ready', this);
  }
}

function onTabLoad (event) {
  let win = event.target.defaultView;

  
  if (win === win.top) {
    emit(this, 'load', this);
  }
}

function onTabPageShow(event) {
  let win = event.target.defaultView;
  if (win === win.top)
    emit(this, 'pageshow', this, event.persisted);
}


function onTabClose(event) {
  let rawTab = getTabForBrowser(event.target);
  if (tabNS(this).tab !== rawTab)
    return;

  emit(this, EVENTS.close.name, this);
  cleanupTab(this);
};

isPrivate.implement(Tab, tab => {
  return isWindowPrivate(getTabContentWindow(tabNS(tab).tab));
});
