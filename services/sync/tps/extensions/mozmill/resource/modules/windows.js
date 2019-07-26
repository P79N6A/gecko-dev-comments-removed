



var EXPORTED_SYMBOLS = ["init", "map"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;


var utils = {}; Cu.import('resource://mozmill/stdlib/utils.js', utils);

var uuidgen = Cc["@mozilla.org/uuid-generator;1"].getService(Ci.nsIUUIDGenerator);





var map = {
  _windows : { },

  






  contains : function (aWindowId) {
    return (aWindowId in this._windows);
  },

  








  getValue : function (aWindowId, aProperty) {
    if (!this.contains(aWindowId)) {
      return undefined;
    } else {
      var win = this._windows[aWindowId];

      return (aProperty in win) ? win[aProperty]
                                : undefined;
    }
  },

  





  remove : function (aWindowId) {
    if (this.contains(aWindowId)) {
      delete this._windows[aWindowId];
    }

    
  },

  









  update : function (aWindowId, aProperty, aValue) {
    if (!this.contains(aWindowId)) {
      this._windows[aWindowId] = { };
    }

    this._windows[aWindowId][aProperty] = aValue;
    
  },

  






  updatePageLoadStatus : function (aId, aIsLoaded) {
    this.update(aId, "loaded", aIsLoaded);

    var uuid = this.getValue(aId, "id_load_in_transition");

    
    if (!uuid || !aIsLoaded) {
      uuid = uuidgen.generateUUID();
      this.update(aId, "id_load_in_transition", uuid);
    }

    
  },

  








  hasPageLoaded : function (aId) {
    var load_current = this.getValue(aId, "id_load_in_transition");
    var load_handled = this.getValue(aId, "id_load_handled");

    var isLoaded = this.contains(aId) && this.getValue(aId, "loaded") &&
                   (load_current !== load_handled);

    if (isLoaded) {
      
      this.update(aId, "id_load_handled", load_current);
    }

    

    return isLoaded;
  }
};



var windowReadyObserver = {
  observe: function (aSubject, aTopic, aData) {
    
    
    var win = utils.getChromeWindow(aSubject);

    
    
    attachEventListeners(win);
  }
};



var windowCloseObserver = {
  observe: function (aSubject, aTopic, aData) {
    var id = utils.getWindowId(aSubject);
    

    map.remove(id);
  }
};




var enterLeavePrivateBrowsingObserver = {
  observe: function (aSubject, aTopic, aData) {
    handleAttachEventListeners();
  }
};







function attachEventListeners(aWindow) {
  
  var pageShowHandler = function (aEvent) {
    var doc = aEvent.originalTarget;

    
    
    if ("defaultView" in doc) {
      var id = utils.getWindowId(doc.defaultView);
      
      map.updatePageLoadStatus(id, true);
    }

    
    aWindow.addEventListener("beforeunload", beforeUnloadHandler, true);
    aWindow.addEventListener("pagehide", pageHideHandler, true);
  };

  var DOMContentLoadedHandler = function (aEvent) {
    var doc = aEvent.originalTarget;

    
    if ("defaultView" in doc) {
      var id = utils.getWindowId(doc.defaultView);
      

      
      var errorRegex = /about:.+(error)|(blocked)\?/;
      if (errorRegex.exec(doc.baseURI)) {
        
        utils.sleep(1000);

        map.updatePageLoadStatus(id, true);
      }

      
      aWindow.addEventListener("beforeunload", beforeUnloadHandler, true);
    }
  };

  
  
  var beforeUnloadHandler = function (aEvent) {
    var doc = aEvent.originalTarget;

    
    if ("defaultView" in doc) {
      var id = utils.getWindowId(doc.defaultView);
      
      map.updatePageLoadStatus(id, false);
    }

    aWindow.removeEventListener("beforeunload", beforeUnloadHandler, true);
  };

  var pageHideHandler = function (aEvent) {
    var doc = aEvent.originalTarget;

    
    if ("defaultView" in doc) {
      var id = utils.getWindowId(doc.defaultView);
      
      map.updatePageLoadStatus(id, false);
    }
    
    
    if (aEvent.persisted)
      aWindow.removeEventListener("beforeunload", beforeUnloadHandler, true);
  };

  var onWindowLoaded = function (aEvent) {
    var id = utils.getWindowId(aWindow);
    

    map.update(id, "loaded", true);

    
    
    
    
    aWindow.addEventListener("DOMContentLoaded", DOMContentLoadedHandler, true);

    
    aWindow.addEventListener("pageshow", pageShowHandler, true);

    
    aWindow.addEventListener("pagehide", pageHideHandler, true);
  };

  
  
  if (aWindow.document.readyState === 'complete') {
    onWindowLoaded();
  } else {
    aWindow.addEventListener("load", onWindowLoaded, false);
  }
}


function handleAttachEventListeners() {
  var enumerator = Cc["@mozilla.org/appshell/window-mediator;1"].
                   getService(Ci.nsIWindowMediator).getEnumerator("");
  while (enumerator.hasMoreElements()) {
    var win = enumerator.getNext();
    attachEventListeners(win);
  }
}

function init() {
  
  var observerService = Cc["@mozilla.org/observer-service;1"].
                        getService(Ci.nsIObserverService);
  observerService.addObserver(windowReadyObserver, "toplevel-window-ready", false);
  observerService.addObserver(windowCloseObserver, "outer-window-destroyed", false);
  observerService.addObserver(enterLeavePrivateBrowsingObserver, "private-browsing", false);

  handleAttachEventListeners();
}
