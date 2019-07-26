


'use strict';

module.metadata = {
  'stability': 'unstable'
};

const { Cc, Ci } = require('chrome');
const array = require('../util/array');
const observers = require('../deprecated/observer-service');
const { defer } = require('sdk/core/promise');

const windowWatcher = Cc['@mozilla.org/embedcomp/window-watcher;1'].
                       getService(Ci.nsIWindowWatcher);
const appShellService = Cc['@mozilla.org/appshell/appShellService;1'].
                        getService(Ci.nsIAppShellService);
const WM = Cc['@mozilla.org/appshell/window-mediator;1'].
           getService(Ci.nsIWindowMediator);

const BROWSER = 'navigator:browser',
      URI_BROWSER = 'chrome://browser/content/browser.xul',
      NAME = '_blank',
      FEATURES = 'chrome,all,dialog=no';

function isWindowPrivate(win) {
  if (!win)
    return false;

  
  
  
  try {
    return win.QueryInterface(Ci.nsIInterfaceRequestor)
                  .getInterface(Ci.nsIWebNavigation)
                  .QueryInterface(Ci.nsILoadContext)
                  .usePrivateBrowsing;
  }
  catch(e) {}

  
  try {
    return !!win.docShell.QueryInterface(Ci.nsILoadContext).usePrivateBrowsing;
  }
  catch (e) {}

  return false;
}
exports.isWindowPrivate = isWindowPrivate;

function getMostRecentBrowserWindow() {
  return getMostRecentWindow(BROWSER);
}
exports.getMostRecentBrowserWindow = getMostRecentBrowserWindow;

function getHiddenWindow() {
  return appShellService.hiddenDOMWindow;
}
exports.getHiddenWindow = getHiddenWindow;

function getMostRecentWindow(type) {
  return WM.getMostRecentWindow(type);
}
exports.getMostRecentWindow = getMostRecentWindow;




function getInnerId(window) {
  return window.QueryInterface(Ci.nsIInterfaceRequestor).
                getInterface(Ci.nsIDOMWindowUtils).currentInnerWindowID;
};
exports.getInnerId = getInnerId;




function getOuterId(window) {
  return window.QueryInterface(Ci.nsIInterfaceRequestor).
                getInterface(Ci.nsIDOMWindowUtils).outerWindowID;
};
exports.getOuterId = getOuterId;




function getXULWindow(window) {
  return window.QueryInterface(Ci.nsIInterfaceRequestor).
    getInterface(Ci.nsIWebNavigation).
    QueryInterface(Ci.nsIDocShellTreeItem).
    treeOwner.QueryInterface(Ci.nsIInterfaceRequestor).
    getInterface(Ci.nsIXULWindow);
};
exports.getXULWindow = getXULWindow;

function getDOMWindow(xulWindow) {
  return xulWindow.QueryInterface(Ci.nsIInterfaceRequestor).
    getInterface(Ci.nsIDOMWindow);
}
exports.getDOMWindow = getDOMWindow;




function getBaseWindow(window) {
  return window.QueryInterface(Ci.nsIInterfaceRequestor).
    getInterface(Ci.nsIWebNavigation).
    QueryInterface(Ci.nsIDocShell).
    QueryInterface(Ci.nsIDocShellTreeItem).
    treeOwner.
    QueryInterface(Ci.nsIBaseWindow);
}
exports.getBaseWindow = getBaseWindow;

function getWindowDocShell(window) window.gBrowser.docShell;
exports.getWindowDocShell = getWindowDocShell;

function getWindowLoadingContext(window) {
  return getWindowDocShell(window).
         QueryInterface(Ci.nsILoadContext);
}
exports.getWindowLoadingContext = getWindowLoadingContext;








function backgroundify(window, options) {
  let base = getBaseWindow(window);
  base.visibility = false;
  base.enabled = false;
  appShellService.unregisterTopLevelWindow(getXULWindow(window));
  if (!options || options.close !== false)
    observers.add('quit-application-granted', window.close.bind(window));

  return window;
}
exports.backgroundify = backgroundify;






function serializeFeatures(options) {
  return Object.keys(options).reduce(function(result, name) {
    let value = options[name];

    
    if ((name == 'private' || name == 'chrome'))
      return result + ((value === true) ? ',' + name : '');

    return result + ',' + name + '=' +
           (value === true ? 'yes' : value === false ? 'no' : value);
  }, '').substr(1);
}












function open(uri, options) {
  options = options || {};
  let newWindow = windowWatcher.
    openWindow(options.parent || null,
               uri || URI_BROWSER,
               options.name || null,
               serializeFeatures(options.features || {}),
               options.args || null);

  return newWindow;
}


exports.open = open;

function onFocus(window) {
  let { resolve, promise } = defer();

  if (isFocused(window)) {
    resolve(window);
  }
  else {
    window.addEventListener("focus", function focusListener() {
      window.removeEventListener("focus", focusListener, true);
      resolve(window);
    }, true);
  }

  return promise;
}
exports.onFocus = onFocus;

function isFocused(window) {
  const FM = Cc["@mozilla.org/focus-manager;1"].
                getService(Ci.nsIFocusManager);

  let childTargetWindow = {};
  FM.getFocusedElementForWindow(window, true, childTargetWindow);
  childTargetWindow = childTargetWindow.value;

  let focusedChildWindow = {};
  if (FM.activeWindow) {
    FM.getFocusedElementForWindow(FM.activeWindow, true, focusedChildWindow);
    focusedChildWindow = focusedChildWindow.value;
  }

  return (focusedChildWindow === childTargetWindow);
}
exports.isFocused = isFocused;







function openDialog(options) {
  options = options || {};

  let features = options.features || FEATURES;
  if (!!options.private &&
      !array.has(features.toLowerCase().split(','), 'private')) {
    features = features.split(',').concat('private').join(',');
  }

  let browser = getMostRecentBrowserWindow();

  
  if (!browser)
    return undefined;

  let newWindow = browser.openDialog.apply(
      browser,
      array.flatten([
        options.url || URI_BROWSER,
        options.name || NAME,
        features,
        options.args || null
      ])
  );

  return newWindow;
}
exports.openDialog = openDialog;





function windows(type, options) {
  options = options || {};
  let list = [];
  let winEnum = WM.getEnumerator(type);
  while (winEnum.hasMoreElements()) {
    let window = winEnum.getNext().QueryInterface(Ci.nsIDOMWindow);
    
    
    if (options.includePrivate || !isWindowPrivate(window)) {
      list.push(window);
    }
  }
  return list;
}
exports.windows = windows;







function isDocumentLoaded(window) {
  return window.document.readyState == "complete";
}
exports.isDocumentLoaded = isDocumentLoaded;

function isBrowser(window) {
  try {
    return window.document.documentElement.getAttribute("windowtype") === BROWSER;
  }
  catch (e) {}
  return false;
};
exports.isBrowser = isBrowser;

function getWindowTitle(window) {
  return window && window.document ? window.document.title : null;
}
exports.getWindowTitle = getWindowTitle;

function isXULBrowser(window) {
  return !!(isBrowser(window) && window.XULBrowserWindow);
}
exports.isXULBrowser = isXULBrowser;




function getFocusedWindow() {
  let window = WM.getMostRecentWindow(BROWSER);

  return window ? window.document.commandDispatcher.focusedWindow : null;
}
exports.getFocusedWindow = getFocusedWindow;




function getFocusedElement() {
  let window = WM.getMostRecentWindow(BROWSER);

  return window ? window.document.commandDispatcher.focusedElement : null;
}
exports.getFocusedElement = getFocusedElement;

function getFrames(window) {
  return Array.slice(window.frames).reduce(function(frames, frame) {
    return frames.concat(frame, getFrames(frame))
  }, [])
}
exports.getFrames = getFrames;
