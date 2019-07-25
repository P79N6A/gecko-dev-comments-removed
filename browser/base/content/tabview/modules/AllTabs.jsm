




































const Cu = Components.utils;
Cu.import("resource://gre/modules/Services.jsm");

let EXPORTED_SYMBOLS = ["AllTabs"];

let AllTabs = {
  
  
  
  toString: function AllTabs_toString() {
    return "[AllTabs]";
  },

  





  get tabs() {
    
    return Array.concat.apply(null, browserWindows.map(function(browserWindow) {
      return Array.filter(browserWindow.gBrowser.tabs, function (tab) !tab.closing);
    }));
  },

  










  register: function register(eventName, callback) {
    
    let listeners = eventListeners[events[eventName]];
    if (listeners)
      listeners.push(callback);
    else
      eventListeners[events[eventName]] = [callback];
  },

  









  unregister: function unregister(eventName, callback) {
    
    let listeners = eventListeners[events[eventName]];
    if (!listeners)
      return;

    
    let index = listeners.indexOf(callback);
    if (index == -1)
      return;

    listeners.splice(index, 1);
  }
};

__defineGetter__("browserWindows", function browserWindows() {
  let browserWindows = [];
  let windows = Services.wm.getEnumerator("navigator:browser");
  while (windows.hasMoreElements())
    browserWindows.push(windows.getNext());
  return browserWindows;
});

let events = {
  attrModified: "TabAttrModified",
  close:        "TabClose",
  move:         "TabMove",
  open:         "TabOpen",
  select:       "TabSelect",
  pinned:       "TabPinned",
  unpinned:     "TabUnpinned"
};
let eventListeners = {};

function registerBrowserWindow(browserWindow) {
  for each (let event in events)
    browserWindow.addEventListener(event, tabEventListener, true);

  browserWindow.addEventListener("unload", unregisterBrowserWindow, false);
}

function unregisterBrowserWindow(unloadEvent) {
  let browserWindow = unloadEvent.currentTarget;

  for each (let event in events)
    browserWindow.removeEventListener(event, tabEventListener, true);

  browserWindow.removeEventListener("unload", unregisterBrowserWindow, false);
}

function tabEventListener(event) {
  
  let listeners = eventListeners[event.type];
  if (!listeners)
    return;

  let tab = event.target;

  
  listeners.slice().forEach(function (callback) {
    try {
      callback(tab, event);
    }
    
    catch (ex) {
      Cu.reportError(ex);
    }
  });
}

function observer(subject, topic, data) {
  switch (topic) {
    case "domwindowopened":
      subject.addEventListener("load", function() {
        subject.removeEventListener("load", arguments.callee, false);

        
        let doc = subject.document.documentElement;
        if (doc.getAttribute("windowtype") == "navigator:browser")
          registerBrowserWindow(subject);
      }, false);
      break;
  }
}


browserWindows.forEach(registerBrowserWindow);
Services.obs.addObserver(observer, "domwindowopened", false);
