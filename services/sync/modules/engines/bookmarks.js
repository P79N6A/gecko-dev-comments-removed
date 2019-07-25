





































const EXPORTED_SYMBOLS = ['BookmarksEngine', 'BookmarksSharingManager'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;


const INCOMING_SHARED_ANNO = "weave/shared-incoming";
const OUTGOING_SHARED_ANNO = "weave/shared-outgoing";
const SERVER_PATH_ANNO = "weave/shared-server-path";

const KEYRING_FILE_NAME = "keyring";
const SHARED_BOOKMARK_FILE_NAME = "shared_bookmarks";

const INCOMING_SHARE_ROOT_ANNO = "weave/mounted-shares-folder";
const INCOMING_SHARE_ROOT_NAME = "Shared Folders";

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/crypto.js");
Cu.import("resource://weave/async.js");
Cu.import("resource://weave/engines.js");
Cu.import("resource://weave/syncCores.js");
Cu.import("resource://weave/stores.js");
Cu.import("resource://weave/trackers.js");
Cu.import("resource://weave/identity.js");
Cu.import("resource://weave/xmpp/xmppClient.js");
Cu.import("resource://weave/notifications.js");
Cu.import("resource://weave/sharing.js");
Cu.import("resource://weave/resource.js");

Function.prototype.async = Async.sugar;

function BookmarksSharingManager(engine) {
  this._init(engine);
}
BookmarksSharingManager.prototype = {
  __annoSvc: null,
  get _annoSvc() {
    if (!this.__anoSvc)
      this.__annoSvc = Cc["@mozilla.org/browser/annotation-service;1"].
        getService(Ci.nsIAnnotationService);
    return this.__annoSvc;
  },

  __bms: null,
  get _bms() {
    if (!this.__bms)
      this.__bms = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
        getService(Ci.nsINavBookmarksService);
    return this.__bms;
  },

  __myUsername: null,
  get _myUsername() {
    if (!this.__myUsername)
      this.__myUsername = ID.get('WeaveID').username;
    return this.__myUsername;
  },

  _init: function SharingManager__init(engine) {
    this._engine = engine;
    this._log = Log4Moz.repository.getLogger("Bookmark Share");
    if ( Utils.prefs.getBoolPref( "xmpp.enabled" ) ) {
      this._log.info( "Starting XMPP client for bookmark engine..." );
      this._startXmppClient.async(this);
    }
  },

  _startXmppClient: function BmkSharing__startXmppClient() {
    
    let self = yield;

    
    let serverUrl = Utils.prefs.getCharPref( "xmpp.server.url" );
    let realm = Utils.prefs.getCharPref( "xmpp.server.realm" );

    

    let clientName = this._myUsername;
    let clientPassword = ID.get('WeaveID').password;

    let transport = new HTTPPollingTransport( serverUrl, false, 15000 );
    let auth = new PlainAuthenticator();
    


    this._xmppClient = new XmppClient( clientName,
                                       realm,
                                       clientPassword,
				       transport,
                                       auth );
    let bmkSharing = this;
    let messageHandler = {
      handle: function ( messageText, from ) {
        













 	let words = messageText.split(" ");
	let commandWord = words[0];
	let serverPath = words[1];
	let directoryName = words.slice(2).join(" ");
        if ( commandWord == "share" ) {
	  bmkSharing._incomingShareOffer(from, serverPath, folderName);
	} else if ( commandWord == "stop" ) {
          bmkSharing._log.info("User " + user + " withdrew " + folderName);
          bmkSharing._stopIncomingShare(user, serverPath, folderName);
	}
      }
    };

    this._xmppClient.registerMessageHandler( messageHandler );
    this._xmppClient.connect( realm, self.cb );
    yield;
    if ( this._xmppClient._connectionStatus == this._xmppClient.FAILED ) {
      this._log.warn( "Weave can't log in to xmpp server: xmpp disabled." );
    } else if ( this._xmppClient._connectionStatus == this._xmppClient.CONNECTED ) {
      this._log.info( "Weave logged into xmpp OK." );
    }
    self.done();
  },

  _incomingShareOffer: function BmkSharing__incomingShareOffer(user,
                                                               serverPath,
                                                               folderName) {
    



    this._log.info("User " + user + " offered to share folder " + folderName);

    let bmkSharing = this;
    let acceptButton = new NotificationButton(
      "Accept Share",
      "a",
      function() {
	
	bmkSharing._log.info("Accepted bookmark share from " + user);
	bmkSharing._createIncomingShare(user, serverPath, folderName);
	bmkSharing.updateAllIncomingShares();
	return false;
      }
    );
    let rejectButton = new NotificationButton(
      "No Thanks",
      "n",
      function() {return false;}
    );

    let title = "Bookmark Share Offer From " + user;
    let description ="Weave user " + user +
      " is offering to share a bookmark folder called " + folderName +
      " with you. Do you want to accept it?";
    let notification = new Notification(title,
				        description,
                                        null,
                                        Notifications.PRIORITY_INFO,
                                        [acceptButton, rejectButton]
                                       );
    Notifications.add(notification);
  },

  _sendXmppNotification: function BmkSharing__sendXmpp(recipient, cmd, path, name) {
    
    if ( this._xmppClient ) {
      if ( this._xmppClient._connectionStatus == this._xmppClient.CONNECTED ) {
	let msgText = "share " + path + " " + name;
	this._log.debug( "Sending XMPP message: " + msgText );
	this._xmppClient.sendMessage( recipient, msgText );
      } else {
	this._log.warn( "No XMPP connection for share notification." );
      }
    }
  },

  _share: function BmkSharing__share( folderId, username ) {
    
    let ret = false;
    let self = yield;

    


    let folderName = this._bms.getItemTitle(folderId);

    
    this._createOutgoingShare.async( this, self.cb,
				     folderId, folderName, username );
    let serverPath = yield;
    this._updateOutgoingShare.async( this, self.cb, folderId );
    yield;

    

    this._annoSvc.setItemAnnotation(folderId,
                                    OUTGOING_SHARED_ANNO,
                                    username,
                                    0,
                                    this._annoSvc.EXPIRE_NEVER);
    



    
    
    let abspath = "/user/" + this._myUsername + "/" + serverPath;
    this._sendXmppNotification( username, "share", abspath, folderName);


    this._log.info("Shared " + folderName +" with " + username);
    ret = true;
    self.done( ret );
  },

  _stopSharing: function BmkSharing__stopSharing( folderId, username ) {
    let self = yield;
    let folderName = this._bms.getItemTitle(folderId);
    let serverPath = "";

    if (this._annoSvc.itemHasAnnotation(folderId, SERVER_PATH_ANNO)){
      serverPath = this._annoSvc.getItemAnnotation(folderId, SERVER_PATH_ANNO);
    } else {
      this._log.warn("The folder you are de-sharing has no path annotation.");
    }

    




    
    this._stopOutgoingShare.async(this, self.cb, folderId);
    yield;

    
    let abspath = "/user/" + this._myUsername + "/" + serverPath;
    this._sendXmppNotification( username, "stop", abspath, folderName );

    this._log.info("Stopped sharing " + folderName + "with " + username);
    self.done( true );
  },

  




  getNewShares: function BmkSharing_getNewShares(onComplete) {
    this._getNewShares.async(this, onComplete);
  },
  _getNewShares: function BmkSharing__getNewShares() {
    let self = yield;


    let result = yield sharingApi.getShares(self.cb);

		this._log.info("Got Shares: " + result);
		let shares = result.split(',');
		if (shares.length > 1) {
		  this._log.info('Found shares');
		  for (var i = 0; i < shares.length - 1; i++) {
		    let share = shares[i].split(':');
		    let name = share[0];
		    let user = share[1];
		    let path = share[2];
		    this._incomingShareOffer(user, '/user/' + user + '/' + path, name);
		  }
		}
  },

  updateAllIncomingShares: function BmkSharing_updateAllIncoming(onComplete) {
    this._updateAllIncomingShares.async(this, onComplete);
  },
  _updateAllIncomingShares: function BmkSharing__updateAllIncoming() {
    






    let self = yield;
    let mounts = this._engine._store.findIncomingShares();
    for (let i = 0; i < mounts.length; i++) {
      try {
	this._log.trace("Update incoming share from " + mounts[i].serverPath);
        this._updateIncomingShare.async(this, self.cb, mounts[i]);
        yield;
      } catch (e) {
        this._log.warn("Could not sync shared folder from " + mounts[i].userid);
        this._log.trace(Utils.stackTrace(e));
      }
    }
  },

  updateAllOutgoingShares: function BmkSharing_updateAllOutgoing(onComplete) {
    this._updateAllOutgoingShares.async(this, onComplete);
  },
  _updateAllOutgoingShares: function BmkSharing__updateAllOutgoing() {
    let self = yield;
    let shares = this._annoSvc.getItemsWithAnnotation(OUTGOING_SHARED_ANNO,
                                                      {});
    for ( let i=0; i < shares.length; i++ ) {
      


      this._updateOutgoingShare.async(this, self.cb, shares[i]);
      yield;
    }
    self.done();
  },

  _createKeyChain: function BmkSharing__createKeychain(serverPath,
						       myUserName,
						       username){
    




    let self = yield;
    
    
    
    let tmpIdentity = {
                        realm   : "temp ID",
                        bulkKey : null,
                        bulkIV  : null
                      };
    Crypto.randomKeyGen.async(Crypto, self.cb, tmpIdentity);
    yield;
    let bulkKey = tmpIdentity.bulkKey;
    let bulkIV  = tmpIdentity.bulkIV;

    

    let idRSA = ID.get('WeaveCryptoID');
    let userPubKeyFile = new Resource("/user/" + username + "/public/pubkey");
    userPubKeyFile.pushFilter( new JsonFilter() );
    
    
    userPubKeyFile.get(self.cb);
    let userPubKey = yield;
    userPubKey = userPubKey.pubkey;

    

    Crypto.wrapKey.async(Crypto, self.cb, bulkKey, {realm : "tmpWrapID",
						    pubkey: idRSA.pubkey} );
    let encryptedForMe = yield;
    Crypto.wrapKey.async(Crypto, self.cb, bulkKey, {realm : "tmpWrapID",
						    pubkey: userPubKey} );
    let encryptedForYou = yield;
    let keys = {
                 ring   : { },
                 bulkIV : bulkIV
               };
    keys.ring[myUserName] = encryptedForMe;
    keys.ring[username]   = encryptedForYou;

    let keyringFile = new Resource( serverPath + "/" + KEYRING_FILE_NAME );
    keyringFile.pushFilter(new JsonFilter());
    keyringFile.put( self.cb, keys);
    yield;

    self.done();
  },

  _createOutgoingShare: function BmkSharing__createOutgoing(folderId,
							    folderName,
							    username) {
    









    let self = yield;
    this._log.debug("Turning folder " + folderName + " into outgoing share"
		     + " with " + username);

    

    let folderGuid = Utils.makeGUID();

    
    let serverPath = "share/" + folderGuid;



    if (!ret) {
      this._log.error("Can't create remote folder for outgoing share.");
      self.done(false);
    }
    

    

    this._annoSvc.setItemAnnotation(folderId,
                                    SERVER_PATH_ANNO,
                                    serverPath,
                                    0,
                                    this._annoSvc.EXPIRE_NEVER);

    let encryptionTurnedOn = true;
    if (encryptionTurnedOn) {
      yield this._createKeyChain.async(this, self.cb, serverPath,
				       this._myUsername, username);
    }

    


    let result = yield sharingApi.shareWithUsers( serverPath,
						  [username], folderName,
						  self.cb );
		this._log.info(result.errorText);
    
    self.done( serverPath );
  },

  _updateOutgoingShare: function BmkSharing__updateOutgoing(folderId) {
    




    let self = yield;
    
    
    if (!this._annoSvc.itemHasAnnotation(folderId, SERVER_PATH_ANNO)) {
      this._log.warn("Outgoing share is invalid and can't be synced.");
      return;
    }
    let serverPath = this._annoSvc.getItemAnnotation(folderId,
                                                     SERVER_PATH_ANNO);
    
    
    
    
    let keyringFile = new Resource(serverPath + "/" + KEYRING_FILE_NAME);
    keyringFile.pushFilter(new JsonFilter());
    keyringFile.get(self.cb);
    let keys = yield;

    
    let idRSA = ID.get('WeaveCryptoID');
    let bulkKey = yield Crypto.unwrapKey.async(Crypto, self.cb,
                           keys.ring[this._myUsername], idRSA);
    let bulkIV = keys.bulkIV;

    
    let wrapMount = this._engine._store._wrapMountOutgoing(folderId);
    let jsonService = Components.classes["@mozilla.org/dom/json;1"]
                 .createInstance(Components.interfaces.nsIJSON);
    let json = jsonService.encode( wrapMount );

    
    let bmkFile = new Resource(serverPath + "/" + SHARED_BOOKMARK_FILE_NAME);
    let tmpIdentity = {
                        realm   : "temp ID",
                        bulkKey : bulkKey,
                        bulkIV  : bulkIV
                      };
    Crypto.encryptData.async( Crypto, self.cb, json, tmpIdentity );
    let cyphertext = yield;
    yield bmkFile.put( self.cb, cyphertext );
    self.done();
  },

  _stopOutgoingShare: function BmkSharing__stopOutgoingShare(folderId) {
    


    let self = yield;
    if (this._annoSvc.itemHasAnnotation(folderId, SERVER_PATH_ANNO)){
      let serverPath = this._annoSvc.getItemAnnotation( folderId,
                                                      SERVER_PATH_ANNO );
      
      
      let keyringFile = new Resource(serverPath + "/" + KEYRING_FILE_NAME);
      keyringFile.delete(self.cb);
      yield;
      let bmkFile = new Resource(serverPath + "/" + SHARED_BOOKMARK_FILE_NAME);
      keyringFile.delete(self.cb);
      yield;
      
      
    }
    
    this._annoSvc.removeItemAnnotation(folderId, SERVER_PATH_ANNO);
    this._annoSvc.removeItemAnnotation(folderId, OUTGOING_SHARED_ANNO);
    self.done();
  },

  _createIncomingShare: function BmkSharing__createShare(user,
                                                         serverPath,
                                                         title) {
    








    

    dump( "I'm in _createIncomingShare.  user= " + user + "path = " +
	  serverPath + ", title= " + title + "\n" );
    let root;
    let a = this._annoSvc.getItemsWithAnnotation(INCOMING_SHARE_ROOT_ANNO,
                                                 {});
    if (a.length == 1)
      root = a[0];
    if (!root) {
      root = this._bms.createFolder(this._bms.toolbarFolder,
			            INCOMING_SHARE_ROOT_NAME,
                                    this._bms.DEFAULT_INDEX);
      this._annoSvc.setItemAnnotation(root,
                                      INCOMING_SHARE_ROOT_ANNO,
                                      true,
                                      0,
                                      this._annoSvc.EXPIRE_NEVER);
    }
    



    let itemExists = false;
    a = this._annoSvc.getItemsWithAnnotation(INCOMING_SHARED_ANNO, {});
    for (let i = 0; i < a.length; i++) {
      let creator = this._annoSvc.getItemAnnotation(a[i], INCOMING_SHARED_ANNO);
      let path = this._annoSvc.getItemAnnotation(a[i], SERVER_PATH_ANNO);
      if ( creator == user && path == serverPath ) {
        itemExists = true;
        break;
      }
    }
    if (!itemExists) {
      let newId = this._bms.createFolder(root, title, this._bms.DEFAULT_INDEX);
      
      this._annoSvc.setItemAnnotation(newId,
                                      INCOMING_SHARED_ANNO,
                                      user,
                                      0,
                                      this._annoSvc.EXPIRE_NEVER);
      
      this._annoSvc.setItemAnnotation(newId,
                                      SERVER_PATH_ANNO,
                                      serverPath,
                                      0,
                                      this._annoSvc.EXPIRE_NEVER);
    }
  },

  _updateIncomingShare: function BmkSharing__updateIncomingShare(mountData) {
    







    
    


    let self = yield;
    let user = mountData.userid;
    
    
    let serverPath = mountData.serverPath;
    
    
    this._log.trace("UpdateIncomingShare: getting keyring file.");
    let keyringFile = new Resource(serverPath + "/" + KEYRING_FILE_NAME);
    keyringFile.pushFilter(new JsonFilter());
    let keys = yield keyringFile.get(self.cb);

    
    this._log.trace("UpdateIncomingShare: decrypting sym key.");
    let idRSA = ID.get('WeaveCryptoID');
    let bulkKey = yield Crypto.unwrapKey.async(Crypto, self.cb,
                           keys.ring[this._myUsername], idRSA);
    let bulkIV = keys.bulkIV;

    
    this._log.trace("UpdateIncomingShare: getting encrypted bookmark file.");
    let bmkFile = new Resource(serverPath + "/" + SHARED_BOOKMARK_FILE_NAME);
    let cyphertext = yield bmkFile.get(self.cb);
    let tmpIdentity = {
                        realm   : "temp ID",
			bulkKey : bulkKey,
                        bulkIV  : bulkIV
                      };
    this._log.trace("UpdateIncomingShare: Decrypting.");
    Crypto.decryptData.async( Crypto, self.cb, cyphertext, tmpIdentity );
    let json = yield;
    
    this._log.trace("UpdateIncomingShare: De-JSON-izing.");
    let jsonService = Components.classes["@mozilla.org/dom/json;1"]
                 .createInstance(Components.interfaces.nsIJSON);
    let serverContents = jsonService.decode( json );

    
    this._log.trace("UpdateIncomingShare: Pruning.");
    for (let guid in serverContents) {
      if (serverContents[guid].type != "bookmark")
        delete serverContents[guid];
      else
        serverContents[guid].parentid = mountData.rootGUID;
    }

    
    this._log.trace("Wiping local contents of incoming share...");
    this._bms.removeFolderChildren( mountData.node );

    

    this._log.trace("Got bookmarks from " + user + ", comparing with local copy");
    this._engine._core.detectUpdates(self.cb, {}, serverContents);
    let diff = yield;

    





    
    this._log.trace("Applying changes to folder from " + user);
    this._engine._store.applyCommands.async(this._engine._store, self.cb, diff);
    yield;

    this._log.trace("Shared folder from " + user + " successfully synced!");
  },

  _stopIncomingShare: function BmkSharing__stopIncomingShare(user,
                                                             serverPath,
                                                             folderName)
  {
  



    let a = this._annoSvc.getItemsWithAnnotation(OUTGOING_SHARED_ANNO, {});
    for (let i = 0; i < a.length; i++) {
      let creator = this._annoSvc.getItemAnnotation(a[i], OUTGOING_SHARED_ANNO);
      let path = this._annoSvc.getItemAnnotation(a[i], SERVER_PATH_ANNO);
      if ( creator == user && path == serverPath ) {
        this._bms.removeFolder( a[i]);
      }
    }
  }
}

function BookmarksEngine(pbeId) {
  this._init(pbeId);
}
BookmarksEngine.prototype = {
  __proto__: SyncEngine.prototype,
  get _super() SyncEngine.prototype,

  get name() "bookmarks",
  get displayName() "Bookmarks",
  get logName() "BmkEngine",
  get serverPrefix() "user-data/bookmarks/",

  get _store() {
    let store = new BookmarksStore();
    this.__defineGetter__("_store", function() store);
    return store;
  },

  get _core() {
    let core = new BookmarksSyncCore();
    this.__defineGetter__("_core", function() core);
    return core;
  },

  get _tracker() {
    let tracker = new BookmarksTracker();
    this.__defineGetter__("_tracker", function() tracker);
    return tracker;
  },

  _getAllIDs: function BmkEngine__getAllIDs() {
    let self = yield;
    let all = this._store.wrap(); 
    delete all["unfiled"];
    delete all["toolbar"];
    delete all["menu"];
    self.done(all);
  },

  _serializeItem: function BmkEngine__serializeItem(id) {
    let self = yield;
    let all = this._store.wrap(); 
    self.done(all[id]);
  },

  _recordLike: function SyncEngine__recordLike(a, b) {
    if (a.parentid != b.parentid)
      return false;
    for (let key in a.cleartext) {
      if (key == "index")
        continue;
      if (!Utils.deepEquals(a.cleartext[key], b.cleartext[key]))
        return false;
    }
    for (key in b.cleartext) {
      if (key == "index")
        continue;
      if (!Utils.deepEquals(a.cleartext[key], b.cleartext[key]))
        return false;
    }
    return true;
  },

  _changeRecordRefs: function BmkEngine__changeRecordRefs(oldID, newID) {
    let self = yield;
    for each (let rec in this.outgoing) {
      if (rec.parentid == oldID) {
        rec.parentid = newID;
        rec.cleartext.parentid = newID;
        yield rec.encrypt(self.cb, ID.get('WeaveCryptoID').password);
      }
    }
  },

  _changeItemID: function BmkEngine__changeRecordID(oldID, newID) {
    let self = yield;
    yield this._store._changeItemID.async(this._store, self.cb, oldID, newID);
  }

  
  
};

function BookmarksSyncCore(store) {
  this._store = store;
  this._init();
}
BookmarksSyncCore.prototype = {
  __proto__: SyncCore.prototype,
  _logName: "BMSync",
  _store: null,

  _getEdits: function BSC__getEdits(a, b) {
    
    
    let ret = SyncCore.prototype._getEdits.call(this, a, b);
    ret.props.type = a.type;
    return ret;
  },

  
  
  
  
  _comp: function BSC__comp(a, b, prop) {
    return (!a.data[prop] && !b.data[prop]) ||
      (a.data[prop] && b.data[prop] && (a.data[prop] == b.data[prop]));
  },

  _commandLike: function BSC__commandLike(a, b) {
    
    
    
    
    
    
    
    
    
    
    
    
    if (!a || !b ||
        a.action != b.action ||
        a.action != "create" ||
        a.data.type != b.data.type ||
        a.data.parentid != b.data.parentid ||
        a.GUID == b.GUID)
      return false;

    
    
    
    switch (a.data.type) {
    case "bookmark":
      if (this._comp(a, b, 'URI') &&
          this._comp(a, b, 'title'))
        return true;
      return false;
    case "query":
      if (this._comp(a, b, 'URI') &&
          this._comp(a, b, 'title'))
        return true;
      return false;
    case "microsummary":
      if (this._comp(a, b, 'URI') &&
          this._comp(a, b, 'generatorURI'))
        return true;
      return false;
    case "folder":
      if (this._comp(a, b, 'title'))
        return true;
      return false;
    case "livemark":
      if (this._comp(a, b, 'title') &&
          this._comp(a, b, 'siteURI') &&
          this._comp(a, b, 'feedURI'))
        return true;
      return false;
    case "separator":
      if (this._comp(a, b, 'index'))
        return true;
      return false;
    default:
      let json = Cc["@mozilla.org/dom/json;1"].createInstance(Ci.nsIJSON);
      this._log.error("commandLike: Unknown item type: " + json.encode(a));
      return false;
    }
  }
};

function BookmarksStore() {
  this._init();
}
BookmarksStore.prototype = {
  __proto__: Store.prototype,
  _logName: "BStore",
  _lookup: null,

  __bms: null,
  get _bms() {
    if (!this.__bms)
      this.__bms = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
                   getService(Ci.nsINavBookmarksService);
    return this.__bms;
  },

  __hsvc: null,
  get _hsvc() {
    if (!this.__hsvc)
      this.__hsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
                    getService(Ci.nsINavHistoryService);
    return this.__hsvc;
  },

  __ls: null,
  get _ls() {
    if (!this.__ls)
      this.__ls = Cc["@mozilla.org/browser/livemark-service;2"].
        getService(Ci.nsILivemarkService);
    return this.__ls;
  },

  __ms: null,
  get _ms() {
    if (!this.__ms)
      this.__ms = Cc["@mozilla.org/microsummary/service;1"].
        getService(Ci.nsIMicrosummaryService);
    return this.__ms;
  },

  __ts: null,
  get _ts() {
    if (!this.__ts)
      this.__ts = Cc["@mozilla.org/browser/tagging-service;1"].
                  getService(Ci.nsITaggingService);
    return this.__ts;
  },

  __ans: null,
  get _ans() {
    if (!this.__ans)
      this.__ans = Cc["@mozilla.org/browser/annotation-service;1"].
                   getService(Ci.nsIAnnotationService);
    return this.__ans;
  },

  _getItemIdForGUID: function BStore__getItemIdForGUID(GUID) {
    switch (GUID) {
    case "menu":
      return this._bms.bookmarksMenuFolder;
    case "toolbar":
      return this._bms.toolbarFolder;
    case "unfiled":
      return this._bms.unfiledBookmarksFolder;
    default:
      return this._bms.getItemIdForGUID(GUID);
    }
    return null;
  },

  applyIncoming: function BStore_applyIncoming(onComplete, record) {
    let fn = function(rec) {
      let self = yield;
      if (!record.cleartext)
        this._removeCommand({GUID: record.id});
      else if (this._getItemIdForGUID(record.id) < 0)
        this._createCommand({GUID: record.id, data: record.cleartext});
      else
        this._editCommand({GUID: record.id, data: record.cleartext});
    };
    fn.async(this, onComplete, record);
  },

  _createCommand: function BStore__createCommand(command) {
    let newId;
    let parentId = this._getItemIdForGUID(command.data.parentid);

    if (parentId < 0) {
      this._log.warn("Creating node with unknown parent -> reparenting to root");
      parentId = this._bms.bookmarksMenuFolder;
    }

    switch (command.data.type) {
    case "query":
    case "bookmark":
    case "microsummary": {
      this._log.debug(" -> creating bookmark \"" + command.data.title + "\"");
      let URI = Utils.makeURI(command.data.URI);
      newId = this._bms.insertBookmark(parentId,
                                       URI,
                                       command.data.index,
                                       command.data.title);
      this._ts.untagURI(URI, null);
      this._ts.tagURI(URI, command.data.tags);
      this._bms.setKeywordForBookmark(newId, command.data.keyword);
      if (command.data.description) {
        this._ans.setItemAnnotation(newId, "bookmarkProperties/description",
                                    command.data.description, 0,
                                    this._ans.EXPIRE_NEVER);
      }

      if (command.data.type == "microsummary") {
        this._log.debug("   \-> is a microsummary");
        this._ans.setItemAnnotation(newId, "bookmarks/staticTitle",
                                    command.data.staticTitle || "", 0, this._ans.EXPIRE_NEVER);
        let genURI = Utils.makeURI(command.data.generatorURI);
        try {
          let micsum = this._ms.createMicrosummary(URI, genURI);
          this._ms.setMicrosummary(newId, micsum);
        }
        catch(ex) {  }
      }
    } break;
    case "folder":
      this._log.debug(" -> creating folder \"" + command.data.title + "\"");
      newId = this._bms.createFolder(parentId,
                                     command.data.title,
                                     command.data.index);
      
      if ( command.data.outgoingSharedAnno != undefined ) {
	this._ans.setItemAnnotation(newId,
				    OUTGOING_SHARED_ANNO,
                                    command.data.outgoingSharedAnno,
				    0,
				    this._ans.EXPIRE_NEVER);
	this._ans.setItemAnnotation(newId,
				    SERVER_PATH_ANNO,
                                    command.data.serverPathAnno,
				    0,
				    this._ans.EXPIRE_NEVER);

      }
      break;
    case "livemark":
      this._log.debug(" -> creating livemark \"" + command.data.title + "\"");
      newId = this._ls.createLivemark(parentId,
                                      command.data.title,
                                      Utils.makeURI(command.data.siteURI),
                                      Utils.makeURI(command.data.feedURI),
                                      command.data.index);
      break;
    case "incoming-share":
      


      this._log.debug(" -> creating incoming-share \"" + command.data.title + "\"");
      newId = this._bms.createFolder(parentId,
                                     command.data.title,
                                     command.data.index);
      this._ans.setItemAnnotation(newId,
				  INCOMING_SHARED_ANNO,
                                  command.data.incomingSharedAnno,
				  0,
				  this._ans.EXPIRE_NEVER);
      this._ans.setItemAnnotation(newId,
				  SERVER_PATH_ANNO,
                                  command.data.serverPathAnno,
				  0,
				  this._ans.EXPIRE_NEVER);
      break;
    case "separator":
      this._log.debug(" -> creating separator");
      newId = this._bms.insertSeparator(parentId, command.data.index);
      break;
    default:
      this._log.error("_createCommand: Unknown item type: " + command.data.type);
      break;
    }
    if (newId) {
      this._log.trace("Setting GUID of new item " + newId + " to " + command.GUID);
      let cur = this._bms.getItemGUID(newId);
      if (cur == command.GUID)
        this._log.warn("Item " + newId + " already has GUID " + command.GUID);
      else {
        this._bms.setItemGUID(newId, command.GUID);
        Engines.get("bookmarks")._tracker._all[newId] = command.GUID; 
      }
    }
  },

  _removeCommand: function BStore__removeCommand(command) {
    if (command.GUID == "menu" ||
        command.GUID == "toolbar" ||
        command.GUID == "unfiled") {
      this._log.warn("Attempted to remove root node (" + command.GUID +
                     ").  Skipping command.");
      return;
    }

    var itemId = this._bms.getItemIdForGUID(command.GUID);
    if (itemId < 0) {
      this._log.debug("Item " + command.GUID + "already removed");
      return;
    }
    var type = this._bms.getItemType(itemId);

    switch (type) {
    case this._bms.TYPE_BOOKMARK:
      this._log.debug("  -> removing bookmark " + command.GUID);
      this._bms.removeItem(itemId);
      break;
    case this._bms.TYPE_FOLDER:
      this._log.debug("  -> removing folder " + command.GUID);
      this._bms.removeFolder(itemId);
      break;
    case this._bms.TYPE_SEPARATOR:
      this._log.debug("  -> removing separator " + command.GUID);
      this._bms.removeItem(itemId);
      break;
    default:
      this._log.error("removeCommand: Unknown item type: " + type);
      break;
    }
  },

  _editCommand: function BStore__editCommand(command) {
    if (command.GUID == "menu" ||
        command.GUID == "toolbar" ||
        command.GUID == "unfiled") {
      this._log.debug("Attempted to edit root node (" + command.GUID +
                      ").  Skipping command.");
      return;
    }

    var itemId = this._getItemIdForGUID(command.GUID);
    if (itemId < 0) {
      this._log.debug("Item for GUID " + command.GUID + " not found.  Skipping.");
      return;
    }
    this._log.trace("Editing item " + itemId);

    for (let key in command.data) {
      switch (key) {
      case "type":
        
        
        break;
      case "title":
        this._bms.setItemTitle(itemId, command.data.title);
        break;
      case "URI":
        this._bms.changeBookmarkURI(itemId, Utils.makeURI(command.data.URI));
        break;
      case "index":
        let curIdx = this._bms.getItemIndex(itemId);
        if (curIdx != command.data.index) {
          
          if (command.data.parentid &&
              (this._bms.getFolderIdForItem(itemId) !=
               this._getItemIdForGUID(command.data.parentid)))
            break;
          this._log.trace("Moving item (changing index)");
          this._bms.moveItem(itemId, this._bms.getFolderIdForItem(itemId),
                             command.data.index);
        }
        break;
      case "parentid": {
        if (command.data.parentid &&
            (this._bms.getFolderIdForItem(itemId) !=
             this._getItemIdForGUID(command.data.parentid))) {
          this._log.trace("Moving item (changing folder)");
          let index = -1;
          if (command.data.index && command.data.index >= 0)
            index = command.data.index;
          this._bms.moveItem(itemId,
                             this._getItemIdForGUID(command.data.parentid), index);
        }
      } break;
      case "tags": {
        
        let tags = command.data.tags.filter(function(t) t);
        let tagsURI = this._bms.getBookmarkURI(itemId);
        this._ts.untagURI(tagsURI, null);
        this._ts.tagURI(tagsURI, tags);
      } break;
      case "keyword":
        this._bms.setKeywordForBookmark(itemId, command.data.keyword);
        break;
      case "description":
        if (command.data.description) {
          this._ans.setItemAnnotation(itemId, "bookmarkProperties/description",
                                      command.data.description, 0,
                                      this._ans.EXPIRE_NEVER);
        }
        break;
      case "generatorURI": {
        let micsumURI = Utils.makeURI(this._bms.getBookmarkURI(itemId));
        let genURI = Utils.makeURI(command.data.generatorURI);
        let micsum = this._ms.createMicrosummary(micsumURI, genURI);
        this._ms.setMicrosummary(itemId, micsum);
      } break;
      case "siteURI":
        this._ls.setSiteURI(itemId, Utils.makeURI(command.data.siteURI));
        break;
      case "feedURI":
        this._ls.setFeedURI(itemId, Utils.makeURI(command.data.feedURI));
        break;
      case "outgoingSharedAnno":
	this._ans.setItemAnnotation(itemId, OUTGOING_SHARED_ANNO,
				    command.data.outgoingSharedAnno, 0,
				    this._ans.EXPIRE_NEVER);
	break;
      case "incomingSharedAnno":
	this._ans.setItemAnnotation(itemId, INCOMING_SHARED_ANNO,
				    command.data.incomingSharedAnno, 0,
				    this._ans.EXPIRE_NEVER);
	break;
      case "serverPathAnno":
	this._ans.setItemAnnotation(itemId, SERVER_PATH_ANNO,
				    command.data.serverPathAnno, 0,
				    this._ans.EXPIRE_NEVER);
	break;
      default:
        this._log.warn("Can't change item property: " + key);
        break;
      }
    }
  },

  _changeItemID: function BSS__changeItemID(oldID, newID) {
    let self = yield;

    var itemId = this._getItemIdForGUID(oldID);
    if (itemId == null) 
      return;
    if (itemId < 0) {
      this._log.warn("Can't change GUID " + oldID + " to " +
                      newID + ": Item does not exist");
      return;
    }

    var collision = this._getItemIdForGUID(newID);
    if (collision >= 0) {
      this._log.warn("Can't change GUID " + oldID + " to " +
                      newID + ": new ID already in use");
      return;
    }

    this._log.debug("Changing GUID " + oldID + " to " + newID);
    this._bms.setItemGUID(itemId, newID);
    Engines.get("bookmarks")._tracker._all[itemId] = newID; 
  },

  _getNode: function BSS__getNode(folder) {
    let query = this._hsvc.getNewQuery();
    query.setFolders([folder], 1);
    return this._hsvc.executeQuery(query, this._hsvc.getNewQueryOptions()).root;
  },

  __wrap: function BSS___wrap(node, items, parentid, index, guidOverride) {
    let GUID, item;

    
    if (guidOverride) {
      GUID = guidOverride;
      item = {};
    } else {
      GUID = this._bms.getItemGUID(node.itemId);
      item = {parentid: parentid, index: index};
    }

    if (node.type == node.RESULT_TYPE_FOLDER) {
      if (this._ls.isLivemark(node.itemId)) {
        item.type = "livemark";
        let siteURI = this._ls.getSiteURI(node.itemId);
        let feedURI = this._ls.getFeedURI(node.itemId);
        item.siteURI = siteURI? siteURI.spec : "";
        item.feedURI = feedURI? feedURI.spec : "";
      } else if (this._ans.itemHasAnnotation(node.itemId, INCOMING_SHARED_ANNO)){
	


	item.type = "incoming-share";
	item.title = node.title;
        item.serverPathAnno = this._ans.getItemAnnotation(node.itemId,
                                                      SERVER_PATH_ANNO);
	item.incomingSharedAnno = this._ans.getItemAnnotation(node.itemId,
                                                      INCOMING_SHARED_ANNO);
      } else {
        item.type = "folder";
        node.QueryInterface(Ci.nsINavHistoryQueryResultNode);
        node.containerOpen = true;
	
	if (this._ans.itemHasAnnotation(node.itemId, OUTGOING_SHARED_ANNO)) {
	  item.outgoingSharedAnno = this._ans.getItemAnnotation(node.itemId,
                                                      OUTGOING_SHARED_ANNO);
	}
	if (this._ans.itemHasAnnotation(node.itemId, SERVER_PATH_ANNO)) {
	  item.serverPathAnno = this._ans.getItemAnnotation(node.itemId,
							    SERVER_PATH_ANNO);
	}

        for (var i = 0; i < node.childCount; i++) {
          this.__wrap(node.getChild(i), items, GUID, i);
        }
      }
      if (!guidOverride)
        item.title = node.title; 

    } else if (node.type == node.RESULT_TYPE_URI ||
               node.type == node.RESULT_TYPE_QUERY) {
      if (this._ms.hasMicrosummary(node.itemId)) {
        item.type = "microsummary";
        let micsum = this._ms.getMicrosummary(node.itemId);
        item.generatorURI = micsum.generator.uri.spec; 
        item.staticTitle = "";
        try {
          item.staticTitle = this._ans.getItemAnnotation(node.itemId,
                                                         "bookmarks/staticTitle");
        } catch (e) {}
      } else if (node.type == node.RESULT_TYPE_QUERY) {
        item.type = "query";
        item.title = node.title;
      } else {
        item.type = "bookmark";
        item.title = node.title;
      }

      try {
        item.description =
          this._ans.getItemAnnotation(node.itemId, "bookmarkProperties/description");
      } catch (e) {
        item.description = undefined;
      }

      item.URI = node.uri;

      
      
      
      
      let uri;
      try {
        uri = Utils.makeURI(node.uri);
      }
      catch(e) {
        this._log.error("error parsing URI string <" + node.uri + "> " +
                        "for item " + node.itemId + " (" + node.title + "): " +
                        e);
      }

      if (uri)
        item.tags = this._ts.getTagsForURI(uri, {});

      item.keyword = this._bms.getKeywordForBookmark(node.itemId);

    } else if (node.type == node.RESULT_TYPE_SEPARATOR) {
      item.type = "separator";

    } else {
      this._log.warn("Warning: unknown item type, cannot serialize: " + node.type);
      return;
    }

    items[GUID] = item;
  },

  
  _wrap: function BStore__wrap(node, items, rootName) {
    return this.__wrap(node, items, null, null, rootName);
  },

  _wrapMountOutgoing: function BStore__wrapById( itemId ) {
    let node = this._getNode(itemId);
    if (node.type != node.RESULT_TYPE_FOLDER)
      throw "Trying to wrap a non-folder mounted share";

    let GUID = this._bms.getItemGUID(itemId);
    let snapshot = {};
    node.QueryInterface(Ci.nsINavHistoryQueryResultNode);
    node.containerOpen = true;
    for (var i = 0; i < node.childCount; i++) {
      this.__wrap(node.getChild(i), snapshot, GUID, i);
    }

    
    for (let guid in snapshot) {
      
      if (snapshot[guid].type == "incoming-share")
        delete snapshot[guid];
    }
    return snapshot;
  },

  findIncomingShares: function BStore_findIncomingShares() {
    

    let ret = [];
    let a = this._ans.getItemsWithAnnotation(INCOMING_SHARED_ANNO, {});
    for (let i = 0; i < a.length; i++) {
      

      let userId = this._ans.getItemAnnotation(a[i], INCOMING_SHARED_ANNO);
      let node = this._getNode(a[i]);
      let GUID = this._bms.getItemGUID(a[i]);
      let path = this._ans.getItemAnnotation(a[i], SERVER_PATH_ANNO);
      let dat = {rootGUID: GUID, userid: userId, serverPath: path, node: node};
      ret.push(dat);
    }
    return ret;
  },

  wrap: function BStore_wrap() {
    var items = {};
    this._wrap(this._getNode(this._bms.bookmarksMenuFolder), items, "menu");
    this._wrap(this._getNode(this._bms.toolbarFolder), items, "toolbar");
    this._wrap(this._getNode(this._bms.unfiledBookmarksFolder), items, "unfiled");
    this._lookup = items;
    return items;
  },

  wipe: function BStore_wipe() {
    this._bms.removeFolderChildren(this._bms.bookmarksMenuFolder);
    this._bms.removeFolderChildren(this._bms.toolbarFolder);
    this._bms.removeFolderChildren(this._bms.unfiledBookmarksFolder);
  },

  __resetGUIDs: function BStore___resetGUIDs(node) {
    let self = yield;

    if (this._ans.itemHasAnnotation(node.itemId, "placesInternal/GUID"))
      this._ans.removeItemAnnotation(node.itemId, "placesInternal/GUID");

    if (node.type == node.RESULT_TYPE_FOLDER &&
        !this._ls.isLivemark(node.itemId)) {
      yield Utils.makeTimerForCall(self.cb); 
      node.QueryInterface(Ci.nsINavHistoryQueryResultNode);
      node.containerOpen = true;
      for (var i = 0; i < node.childCount; i++) {
        this.__resetGUIDs(node.getChild(i));
      }
    }
  },

  _resetGUIDs: function BStore__resetGUIDs() {
    let self = yield;
    this.__resetGUIDs(this._getNode(this._bms.bookmarksMenuFolder));
    this.__resetGUIDs(this._getNode(this._bms.toolbarFolder));
    this.__resetGUIDs(this._getNode(this._bms.unfiledBookmarksFolder));
  }
};








function BookmarksTracker() {
  this._init();
}
BookmarksTracker.prototype = {
  __proto__: Tracker.prototype,
  _logName: "BmkTracker",
  file: "bookmarks",

  get _bms() {
    let bms = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
      getService(Ci.nsINavBookmarksService);
    this.__defineGetter__("_bms", function() bms);
    return bms;
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsINavBookmarkObserver]),

  _init: function BMT__init() {
    this.__proto__.__proto__._init.call(this);

    
    
    
    
    

    
    let store = new BookmarksStore();
    let all = store.wrap();
    this._all = {};
    for (let guid in all) {
      this._all[this._bms.getItemIdForGUID(guid)] = guid;
    }

    this._bms.addObserver(this, false);
  },

  
  _upScore: function BMT__upScore() {
    if (!this.enabled)
      return;
    this._score += 10;
  },

  onItemAdded: function BMT_onEndUpdateBatch(itemId, folder, index) {
    this._log.trace("onItemAdded: " + itemId);

    this._all[itemId] = this._bms.getItemGUID(itemId);
    this.addChangedID(this._all[itemId]);

    this._upScore();
  },

  onItemRemoved: function BMT_onItemRemoved(itemId, folder, index) {
    this._log.trace("onItemRemoved: " + itemId);

    this.addChangedID(this._all[itemId]);
    delete this._all[itemId];

    this._upScore();
  },

  onItemChanged: function BMT_onItemChanged(itemId, property, isAnnotationProperty, value) {
    this._log.trace("onItemChanged: " + itemId + ", property: " + property +
                    ", isAnno: " + isAnnotationProperty + ", value: " + value);

    
    
    
    
    
    
    
    let guid = this._bms.getItemGUID(itemId);
    if (guid != this._all[itemId])
      this._log.trace("GUID change, ignoring");
    else
      this.addChangedID(this._all[itemId]); 

    this._upScore();
  },

  onItemMoved: function BMT_onItemMoved(itemId, oldParent, oldIndex, newParent, newIndex) {
    this._log.trace("onItemMoved: " + itemId);
    this.addChangedID(this._all[itemId]);
    this._upScore();
  },

  onBeginUpdateBatch: function BMT_onBeginUpdateBatch() {},
  onEndUpdateBatch: function BMT_onEndUpdateBatch() {},
  onItemVisited: function BMT_onItemVisited(itemId, aVisitID, time) {}
};
