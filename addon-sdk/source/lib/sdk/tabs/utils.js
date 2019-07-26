



'use strict';

module.metadata = {
  'stability': 'unstable'
};





const { Ci } = require('chrome');
const { defer } = require("../lang/functional");
const { windows, isBrowser } = require('../window/utils');
const { isPrivateBrowsingSupported } = require('../self');


function getWindows() windows(null, { includePrivate: isPrivateBrowsingSupported });

function activateTab(tab, window) {
  let gBrowser = getTabBrowserForTab(tab);

  
  if (gBrowser) {
    gBrowser.selectedTab = tab;
  }
  
  else if (window && window.BrowserApp) {
    window.BrowserApp.selectTab(tab);
  }
  return null;
}
exports.activateTab = activateTab;

function getTabBrowser(window) {
  return window.gBrowser;
}
exports.getTabBrowser = getTabBrowser;

function getTabContainer(window) {
  return getTabBrowser(window).tabContainer;
}
exports.getTabContainer = getTabContainer;










function getTabs(window) {
  if (arguments.length === 0) {
    return getWindows().filter(isBrowser).reduce(function(tabs, window) {
      return tabs.concat(getTabs(window))
    }, []);
  }

  
  if (window.BrowserApp)
    return window.BrowserApp.tabs;

  
  return Array.slice(getTabContainer(window).children);
}
exports.getTabs = getTabs;

function getActiveTab(window) {
  return getSelectedTab(window);
}
exports.getActiveTab = getActiveTab;

function getOwnerWindow(tab) {
  
  if (tab.ownerDocument)
    return tab.ownerDocument.defaultView;

  
  return getWindowHoldingTab(tab);
}
exports.getOwnerWindow = getOwnerWindow;


function getWindowHoldingTab(rawTab) {
  for each (let window in getWindows()) {
    
    
    if (!window.BrowserApp)
      continue;

    for each (let tab in window.BrowserApp.tabs) {
      if (tab === rawTab)
        return window;
    }
  }

  return null;
}

function openTab(window, url, options) {
  options = options || {};

  
  if (window.BrowserApp) {
    return window.BrowserApp.addTab(url, {
      selected: options.inBackground ? false : true,
      pinned: options.isPinned || false,
      isPrivate: options.isPrivate || false
    });
  }

  
  let newTab = window.gBrowser.addTab(url);
  if (!options.inBackground) {
    activateTab(newTab);
  }
  return newTab;
};
exports.openTab = openTab;

function isTabOpen(tab) {
  
  return !!((tab.linkedBrowser) || getWindowHoldingTab(tab));
}
exports.isTabOpen = isTabOpen;

function closeTab(tab) {
  let gBrowser = getTabBrowserForTab(tab);
  
  if (gBrowser)
    return gBrowser.removeTab(tab);

  let window = getWindowHoldingTab(tab);
  
  if (window && window.BrowserApp)
    return window.BrowserApp.closeTab(tab);
  return null;
}
exports.closeTab = closeTab;

function getURI(tab) {
  if (tab.browser) 
    return tab.browser.currentURI.spec;
  return tab.linkedBrowser.currentURI.spec;
}
exports.getURI = getURI;

function getTabBrowserForTab(tab) {
  let outerWin = getOwnerWindow(tab);
  if (outerWin)
    return getOwnerWindow(tab).gBrowser;
  return null;
}
exports.getTabBrowserForTab = getTabBrowserForTab;

function getBrowserForTab(tab) {
  if (tab.browser) 
    return tab.browser;

  return tab.linkedBrowser;
}
exports.getBrowserForTab = getBrowserForTab;


function getContentWindowForTab(tab) {
  return getBrowserForTab(tab).contentWindow;
}
exports.getContentWindowForTab = getContentWindowForTab;

function getTabId(tab) {
  if (tab.browser) 
    return tab.id

  return String.split(tab.linkedPanel, 'panel').pop();
}
exports.getTabId = getTabId;

function getTabTitle(tab) {
  return getBrowserForTab(tab).contentDocument.title || tab.label || "";
}
exports.getTabTitle = getTabTitle;

function setTabTitle(tab, title) {
  title = String(title);
  if (tab.browser)
    tab.browser.contentDocument.title = title;
  tab.label = String(title);
}
exports.setTabTitle = setTabTitle;

function getTabContentWindow(tab) {
  return getBrowserForTab(tab).contentWindow;
}
exports.getTabContentWindow = getTabContentWindow;




function getAllTabContentWindows() {
  return getTabs().map(getTabContentWindow);
}
exports.getAllTabContentWindows = getAllTabContentWindows;


function getTabForContentWindow(window) {
  
  
  let browser = window.QueryInterface(Ci.nsIInterfaceRequestor)
                   .getInterface(Ci.nsIWebNavigation)
                   .QueryInterface(Ci.nsIDocShell)
                   .chromeEventHandler;

  
  if (!browser) {
    return false;
  }

  
  let chromeWindow = browser.ownerDocument.defaultView;

  
  
  
  if ('gBrowser' in chromeWindow && chromeWindow.gBrowser &&
      'browsers' in chromeWindow.gBrowser) {
    
    
    let browsers = chromeWindow.gBrowser.browsers;
    let i = browsers.indexOf(browser);
    if (i !== -1)
      return chromeWindow.gBrowser.tabs[i];
    return null;
  }
  
  else if ('BrowserApp' in chromeWindow) {
    return getTabForWindow(window);
  }

  return null;
}
exports.getTabForContentWindow = getTabForContentWindow;


function getTabForWindow(window) {
  for each (let { BrowserApp } in getWindows()) {
    if (!BrowserApp)
      continue;

    for each (let tab in BrowserApp.tabs) {
      if (tab.browser.contentWindow == window.top)
        return tab;
    }
  }
  return null; 
}

function getTabURL(tab) {
  if (tab.browser) 
    return String(tab.browser.currentURI.spec);
  return String(getBrowserForTab(tab).currentURI.spec);
}
exports.getTabURL = getTabURL;

function setTabURL(tab, url) {
  url = String(url);
  if (tab.browser)
    return tab.browser.loadURI(url);
  return getBrowserForTab(tab).loadURI(url);
}




exports.setTabURL = defer(setTabURL);

function getTabContentType(tab) {
  return getBrowserForTab(tab).contentDocument.contentType;
}
exports.getTabContentType = getTabContentType;

function getSelectedTab(window) {
  if (window.BrowserApp) 
    return window.BrowserApp.selectedTab;
  if (window.gBrowser)
    return window.gBrowser.selectedTab;
  return null;
}
exports.getSelectedTab = getSelectedTab;


function getTabForBrowser(browser) {
  for each (let window in getWindows()) {
    
    if (!window.BrowserApp)
      continue;

    for each (let tab in window.BrowserApp.tabs) {
      if (tab.browser === browser)
        return tab;
    }
  }
  return null;
}
exports.getTabForBrowser = getTabForBrowser;

