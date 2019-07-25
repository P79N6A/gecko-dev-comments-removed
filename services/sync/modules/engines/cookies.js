



































const EXPORTED_SYMBOLS = ['CookieEngine', 'CookieTracker', 'CookieStore'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/engines.js");
Cu.import("resource://weave/stores.js");
Cu.import("resource://weave/trackers.js");

function CookieEngine(pbeId) {
  this._init(pbeId);
}
CookieEngine.prototype = {
  get enabled() null, 
  __proto__: SyncEngine.prototype,

  get name() { return "cookies"; },
  get displayName() { return "Cookies"; },
  get logName() { return "CookieEngine"; },
  get serverPrefix() { return "user-data/cookies/"; },

  __store: null,
  get _store() {
    if (!this.__store)
      this.__store = new CookieStore();
    return this.__store;
  },

  __tracker: null,
  get _tracker() {
    if (!this.__tracker)
      this.__tracker = new CookieTracker();
    return this.__tracker;
  }
};

function CookieStore( cookieManagerStub ) {
  
  return;
  



  this._init();
  this._cookieManagerStub = cookieManagerStub;
}
CookieStore.prototype = {
  __proto__: Store.prototype,
  _logName: "CookieStore",
  _lookup: null,

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  __cookieManager: null,
  get _cookieManager() {
    if ( this._cookieManagerStub != undefined ) {
      return this._cookieManagerStub;
    }
    
    if (!this.__cookieManager)
      this.__cookieManager = Cc["@mozilla.org/cookiemanager;1"].
                             getService(Ci.nsICookieManager2);
    
    
    return this.__cookieManager;
  },

  _createCommand: function CookieStore__createCommand(command) {
    


    this._log.debug("CookieStore got create command for: " + command.GUID);

    
    if ( !command.data.isSession ) {
      
      this._cookieManager.add( command.data.host,
			       command.data.path,
			       command.data.name,
			       command.data.value,
			       command.data.isSecure,
			       command.data.isHttpOnly,
			       command.data.isSession,
			       command.data.expiry );
    }
  },

  _removeCommand: function CookieStore__removeCommand(command) {
    




    if (!(command.GUID in this._lookup)) {
      this._log.warn("Warning! Remove command for unknown item: " + command.GUID);
      return;
    }

    this._log.debug("CookieStore got remove command for: " + command.GUID);

    



    this._cookieManager.remove(this._lookup[command.GUID].host,
                               this._lookup[command.GUID].name,
                               this._lookup[command.GUID].path,
                               false);
  },

  _editCommand: function CookieStore__editCommand(command) {
    


    if (!(command.GUID in this._lookup)) {
      this._log.warn("Warning! Edit command for unknown item: " + command.GUID);
      return;
    }

    this._log.debug("CookieStore got edit command for: " + command.GUID);

    
    var iter = this._cookieManager.enumerator;
    var matchingCookie = null;
    while (iter.hasMoreElements()){
      let cookie = iter.getNext();
      if (cookie.QueryInterface( Ci.nsICookie2 ) ){
        
	let key = cookie.host + ":" + cookie.path + ":" + cookie.name;
	if (key == command.GUID) {
	  matchingCookie = cookie;
	  break;
	}
      }
    }
    
    for (var key in command.data) {
      
      matchingCookie[ key ] = command.data[ key ];
    }
    
    this._cookieManager.remove( matchingCookie.host,
				matchingCookie.name,
				matchingCookie.path,
				false );

    
    if ( !command.data.isSession ) {
      
      this._cookieManager.add( matchingCookie.host,
			       matchingCookie.path,
			       matchingCookie.name,
			       matchingCookie.value,
			       matchingCookie.isSecure,
			       matchingCookie.isHttpOnly,
			       matchingCookie.isSession,
			       matchingCookie.expiry );
    }

    
    
  },

  wrap: function CookieStore_wrap() {
    


    let items = {};
    var iter = this._cookieManager.enumerator;
    while (iter.hasMoreElements()) {
      var cookie = iter.getNext();
      if (cookie.QueryInterface( Ci.nsICookie2 )) {
	      
	      
	      if ( cookie.isSession ) {
	        
	        continue;
	      }

	      let key = cookie.host + ":" + cookie.path + ":" + cookie.name;
	      items[ key ] = { parentid: '',
			       name: cookie.name,
			       value: cookie.value,
			       isDomain: cookie.isDomain,
			       host: cookie.host,
			       path: cookie.path,
			       isSecure: cookie.isSecure,
			       
			       rawHost: cookie.rawHost,
			       isSession: cookie.isSession,
			       expiry: cookie.expiry,
			       isHttpOnly: cookie.isHttpOnly };

	      



      }
    }
    this._lookup = items;
    return items;
  },

  wipe: function CookieStore_wipe() {
    



    this._cookieManager.removeAll();
  },

  _resetGUIDs: function CookieStore__resetGUIDs() {
    




  }
};

function CookieTracker() {
  
  return;
  this._init();
}
CookieTracker.prototype = {
  __proto__: Tracker.prototype,
  _logName: "CookieTracker",

  _init: function CT__init() {
    this._log = Log4Moz.Service.getLogger("Service." + this._logName);
    this._score = 0;
    


    let observerService = Cc["@mozilla.org/observer-service;1"].
                          getService(Ci.nsIObserverService);
    observerService.addObserver( this, 'cookie-changed', false );
  },

  
  observe: function ( aSubject, aTopic, aData ) {
    



    var newCookie = aSubject.QueryInterface( Ci.nsICookie2 );
    if ( newCookie ) {
      if ( !newCookie.isSession ) {
	

	this._score += 10;
      }
    }
  }
}
