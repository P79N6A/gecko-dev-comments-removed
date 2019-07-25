





































var EXPORTED_SYMBOLS = ["mozmill"];

var mozmill = Components.utils.import('resource://mozmill/modules/mozmill.js');
var utils = {}; Components.utils.import('resource://mozmill/modules/utils.js', utils);
var enumerator = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                   .getService(Components.interfaces.nsIWindowMediator)
                   .getEnumerator("");



while(enumerator.hasMoreElements()) {
  var win = enumerator.getNext();
  win.documentLoaded = true;
  
  try {
    win.content.documentLoaded = true;
  } catch(e){}

  win.addEventListener("DOMContentLoaded", function(event) {
    win.documentLoaded = true;
    
    
    
    try {
      win.content.addEventListener("load", function(event) {
        win.content.documentLoaded = true;
      }, false);
      win.content.addEventListener("beforeunload", function(event) {
        win.content.documentLoaded = false;
      }, false);
    } catch(err){}

  }, false);
  
};


var observer = {
  observe: function(subject,topic,data){
      subject.addEventListener("DOMContentLoaded", function(event) {
        subject.documentLoaded = true;
        
        
        
        try {
          subject.content.addEventListener("load", function(event) {
            subject.content.documentLoaded = true;
          }, false);
          subject.content.addEventListener("beforeunload", function(event) {
            subject.content.documentLoaded = false;
          }, false);
        } catch(err){}

      }, false);  
  }
};

var observerService =
  Components.classes["@mozilla.org/observer-service;1"]
    .getService(Components.interfaces.nsIObserverService);

observerService.addObserver(observer, "toplevel-window-ready", false);

