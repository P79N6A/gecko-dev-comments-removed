




































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/Services.jsm");

let EXPORTED_SYMBOLS = ["AllTabs"];

let AllTabs = {
  





  get tabs() {
    
    return Array.concat.apply(null, browserWindows.map(function(browserWindow) {
      return Array.slice(browserWindow.gBrowser.tabs);
    }));
  },

  










  register: function register(eventName, callback) {
    
    let listeners = eventListeners[eventName];
    if (listeners == null)
      eventListeners[eventName] = [callback];
    else
      listeners.push(callback);
  },

  









  unregister: function unregister(eventName, callback) {
    
    let listeners = eventListeners[eventName];
    if (listeners == null)
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

let events = ["attrModified", "close", "move", "open", "select"];
let eventListeners = {};

function registerBrowserWindow(browserWindow) {
  events.forEach(function(eventName) {
    let tabEvent = "Tab" + eventName[0].toUpperCase() + eventName.slice(1);
    browserWindow.addEventListener(tabEvent, function(event) {
      
      let listeners = eventListeners[eventName];
      if (listeners == null)
        return;

      let tab = event.originalTarget;

      
      listeners.slice().forEach(function(callback) {
        try {
          callback(tab, event);
        }
        
        catch(ex) {}
      });
    }, true);
  });
}

let observer = {
  observe: function observe(subject, topic, data) {
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
};


browserWindows.forEach(registerBrowserWindow);
Services.obs.addObserver(observer, "domwindowopened", false);
