






































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
  window.addEventListener("load", function(event) {
    window.documentLoaded = true;
 
    if (window.gBrowser) {
      
      window.gBrowser.addEventListener("load", function(event) {
        event.target.documentLoaded = true;
      }, true);
 
      
      
      
      
      window.gBrowser.addEventListener("DOMContentLoaded", function(event) {
        var errorRegex = /about:.+(error)|(blocked)\?/;
        if (errorRegex.exec(event.target.baseURI)) {
          
          mozmill.utils.sleep(1000);
          event.target.documentLoaded = true;
        }
      }, true);
  
      
      window.gBrowser.addEventListener("beforeunload", function(event) {
        event.target.documentLoaded = false;
      }, true);
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

    
    
    
    win.documentLoaded = true;
  };
}

initialize();

