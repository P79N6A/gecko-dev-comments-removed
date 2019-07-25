




































const EXPORTED_SYMBOLS = ['BookmarksEngine'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;


const INCOMING_SHARED_ANNO = "weave/shared-incoming";
const OUTGOING_SHARED_ANNO = "weave/shared-outgoing";
const SERVER_PATH_ANNO = "weave/shared-server-path";

const KEYRING_FILE_NAME = "keyring";
const SHARED_BOOKMARK_FILE_NAME = "shared_bookmarks";

Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/dav.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/crypto.js");
Cu.import("resource://weave/async.js");
Cu.import("resource://weave/engines.js");
Cu.import("resource://weave/syncCores.js");
Cu.import("resource://weave/stores.js");
Cu.import("resource://weave/trackers.js");





Cu.import("resource://weave/xmpp/xmppClient.js");

Function.prototype.async = Async.sugar;

function BookmarksEngine(pbeId) {
  this._init(pbeId);
}
BookmarksEngine.prototype = {
  get name() { return "bookmarks"; },
  get logName() { return "BmkEngine"; },
  get serverPrefix() { return "user-data/bookmarks/"; },

  __core: null,
  get _core() {
    if (!this.__core)
      this.__core = new BookmarksSyncCore();
    return this.__core;
  },

  __store: null,
  get _store() {
    if (!this.__store)
      this.__store = new BookmarksStore();
    return this.__store;
  },

  __tracker: null,
  get _tracker() {
    if (!this.__tracker)
      this.__tracker = new BookmarksTracker();
    return this.__tracker;
  },
  
  __annoSvc: null,
  get _annoSvc() {
    if (!this.__anoSvc)
      this.__annoSvc = Cc["@mozilla.org/browser/annotation-service;1"].
        getService(Ci.nsIAnnotationService);
    return this.__annoSvc;
  }

  _init: function BmkEngine__init( pbeId ) {
    this.__proto__.__proto__._init.call( this, pbeId );
    if ( Utils.prefs.getBoolPref( "xmpp.enabled" ) ) {
      dump( "Starting XMPP client for bookmark engine..." );
      this._startXmppClient.async(this);
    }
  },

  _startXmppClient: function BmkEngine__startXmppClient() {
    
    let self = yield;

    
    let serverUrl = Utils.prefs.getCharPref( "xmpp.server.url" );
    let realm = Utils.prefs.getCharPref( "xmpp.server.realm" );

    
    
    
    
    
    
    let clientName = Utils.prefs.getCharPref( "xmpp.client.name" );
    let clientPassword = Utils.prefs.getCharPref( "xmpp.client.password" );
    let transport = new HTTPPollingTransport( serverUrl, false, 15000 );
    let auth = new PlainAuthenticator();
    
    
    this._xmppClient = new XmppClient( clientName,
                                       realm,
                                       clientPassword,
				       transport,
                                       auth );
    let bmkEngine = this;
    let messageHandler = {
      handle: function ( messageText, from ) {
        













 	let words = messageText.split(" ");
	let commandWord = words[0];
	let serverPath = words[1];
	let directoryName = words.slice(2).join(" ");
        if ( commandWord == "share" ) {
	  bmkEngine._incomingShareOffer(from, serverPath, folderName);
	} else if ( commandWord == "stop" ) {
	  bmkEngine._incomingShareWithdrawn(from, serverPath, folderName);
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

  _incomingShareOffer: function BmkEngine__incomingShareOffer(user,
                                                              serverPath,
                                                              folderName) {
    










    dump( "I was offered the directory " + dir + " from user " + dir );
    _createIncomingShare( user, serverPath, folderName );
  },

  _incomingShareWithdrawn: function BmkEngine__incomingShareStop(user,
                                                                 serverPath,
                                                                 folderName) {
    







  },

  _sync: function BmkEngine__sync() {
    

    let self = yield;
    this.__proto__.__proto__._sync.async(this, self.cb );
    yield;
    this.updateAllOutgoingShares(self.cb);
    yield;
    this.updateAllIncomingShares(self.cb);
    yield;
    self.done();
  },

  _share: function BmkEngine__share( selectedFolder, username ) {
    
    let ret = false;
    let self = yield;

    



    
    this._createOutgoingShare.async( this, self.cb, selectedFolder, username );
    let serverPath = yield;
    this._updateOutgoingShare.async( this, self.cb, selectedFolder );
    yield;

    

    let folderItemId = selectedFolder.node.itemId;
    let folderName = selectedFolder.getAttribute( "label" );
    this._annoSvc.setItemAnnotation(folderItemId,
                                    OUTGOING_SHARED_ANNO,
                                    username,
                                    0,
                                    this._annoSvc.EXPIRE_NEVER);
    
    if ( this._xmppClient ) {
      if ( this._xmppClient._connectionStatus == this._xmppClient.CONNECTED ) {
	let msgText = "share " + serverPath + " " + folderName;
	this._log.debug( "Sending XMPP message: " + msgText );
	this._xmppClient.sendMessage( username, msgText );
      } else {
	this._log.warn( "No XMPP connection for share notification." );
      }
    }

    



    this._log.info("Shared " + folderName +" with " + username);
    ret = true;
    self.done( true );
  },

  updateAllIncomingShares: function BmkEngine_updateAllIncoming(onComplete) {
    this._updateAllIncomingShares.async(this, onComplete);
  },
  _updateAllIncomingShares: function BmkEngine__updateAllIncoming() {
    






    let self = yield;
    let mounts = this._store.findIncomingShares();

    for (let i = 0; i < mounts.length; i++) {
      try {
        this._updateIncomingShare.async(this, self.cb, mounts[i]);
        yield;
      } catch (e) {
        this._log.warn("Could not sync shared folder from " + mounts[i].userid);
        this._log.trace(Utils.stackTrace(e));
      }
    }
  },

  updateAllOutgoingShares: function BmkEngine_updateAllOutgoing(onComplete) {
    this._updateAllOutgoingShares.async(this, onComplete);
  },
  _updateAllOutgoingShares: function BmkEngine__updateAllOutgoing() {
    let self = yield;
    let shares = this._annoSvc.getItemsWithAnnotation(OUTGOING_SHARED_ANNO, 
                                                      {});
    for ( let i=0; i < shares.length; i++ ) {
      


      this._updateOutgoingShare.async(this, self.cb, shares[i]);
      yield;
    }
    self.done();
  },

  _createOutgoingShare: function BmkEngine__createOutgoing(folder, username) {
    





    let self = yield;
    let myUserName = ID.get('WeaveID').username;
    this._log.debug("Sharing bookmarks from " + folder.getAttribute( "label" )
                    + " with " + username);

    

    let uuidgen = Cc["@mozilla.org/uuid-generator;1"].
        getService(Ci.nsIUUIDGenerator);
    let folderGuid = uuidgen.generateUUID().toString().replace(/[{}]/g, '');

    
    let serverPath = "/user/" + myUserName + "/share/" + folderGuid;
    DAV.MKCOL(serverPath, self.cb);
    let ret = yield;
    if (!ret) {
      this._log.error("Can't create remote folder for outgoing share.");
      self.done(false);
    }
    

    

    this._annoSvc.setItemAnnotation(folder.node.itemId,
                                    SERVER_PATH_ANNO,
                                    serverPath,
                                    0,
                                    this._annoSvc.EXPIRE_NEVER);

    
    Crypto.PBEkeygen.async(Crypto, self.cb);
    let newSymKey = yield;

    

    let myPubKeyFile = new Resource("/user/" + myUserName + "/public/pubkey");
    myPubKeyFile.get(self.cb);
    let myPubKey = yield;
    let userPubKeyFile = new Resource("/user/" + username + "/public/pubkey");
    userPubKeyFile.get(self.cb);
    let userPubKey = yield;

    

    Crypto.RSAencrypt.async(Crypto, self.cb, symKey, {pubkey: myPubKey} );
    let encryptedForMe = yield;
    Crypto.RSAencrypt.async(Crypto, self.cb, symKey, {pubkey: userPubKey} );
    let encryptedForYou = yield;
    let keyring = { myUserName: encryptedForMe,
                    username: encryptedForYou };
    let keyringFile = new Resource( serverPath + "/" + KEYRING_FILE_NAME );
    keyringFile.put( self.cb, this._json.encode( keyring ) );
    yield;

    
    let sharingApi = new Sharing.Api( DAV );
    sharingApi.shareWithUsers( serverPath, [username], self.cb );
    let result = yield;

    
    self.done( serverPath );
  },

  _updateOutgoingShare: function BmkEngine__updateOutgoing(folderNode) {
    




    let self = yield;
    let myUserName = ID.get('WeaveID').username;
    
    
    let serverPath = this._annoSvc.getItemAnnotation(folderNode,
                                                     SERVER_PATH_ANNO);
    
    
    let keyringFile = new Resource(serverPath + "/" + KEYRING_FILE_NAME);
    keyringFile.get(self.cb);
    let keyring = yield;
    let symKey = keyring[ myUserName ];
    
    let json = this._store._wrapMount( folderNode, myUserName );
    


    
    let bmkFile = new Resource(serverPath + "/" + SHARED_BOOKMARK_FILE_NAME);
    Crypto.PBEencrypt.async( Crypto, self.cb, json, {password:symKey} );
    let cyphertext = yield;
    bmkFile.put( self.cb, cyphertext );
    yield;
    self.done();
  },

  _stopOutgoingShare: function BmkEngine__stopOutgoingShare(folderNode) {
    


    
    let self = yield;
    let serverPath = this._annoSvc.getItemAnnotation( folderNode,
                                                      SERVER_PATH_ANNO );
    let username = this._annoSvc.getItemAnnotation( folderNode,
                                                    OUTGOING_SHARE_ANNO );
    
    
    let keyringFile = new Resource(serverPath + "/" + KEYRING_FILE_NAME);
    keyringFile.delete(self.cb);
    yield;
    let bmkFile = new Resource(serverPath + "/" + SHARED_BOOKMARK_FILE_NAME);
    keyringFile.delete(self.cb);
    yield;
    
    
    
    
    this._annoSvc.setItemAnnotation(folderNode,
                                    SERVER_PATH_ANNO
                                    "",
                                    0,
                                    this._annoSvc.EXPIRE_NEVER);
    this._annoSvc.setItemAnnotation(folderNode,
                                    SERVER_PATH_ANNO
                                    "",
                                    0,
                                    this._annoSvc.EXPIRE_NEVER);
    
    

    
    if ( this._xmppClient ) {
      if ( this._xmppClient._connectionStatus == this._xmppClient.CONNECTED ) {
 	let folderName = folderNode.getAttribute( "label" );
	let msgText = "stop " + serverPath + " " + folderName;
	this._log.debug( "Sending XMPP message: " + msgText );
	this._xmppClient.sendMessage( username, msgText );
      } else {
	this._log.warn( "No XMPP connection for share notification." );
      }
    }

  },

  _createIncomingShare: function BookmarkEngine__createShare(user,
                                                             serverPath,
                                                             title) {
    







    let bms = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
      getService(Ci.nsINavBookmarksService);

    

    let root;
    let a = this._annoSvc.getItemsWithAnnotation("weave/mounted-shares-folder",
                                                 {});
    if (a.length == 1)
      root = a[0];
    if (!root) {
      root = bms.createFolder(bms.toolbarFolder, "Shared Folders",
                              bms.DEFAULT_INDEX);
      this._annoSvc.setItemAnnotation(root,
                                      "weave/mounted-shares-folder",
                                      true,
                                      0,
                                      this._annoSvc.EXPIRE_NEVER);
    }
    



    let itemExists = false;
    a = this._annoSvc.getItemsWithAnnotation("weave/mounted-share-id", {});
    for (let i = 0; i < a.length; i++) {
      let creator = this._annoSvc.getItemAnnotation(a[i], OUTGOING_SHARED_ANNO);
      let path = this._annoSvc.getItemAnnotation(a[i], SERVER_PATH_ANNO);
      if ( creator == user && path == serverPath ) {
        itemExists = true;
        break;
      }
    }
    if (!itemExists) {
      let newId = bms.createFolder(root, title, bms.DEFAULT_INDEX);
      


      this._annoSvc.setItemAnnotation(newId,
                                      "weave/mounted-share-id",
                                      id,
                                      0,
                                      this._annoSvc.EXPIRE_NEVER);
      
      this._annoSvc.setItemAnnotation(newId,
                                      OUTGOING_SHARED_ANNO
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

  _updateIncomingShare: function BmkEngine__updateIncomingShare(mountData) {
    








    let self = yield;
    let user = mountData.userid;
    let myUserName = ID.get('WeaveID').username;
    
    
    let serverPath = this._annoSvc.getItemAnnotation(mountData.node,
                                                     SERVER_PATH_ANNO);
    
    
    let keyringFile = new Resource(serverPath + "/" + KEYRING_FILE_NAME);
    keyringFile.get(self.cb);
    let keyring = yield;
    let symKey = keyring[ myUserName ];

    
    let bmkFile = new Resource(serverPath + "/" + SHARED_BOOKMARK_FILE_NAME);
    bmkFile.get(self.cb);
    let cyphertext = yield;
    Crypto.PBEdecrypt.async( Crypto, self.cb, cyphertext, {password:symKey} );
    let json = yield;
    

    
    for (let guid in json) {
      if (json[guid].type != "bookmark")
        delete json[guid];
      else
        json[guid].parentGUID = mountData.rootGUID;
    }

    

    this._log.trace("Got bookmarks from " + user + ", comparing with local copy");
    this._core.detectUpdates(self.cb, mountData.snapshot, snap.data);
    let diff = yield;

    
    this._log.trace("Applying changes to folder from " + user);
    this._store.applyCommands.async(this._store, self.cb, diff);
    yield;

    this._log.trace("Shared folder from " + user + " successfully synced!");
  }
};
BookmarksEngine.prototype.__proto__ = new Engine();

function BookmarksSyncCore() {
  this._init();
}
BookmarksSyncCore.prototype = {
  _logName: "BMSync",

  __bms: null,
  get _bms() {
    if (!this.__bms)
      this.__bms = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
                   getService(Ci.nsINavBookmarksService);
    return this.__bms;
  },

  _itemExists: function BSC__itemExists(GUID) {
    return this._bms.getItemIdForGUID(GUID) >= 0;
  },

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
        a.data.parentGUID != b.data.parentGUID ||
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
BookmarksSyncCore.prototype.__proto__ = new SyncCore();

function BookmarksStore() {
  this._init();
}
BookmarksStore.prototype = {
  _logName: "BStore",

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

  _createCommand: function BStore__createCommand(command) {
    let newId;
    let parentId = this._getItemIdForGUID(command.data.parentGUID);

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
      break;
    case "livemark":
      this._log.debug(" -> creating livemark \"" + command.data.title + "\"");
      newId = this._ls.createLivemark(parentId,
                                      command.data.title,
                                      Utils.makeURI(command.data.siteURI),
                                      Utils.makeURI(command.data.feedURI),
                                      command.data.index);
      break;
    case "mounted-share":
    
    
    
      this._log.debug(" -> creating share mountpoint \"" + command.data.title + "\"");
      newId = this._bms.createFolder(parentId,
                                     command.data.title,
                                     command.data.index);

      this._ans.setItemAnnotation(newId, "weave/mounted-share-id",
                                  command.data.mountId, 0, this._ans.EXPIRE_NEVER);
      break;
    case "separator":
      this._log.debug(" -> creating separator");
      newId = this._bms.insertSeparator(parentId, command.data.index);
      break;
    default:
      this._log.error("_createCommand: Unknown item type: " + command.data.type);
      break;
    }
    if (newId)
      this._bms.setItemGUID(newId, command.GUID);
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
      this._log.warn("Attempted to remove item " + command.GUID +
                     ", but it does not exist.  Skipping.");
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
      this._log.warn("Attempted to edit root node (" + command.GUID +
                     ").  Skipping command.");
      return;
    }

    var itemId = this._bms.getItemIdForGUID(command.GUID);
    if (itemId < 0) {
      this._log.warn("Item for GUID " + command.GUID + " not found.  Skipping.");
      return;
    }

    for (let key in command.data) {
      switch (key) {
      case "type":
        
        
        break;
      case "GUID":
        var existing = this._getItemIdForGUID(command.data.GUID);
        if (existing < 0)
          this._bms.setItemGUID(itemId, command.data.GUID);
        else
          this._log.warn("Can't change GUID " + command.GUID +
                         " to " + command.data.GUID + ": GUID already exists.");
        break;
      case "title":
        this._bms.setItemTitle(itemId, command.data.title);
        break;
      case "URI":
        this._bms.changeBookmarkURI(itemId, Utils.makeURI(command.data.URI));
        break;
      case "index":
        this._bms.moveItem(itemId, this._bms.getFolderIdForItem(itemId),
                           command.data.index);
        break;
      case "parentGUID": {
        let index = -1;
        if (command.data.index && command.data.index >= 0)
          index = command.data.index;
        this._bms.moveItem(
          itemId, this._getItemIdForGUID(command.data.parentGUID), index);
      } break;
      case "tags": {
        let tagsURI = this._bms.getBookmarkURI(itemId);
        this._ts.untagURI(tagsURI, null);
        this._ts.tagURI(tagsURI, command.data.tags);
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
      default:
        this._log.warn("Can't change item property: " + key);
        break;
      }
    }
  },

  _getNode: function BSS__getNode(folder) {
    let query = this._hsvc.getNewQuery();
    query.setFolders([folder], 1);
    return this._hsvc.executeQuery(query, this._hsvc.getNewQueryOptions()).root;
  },

  __wrap: function BSS___wrap(node, items, parentGUID, index, guidOverride) {
    let GUID, item;

    
    if (guidOverride) {
      GUID = guidOverride;
      item = {};
    } else {
      GUID = this._bms.getItemGUID(node.itemId);
      item = {parentGUID: parentGUID, index: index};
    }

    if (node.type == node.RESULT_TYPE_FOLDER) {
      if (this._ls.isLivemark(node.itemId)) {
        item.type = "livemark";
        let siteURI = this._ls.getSiteURI(node.itemId);
        let feedURI = this._ls.getFeedURI(node.itemId);
        item.siteURI = siteURI? siteURI.spec : "";
        item.feedURI = feedURI? feedURI.spec : "";

      } else if (this._ans.itemHasAnnotation(node.itemId,
                                             "weave/mounted-share-id")) {
	

        item.type = "mounted-share";
        item.title = node.title;
        item.mountId = this._ans.getItemAnnotation(node.itemId,
                                                   "weave/mounted-share-id");

      } else {
        item.type = "folder";
        node.QueryInterface(Ci.nsINavHistoryQueryResultNode);
        node.containerOpen = true;
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
        item.staticTitle = this._ans.getItemAnnotation(node.itemId, "bookmarks/staticTitle");
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

  _wrapMount: function BStore__wrapMount(node, id) {
    if (node.type != node.RESULT_TYPE_FOLDER)
      throw "Trying to wrap a non-folder mounted share";

    let GUID = this._bms.getItemGUID(node.itemId);
    let ret = {rootGUID: GUID, userid: id, snapshot: {}, folderNode: node};

    node.QueryInterface(Ci.nsINavHistoryQueryResultNode);
    node.containerOpen = true;
    for (var i = 0; i < node.childCount; i++) {
      this.__wrap(node.getChild(i), ret.snapshot, GUID, i);
    }

    
    for (let guid in ret.snapshot) {
      
      if (ret.snapshot[guid].type == "mounted-share")
        delete ret.snapshot[guid];
    }

    return ret;
  },

  _resetGUIDs: function BSS__resetGUIDs(node) {
    if (this._ans.itemHasAnnotation(node.itemId, "placesInternal/GUID"))
      this._ans.removeItemAnnotation(node.itemId, "placesInternal/GUID");

    if (node.type == node.RESULT_TYPE_FOLDER &&
        !this._ls.isLivemark(node.itemId)) {
      node.QueryInterface(Ci.nsINavHistoryQueryResultNode);
      node.containerOpen = true;
      for (var i = 0; i < node.childCount; i++) {
        this._resetGUIDs(node.getChild(i));
      }
    }
  },

  findIncomingShares: function BStore_findIncomingShares() {
    

    let ret = [];
    let a = this._ans.getItemsWithAnnotation(INCOMING_SHARED_ANNO, {});
    for (let i = 0; i < a.length; i++) {
      

      let id = this._ans.getItemAnnotation(a[i], INCOMING_SHARED_ANNO);
      ret.push(this._wrapMount(this._getNode(a[i]), id));
    }
    return ret;
  },

  wrap: function BStore_wrap() {
    var items = {};
    this._wrap(this._getNode(this._bms.bookmarksMenuFolder), items, "menu");
    this._wrap(this._getNode(this._bms.toolbarFolder), items, "toolbar");
    this._wrap(this._getNode(this._bms.unfiledBookmarksFolder), items, "unfiled");
    return items;
  },

  wipe: function BStore_wipe() {
    this._bms.removeFolderChildren(this._bms.bookmarksMenuFolder);
    this._bms.removeFolderChildren(this._bms.toolbarFolder);
    this._bms.removeFolderChildren(this._bms.unfiledBookmarksFolder);
  },

  resetGUIDs: function BStore_resetGUIDs() {
    this._resetGUIDs(this._getNode(this._bms.bookmarksMenuFolder));
    this._resetGUIDs(this._getNode(this._bms.toolbarFolder));
    this._resetGUIDs(this._getNode(this._bms.unfiledBookmarksFolder));
  }
};
BookmarksStore.prototype.__proto__ = new Store();












function BookmarksTracker() {
  this._init();
}
BookmarksTracker.prototype = {
  _logName: "BMTracker",

  
  onBeginUpdateBatch: function BMT_onBeginUpdateBatch() {

  },
  onEndUpdateBatch: function BMT_onEndUpdateBatch() {

  },
  onItemVisited: function BMT_onItemVisited() {

  },

  



  onItemAdded: function BMT_onEndUpdateBatch() {
    this._score += 4;
  },
  onItemRemoved: function BMT_onItemRemoved() {
    this._score += 4;
  },
  
  onItemChanged: function BMT_onItemChanged() {
    this._score += 2;
  },

  _init: function BMT__init() {
    this._log = Log4Moz.Service.getLogger("Service." + this._logName);
    this._score = 0;

    Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
    getService(Ci.nsINavBookmarksService).
    addObserver(this, false);
  }
}
BookmarksTracker.prototype.__proto__ = new Tracker();
