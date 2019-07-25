






































var frame = {};   Components.utils.import('resource://mozmill/modules/frame.js', frame);





function ConsoleListener() {
 this.register();
}
ConsoleListener.prototype = {
 observe: function(aMessage) {
   var msg = aMessage.message;
   var re = /^\[.*Error:.*(chrome|resource):\/\/.*/i;
   if (msg.match(re)) {
     frame.events.fail(aMessage);
   }
 },
 QueryInterface: function (iid) {
	if (!iid.equals(Components.interfaces.nsIConsoleListener) && !iid.equals(Components.interfaces.nsISupports)) {
		throw Components.results.NS_ERROR_NO_INTERFACE;
   }
   return this;
 },
 register: function() {
   var aConsoleService = Components.classes["@mozilla.org/consoleservice;1"]
                              .getService(Components.interfaces.nsIConsoleService);
   aConsoleService.registerListener(this);
 },
 unregister: function() {
   var aConsoleService = Components.classes["@mozilla.org/consoleservice;1"]
                              .getService(Components.interfaces.nsIConsoleService);
   aConsoleService.unregisterListener(this);
 }
}


var consoleListener = new ConsoleListener();

var EXPORTED_SYMBOLS = ["mozmill"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

var mozmill = Cu.import('resource://mozmill/modules/mozmill.js');


var windowObserver = {
  observe: function(subject, topic, data) {
    attachEventListeners(subject);
  }
};




function attachEventListeners(window) {
  
  function pageShowHandler(event) {
    var doc = event.originalTarget;
    var tab = window.gBrowser.getBrowserForDocument(doc);

    if (tab) {
      
      tab.mozmillDocumentLoaded = true;
    } else {
      
      doc.defaultView.mozmillDocumentLoaded = true;
    }

    
    window.gBrowser.addEventListener("beforeunload", beforeUnloadHandler, true);
    window.gBrowser.addEventListener("pagehide", pageHideHandler, true);
  };

  var DOMContentLoadedHandler = function(event) {
    var errorRegex = /about:.+(error)|(blocked)\?/;
    if (errorRegex.exec(event.target.baseURI)) {
      
      mozmill.utils.sleep(1000);

      var tab = window.gBrowser.getBrowserForDocument(event.target);
      if (tab)
        tab.mozmillDocumentLoaded = true;

      
      window.gBrowser.addEventListener("beforeunload", beforeUnloadHandler, true);
    }
  };

  
  
  function beforeUnloadHandler(event) {
    var doc = event.originalTarget;
    var tab = window.gBrowser.getBrowserForDocument(event.target);

    if (tab) {
      
      tab.mozmillDocumentLoaded = false;
    } else {
      
      doc.defaultView.mozmillDocumentLoaded = false;
    }

    window.gBrowser.removeEventListener("beforeunload", beforeUnloadHandler, true);
  };

  var pageHideHandler = function(event) {
    
    
    if (event.persisted) {
      var doc = event.originalTarget;
      var tab = window.gBrowser.getBrowserForDocument(event.target);

      if (tab) {
        
        tab.mozmillDocumentLoaded = false;
      } else {
        
        doc.defaultView.mozmillDocumentLoaded = false;
      }

      window.gBrowser.removeEventListener("beforeunload", beforeUnloadHandler, true);
    }

  };

  
  window.addEventListener("load", function(event) {
    window.mozmillDocumentLoaded = true;


    if (window.gBrowser) {
      
      window.gBrowser.addEventListener("pageshow", pageShowHandler, true);

      
      
      
      
      window.gBrowser.addEventListener("DOMContentLoaded", DOMContentLoadedHandler, true);

      
      window.gBrowser.addEventListener("pagehide", pageHideHandler, true);
    }
  }, false);
}




function initialize() {
  
  var observerService = Cc["@mozilla.org/observer-service;1"].
                        getService(Ci.nsIObserverService);
  observerService.addObserver(windowObserver, "toplevel-window-ready", false);

  
  var enumerator = Cc["@mozilla.org/appshell/window-mediator;1"].
                   getService(Ci.nsIWindowMediator).getEnumerator("");
  while (enumerator.hasMoreElements()) {
    var win = enumerator.getNext();
    attachEventListeners(win);

    
    
    
    win.mozmillDocumentLoaded = true;
  };
}

initialize();

