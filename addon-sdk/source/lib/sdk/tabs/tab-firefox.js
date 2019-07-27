


'use strict';

const { Class } = require('../core/heritage');
const { observer } = require('./observer');
const { observer: windowObserver } = require('../windows/observer');
const { addListItem, removeListItem } = require('../util/list');
const { viewFor } = require('../view/core');
const { modelFor } = require('../model/core');
const { emit, setListeners } = require('../event/core');
const { EventTarget } = require('../event/target');
const { getBrowserForTab, setTabURL, getTabId, getTabURL, getTabForBrowser,
        getTabs, getTabTitle, setTabTitle, getIndex, closeTab, reload, move,
        activateTab, pin, unpin, isTab } = require('./utils');
const { isBrowser, getInnerId, isWindowPrivate } = require('../window/utils');
const { getThumbnailURIForWindow, BLANK } = require("../content/thumbnail");
const { when } = require('../system/unload');
const { ignoreWindow, isPrivate } = require('../private-browsing/utils')
const { defer } = require('../lang/functional');
const { getURL } = require('../url/utils');
const { frames, remoteRequire } = require('../remote/parent');
remoteRequire('sdk/content/tab-events');

const modelsFor = new WeakMap();
const viewsFor = new WeakMap();
const destroyed = new WeakMap();

const tabEvents = {};
exports.tabEvents = tabEvents;

function browser(tab) {
  return getBrowserForTab(viewsFor.get(tab));
}

function isDestroyed(tab) {
  return destroyed.has(tab);
}

function isClosed(tab) {
  if (!viewsFor.has(tab))
    return true;
  return viewsFor.get(tab).closing;
}

const Tab = Class({
  implements: [EventTarget],
  initialize: function(tabElement, options = null) {
    modelsFor.set(tabElement, this);
    viewsFor.set(this, tabElement);

    if (options) {
      EventTarget.prototype.initialize.call(this, options);

      if (options.isPinned)
        this.pin();

      
      
      if (!options.inBackground)
        this.activate();
    }

    getURL.implement(this, tab => tab.url);
    isPrivate.implement(this, tab => {
      return isWindowPrivate(viewsFor.get(tab).ownerDocument.defaultView);
    });
  },

  get id() {
    return isDestroyed(this) ? undefined : getTabId(viewsFor.get(this));
  },

  get title() {
    return isDestroyed(this) ? undefined : getTabTitle(viewsFor.get(this));
  },

  set title(val) {
    if (isDestroyed(this))
      return;

    setTabTitle(viewsFor.get(this), val);
  },

  get url() {
    return isDestroyed(this) ? undefined : getTabURL(viewsFor.get(this));
  },

  set url(val) {
    if (isDestroyed(this))
      return;

    setTabURL(viewsFor.get(this), val);
  },

  get contentType() {
    return isDestroyed(this) ? undefined : browser(this).documentContentType;
  },

  get index() {
    return isDestroyed(this) ? undefined : getIndex(viewsFor.get(this));
  },

  set index(val) {
    if (isDestroyed(this))
      return;

    move(viewsFor.get(this), val);
  },

  get isPinned() {
    return isDestroyed(this) ? undefined : viewsFor.get(this).pinned;
  },

  get window() {
    if (isClosed(this))
      return undefined;

    
    require('../windows');
    let tabElement = viewsFor.get(this);
    let domWindow = tabElement.ownerDocument.defaultView;
    return modelFor(domWindow);
  },

  get readyState() {
    
    return isDestroyed(this) ? undefined : browser(this).contentDocument.readyState;
  },

  pin: function() {
    if (isDestroyed(this))
      return;

    pin(viewsFor.get(this));
  },

  unpin: function() {
    if (isDestroyed(this))
      return;

    unpin(viewsFor.get(this));
  },

  close: function(callback) {
    let tabElement = viewsFor.get(this);

    if (isDestroyed(this) || !tabElement || !tabElement.parentNode) {
      if (callback)
        callback();
      return;
    }

    this.once('close', () => {
      this.destroy();
      if (callback)
        callback();
    });

    closeTab(tabElement);
  },

  reload: function() {
    if (isDestroyed(this))
      return;

    reload(viewsFor.get(this));
  },

  activate: defer(function() {
    if (isDestroyed(this))
      return;

    activateTab(viewsFor.get(this));
  }),

  getThumbnail: function() {
    if (isDestroyed(this))
      return BLANK;

    
    if (browser(this).isRemoteBrowser) {
      console.error('This method is not supported with E10S');
      return BLANK;
    }
    return getThumbnailURIForWindow(browser(this).contentWindow);
  },

  attach: function(options) {
    if (isDestroyed(this))
      return;

    
    
    let { Worker } = require('./worker');
    return Worker(options, browser(this).contentWindow);
  },

  destroy: function() {
    if (isDestroyed(this))
      return;

    destroyed.set(this, true);
  }
});
exports.Tab = Tab;

viewFor.define(Tab, tab => viewsFor.get(tab));



function maybeWindowFor(domWindow) {
  try {
    return modelFor(domWindow);
  }
  catch (e) {
    return null;
  }
}

function tabEmit(tab, event, ...args) {
  
  if (isDestroyed(tab))
    return;

  
  
  let tabElement = viewsFor.get(tab);
  let window = maybeWindowFor(tabElement.ownerDocument.defaultView);
  if (window)
    emit(window.tabs, event, tab, ...args);

  emit(tabEvents, event, tab, ...args);
  emit(tab, event, tab, ...args);
}

function windowClosed(domWindow) {
  if (!isBrowser(domWindow))
    return;

  for (let tabElement of getTabs(domWindow)) {
    tabEventListener("close", tabElement);
  }
}
windowObserver.on('close', windowClosed);


when(_ => {
  windowObserver.off('close', windowClosed);
});


function tabEventListener(event, tabElement, ...args) {
  let domWindow = tabElement.ownerDocument.defaultView;

  if (ignoreWindow(domWindow))
    return;

  
  if (event != "close" && (tabElement.closing || !tabElement.parentNode))
    return;

  let tab = modelsFor.get(tabElement);
  if (!tab)
    tab = new Tab(tabElement);

  let window = maybeWindowFor(domWindow);

  if (event == "open") {
    
    
    if (window)
      addListItem(window.tabs, tab);
    
  }
  else if (event == "close") {
    if (window)
      removeListItem(window.tabs, tab);
    
  }
  else if (event == "ready" || event == "load") {
    
    
    if (isBrowser(domWindow) && !domWindow.gBrowserInit.delayedStartupFinished)
      return;
  }

  tabEmit(tab, event, ...args);

  
  if (event == "close") {
    viewsFor.delete(tab);
    modelsFor.delete(tabElement);
  }
}
observer.on('*', tabEventListener);


frames.port.on('sdk/tab/event', (frame, event, ...args) => {
  if (!frame.isTab)
    return;

  let tabElement = getTabForBrowser(frame.frameElement);
  if (!tabElement)
    return;

  tabEventListener(event, tabElement, ...args);
});


modelFor.when(isTab, view => {
  return modelsFor.get(view);
});
