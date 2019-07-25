




































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/Services.jsm");

let EXPORTED_SYMBOLS = ["AllTabs"];

let AllTabs = {
  
  
  

  





  get tabs() {
    
    let browserWindows = AllTabs.allBrowserWindows;
    return Array.concat.apply({}, browserWindows.map(function(browserWindow) {
      return Array.slice(browserWindow.gBrowser.tabs);
    }));
  },

  











  get onChange() AllTabs.makeBind("onChange"),

  










  get onClose() AllTabs.makeBind("onClose"),

  










  get onMove() AllTabs.makeBind("onMove"),

  










  get onOpen() AllTabs.makeBind("onOpen"),

  










  get onSelect() AllTabs.makeBind("onSelect"),

  
  
  

  get allBrowserWindows() {
    let browserWindows = [];
    let windows = Services.wm.getEnumerator("navigator:browser");
    while (windows.hasMoreElements())
      browserWindows.push(windows.getNext());
    return browserWindows;
  },

  eventMap: {
    TabAttrModified: "onChange",
    TabClose: "onClose",
    TabMove: "onMove",
    TabOpen: "onOpen",
    TabSelect: "onSelect",
  },

  registerBrowserWindow: function registerBrowserWindow(browserWindow) {
    
    [i for (i in Iterator(AllTabs.eventMap))].forEach(function([tabEvent, topic]) {
      browserWindow.addEventListener(tabEvent, function(event) {
        AllTabs.trigger(topic, event.originalTarget, event);
      }, true);
    });
  },

  listeners: {},

  makeBind: function makeBind(topic) {
    delete AllTabs[topic];
    AllTabs.listeners[topic] = [];

    
    AllTabs[topic] = function bind(callback) {
      AllTabs.listeners[topic].push(callback);
    };

    
    AllTabs[topic].unbind = function unbind(callback) {
      let index = AllTabs.listeners[topic].indexOf(callback);
      if (index != -1)
        AllTabs.listeners[topic].splice(index, 1);
    };

    return AllTabs[topic];
  },

  trigger: function trigger(topic, tab, event) {
    
    let listeners = AllTabs.listeners[topic];
    if (listeners == null)
      return;

    
    listeners.slice().forEach(function(callback) {
      try {
        callback.call(tab, event);
      }
      
      catch(ex) {}
    });
  },

  
  
  

  observe: function observe(subject, topic, data) {
    switch (topic) {
      case "domwindowopened":
        subject.addEventListener("load", function() {
          subject.removeEventListener("load", arguments.callee, false);

          
          let doc = subject.document.documentElement;
          if (doc.getAttribute("windowtype") == "navigator:browser")
            AllTabs.registerBrowserWindow(subject);
        }, false);
        break;
    }
  },
};


AllTabs.allBrowserWindows.forEach(AllTabs.registerBrowserWindow);
Services.obs.addObserver(AllTabs, "domwindowopened", false);
