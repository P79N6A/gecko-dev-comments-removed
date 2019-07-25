const EXPORTED_SYMBOLS = ['CookieEngine', 'CookieTracker', 'CookieStore'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/engines.js");
Cu.import("resource://weave/syncCores.js");
Cu.import("resource://weave/stores.js");
Cu.import("resource://weave/trackers.js");

function CookieEngine(pbeId) {
  this._init(pbeId);
}
CookieEngine.prototype = {
  get name() { return "cookies"; },
  get logName() { return "CookieEngine"; },
  get serverPrefix() { return "user-data/cookies/"; },

  __core: null,
  get _core() {
    if (!this.__core)
      this.__core = new CookieSyncCore();
    return this.__core;
  },

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
CookieEngine.prototype.__proto__ = new Engine();

function CookieSyncCore() {
  this._init();
}
CookieSyncCore.prototype = {
  _logName: "CookieSync",

  __cookieManager: null,
  get _cookieManager() {
    if (!this.__cookieManager)
      this.__cookieManager = Cc["@mozilla.org/cookiemanager;1"].
                             getService(Ci.nsICookieManager2);
    

    return this.__cookieManager;
  },


  _itemExists: function CSC__itemExists(GUID) {
    





    



    let cookieArray = GUID.split( ":" );
    let cookieHost = cookieArray[0];
    let cookiePath = cookieArray[1];
    let cookieName = cookieArray[2];

    



    let enumerator = this._cookieManager.enumerator;
    while (enumerator.hasMoreElements())
      {
	let aCookie = enumerator.getNext();
	if (aCookie.host == cookieHost &&
	    aCookie.path == cookiePath &&
	    aCookie.name == cookieName ) {
	  return true;
	}
      }
    return false;
    





  },

  _commandLike: function CSC_commandLike(a, b) {
    





    return false;
  }
};
CookieSyncCore.prototype.__proto__ = new SyncCore();

function CookieStore( cookieManagerStub ) {
  



  this._init();
  this._cookieManagerStub = cookieManagerStub;
}
CookieStore.prototype = {
  _logName: "CookieStore",


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  __cookieManager: null,
  get _cookieManager() {
    if ( this._cookieManagerStub != undefined ) {
      return this._cookieManagerStub;
    }
    
    if (!this.__cookieManager)
      this.__cookieManager = Cc["@mozilla.org/cookiemanager;1"].
                             getService(Ci.nsICookieManager2);
    
    
    return this.__cookieManager
  },

  _createCommand: function CookieStore__createCommand(command) {
    


    this._log.info("CookieStore got createCommand: " + command );
    
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
    




    this._log.info("CookieStore got removeCommand: " + command );

    



    this._cookieManager.remove( command.data.host,
				command.data.name,
				command.data.path,
				false );
  },

  _editCommand: function CookieStore__editCommand(command) {
    

    this._log.info("CookieStore got editCommand: " + command );

    
    var iter = this._cookieManager.enumerator;
    var matchingCookie = null;
    while (iter.hasMoreElements()){
      let cookie = iter.getNext();
      if (cookie.QueryInterface( Ci.nsICookie ) ){
        
	let key = cookie.host + ":" + cookie.path + ":" + cookie.name;
	if (key == command.GUID) {
	  matchingCookie = cookie;
	  break;
	}
      }
    }
    
    for (var key in command.data) {
      
      matchingCookie[ key ] = command.data[ key ]
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
    while (iter.hasMoreElements()){
      var cookie = iter.getNext();
      if (cookie.QueryInterface( Ci.nsICookie )){
	
	
	if ( cookie.isSession ) {
	  
	  continue;
	}

	let key = cookie.host + ":" + cookie.path + ":" + cookie.name;
	items[ key ] = { parentGUID: '',
			 name: cookie.name,
			 value: cookie.value,
			 isDomain: cookie.isDomain,
			 host: cookie.host,
			 path: cookie.path,
			 isSecure: cookie.isSecure,
			 
			 rawHost: cookie.rawHost,
			 isSession: cookie.isSession,
			 expiry: cookie.expiry,
			 isHttpOnly: cookie.isHttpOnly }

	



      }
    }
    return items;
  },

  wipe: function CookieStore_wipe() {
    



    this._cookieManager.removeAll()
  },

  resetGUIDs: function CookieStore_resetGUIDs() {
    




  }
};
CookieStore.prototype.__proto__ = new Store();

function CookieTracker() {
  this._init();
}
CookieTracker.prototype = {
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
CookieTracker.prototype.__proto__ = new Tracker();
