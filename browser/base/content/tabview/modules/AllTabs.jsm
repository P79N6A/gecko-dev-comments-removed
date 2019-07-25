




































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

let EXPORTED_SYMBOLS = ["Tabs"];

let Tabs = let (T = {
  
  
  

  





  get allTabs() {
    
    return Array.concat.apply({}, T.allBrowsers.map(function(browser) {
      return Array.slice(browser.gBrowser.tabs);
    }));
  },

  











  get onChange() T.makeBind("onChange"),

  










  get onClose() T.makeBind("onClose"),

  










  get onMove() T.makeBind("onMove"),

  










  get onOpen() T.makeBind("onOpen"),

  










  get onSelect() T.makeBind("onSelect"),

  
  
  

  init: function init() {
    
    T.init = function() T;

    
    T.allBrowsers.forEach(T.registerBrowser);
    Services.obs.addObserver(T, "domwindowopened", false);

    return T;
  },

  get allBrowsers() {
    let browsers = [];
    let windows = Services.wm.getEnumerator("navigator:browser");
    while (windows.hasMoreElements())
      browsers.push(windows.getNext());
    return browsers;
  },

  eventMap: {
    TabAttrModified: "onChange",
    TabClose: "onClose",
    TabMove: "onMove",
    TabOpen: "onOpen",
    TabSelect: "onSelect",
  },

  registerBrowser: function registerBrowser(browser) {
    
    [i for (i in Iterator(T.eventMap))].forEach(function([tabEvent, topic]) {
      browser.addEventListener(tabEvent, function(event) {
        T.trigger(topic, event.originalTarget, event);
      }, true);
    });
  },

  listeners: {},

  makeBind: function makeBind(topic) {
    delete T[topic];
    T.listeners[topic] = [];

    
    T[topic] = function bind(callback) {
      T.listeners[topic].push(callback);
    };

    
    T[topic].unbind = function unbind(callback) {
      let index = T.listeners[topic].indexOf(callback);
      if (index != -1)
        T.listeners[topic].splice(index, 1);
    };

    return T[topic];
  },

  trigger: function trigger(topic, tab, event) {
    
    let listeners = T.listeners[topic];
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
            T.registerBrowser(subject);
        }, false);
        break;
    }
  },

  
  
  

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver]),
}) T.init();
