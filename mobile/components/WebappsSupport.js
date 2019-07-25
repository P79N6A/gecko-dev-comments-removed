




































const Cu = Components.utils; 
const Cc = Components.classes;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

const DB_VERSION = 1;

function WebappsSupport() {
  this.init();
}

WebappsSupport.prototype = {
  db: null,
  
  init: function() {
    let file =  Cc["@mozilla.org/file/directory_service;1"].getService(Ci.nsIProperties).get("ProfD", Ci.nsIFile);
    file.append("webapps.sqlite");
    this.db = Services.storage.openDatabase(file);
    let version = this.db.schemaVersion;
    
    if (version == 0) {
      this.db.executeSimpleSQL("CREATE TABLE webapps (title TEXT, uri TEXT PRIMARY KEY, icon TEXT)");
      this.db.schemaVersion = DB_VERSION;
    }

    XPCOMUtils.defineLazyGetter(this, "_installQuery", function() {
      return this.db.createAsyncStatement("INSERT INTO webapps (title, uri, icon) VALUES(:title, :uri, :icon)");
    });

    XPCOMUtils.defineLazyGetter(this, "_findQuery", function() {
      return this.db.createStatement("SELECT uri FROM webapps where uri = :uri");
    });

    Services.obs.addObserver(this, "quit-application-granted", false);
  },
 
  
  installApplication: function(aTitle, aURI, aIconURI, aIconData) {
    let stmt = this._installQuery;
    stmt.params.title = aTitle;
    stmt.params.uri = aURI;
    stmt.params.icon = aIconData;
    stmt.executeAsync();
  },
 
  isApplicationInstalled: function(aURI) {
    let stmt = this._findQuery;
    let found = false;
    try {
      stmt.params.uri = aURI;
      found = stmt.executeStep();
    } finally {
      stmt.reset();
    }
    return found;
  },

  
  observe: function(aSubject, aTopic, aData) {
    Services.obs.removeObserver(this, "quit-application-granted");

    
    let stmts = [
      "_installQuery",
      "_findQuery"
    ];
    for (let i = 0; i < stmts.length; i++) {
      
      
      if (Object.getOwnPropertyDescriptor(this, stmts[i]).value !== undefined) {
        this[stmts[i]].finalize();
      }
    }

    this.db.asyncClose();
  },

  
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIWebappsSupport]),

  
  classID: Components.ID("{cb1107c1-1e15-4f11-99c8-27b9ec221a2a}")
};

const NSGetFactory = XPCOMUtils.generateNSGetFactory([WebappsSupport]);

