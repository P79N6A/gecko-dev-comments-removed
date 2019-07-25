



































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

function BookmarksSyncService() { this._init(); }
BookmarksSyncService.prototype = {

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

  __ans: null,
  get _ans() {
    if (!this.__ans)
      this.__ans = Cc["@mozilla.org/browser/annotation-service;1"].
                   getService(Ci.nsIAnnotationService);
    return this.__ans;
  },

  
  _dav: null,

  
  _sync: {},

  
  _utils: {},

  
  
  _snapshot: {},
  _snapshotVersion: 0,

  _init: function BSS__init() {

    var serverUrl = "http://sync.server.url/";
    try {
      var branch = Cc["@mozilla.org/preferences-service;1"].
        getService(Ci.nsIPrefBranch);
      serverUrl = branch.getCharPref("browser.places.sync.serverUrl");
    }
    catch (ex) {  }
    LOG("Bookmarks sync server: " + serverUrl);
    this._dav = new DAVCollection(serverUrl);

    var jsLoader = Cc["@mozilla.org/moz/jssubscript-loader;1"].
      getService(Ci.mozIJSSubScriptLoader);
    jsLoader.loadSubScript("chrome://sync/content/sync-engine.js", this._sync);
  },

  _wrapNode: function BSS__wrapNode(node) {
    var items = {};
    this._wrapNodeInternal(node, items);

    
    var rootGuid = this._bms.getItemGUID(node.itemId);
    for (var wanted in items) {
      if (rootGuid == wanted)
        continue;
      var found = false;
      for (var parent in items) {
        if (items[parent].children && items[parent].children.indexOf(wanted) >= 0) {
          found = true;
          continue;
        }
      }
      if (!found) {
        LOG("wrapNode error: node has no parent (" + wanted + ")");
      }
    }

    return items;
  },

  _wrapNodeInternal: function BSS__wrapNodeInternal(node, items) {
    var guid = this._bms.getItemGUID(node.itemId);
    var item = {"type": node.type};

    if (node.type == node.RESULT_TYPE_FOLDER) {
      node.QueryInterface(Ci.nsINavHistoryQueryResultNode);
      var openState = node.containerOpen;
      node.containerOpen = true;
      var children = [];
      for (var i = 0; i < node.childCount; i++) {
        var child = node.getChild(i);
        this._wrapNodeInternal(child, items);
        children.push(this._bms.getItemGUID(child.itemId));
      }
      item["children"] = children;
      item["title"] = node.title;
      node.containerOpen = openState;
    } else if (node.type == node.RESULT_TYPE_SEPARATOR) {
    } else if (node.type == node.RESULT_TYPE_URI) {
      
      item["title"] = node.title;
      item["uri"] = node.uri;
    } else {
      
    }

    items[guid] = item;
  },

  
  
  _findItemParent: function BSS__findItemParent(itemGuid) {
    var parent;
    var index;
    for (var item in this._snapshot) {
      if (this._snapshot[item].children) {
        index = this._snapshot[item].children.indexOf(itemGuid);
        if (index >= 0) {
          parent = item;
          break;
        }
      }
    }
    return [parent, index];
  },

  _combineCommands: function BSS__combineCommands(commandList) {
    var newList = [];
    var lastObj;
    if (newList.length)
      lastObj = newList[newList.length - 1];

    for (var i = 0; i < commandList.length; i++) {
      LOG("Command: " + uneval(commandList[i]) + "\n");
      var action = commandList[i].action;
      var path = commandList[i].path;
      var guid = path.shift();

      
      
      if ((action == "create" || action == "remove") &&
          (!newList.length ||
           (lastObj && lastObj.guid != guid && lastObj.action != action))) {
        
        if (path.length != 1)
          continue;

        let [parent, index] = this._findItemParent(guid);
        if (!parent) {
          LOG("Warning: item has no parent!\n");
          continue;
        }

        newList.push({action: action,
                      guid: guid,
                      parentGuid: parent,
                      index: index,
                      data: this._snapshot[guid]});

      } else if (action == "edit") {
        
        if (path.length != 1) {
          LOG("Warning: editing deep property - dropping");
          continue;
        }

        if (!newList.length ||
            (lastObj && lastObj.guid != guid && lastObj.action != action))
          newList.push({action: action, guid: guid});

        var key = path[path.length - 1];
        var value = this._snapshot[guid][key];
        newList[newList.length - 1].data[key] = value;
      }
    }
    LOG("Combined list:\n");
    LOG(uneval(newList) + "\n");
    return newList;
  },

  _applyCommands: function BSS__applyCommands(commandList) {
    for (var i = 0; i < commandList.length; i++) {
      var command = commandList[i];
      LOG("Processing command: " + uneval(command));
      switch (command["action"]) {
      case "create":
        this._createCommand(command);
        break;
      case "remove":
        this._removeCommand(command);
        break;
      case "edit":
        this._editCommand(command);
        break;
      default:
        LOG("unknown action in command: " + command["action"]);
        break;
      }
    }
  },

  _createCommand: function BSS__createCommand(command) {
    var newId;
    var parentId = this._bms.getItemIdForGUID(command.parentGuid);
    switch (command.data.type) {
    case 0:
      LOG("  -> creating a bookmark: '" + command.data.title +
          "' -> " + command.data.uri);
      newId = this._bms.insertBookmark(parentId,
                                       makeURI(command.data.uri),
                                       command.index,
                                       command.data.title);
      break;
    case 6:
      LOG("  -> creating a folder: '" + command.data.title + "'");
      newId = this._bms.createFolder(parentId,
                                     command.data.title,
                                     command.index);
      break;
    case 7:
      LOG("  -> creating a separator");
      newId = this._bms.insertSeparator(parentId, command.index);
      break;
    default:
      LOG("createCommand: Unknown item type: " + command.data.type);
      break;
    }
    if (newId)
      this._bms.setItemGUID(newId, command.guid);
  },

  _removeCommand: function BSS__removeCommand(command) {
    var iid = this._bss.getItemIdForGUID(aCommand.guid);
    if (command["path"].length == 0) {
      LOG("removing item");
      switch (node.type) {
      case node.RESULT_TYPE_URI:
        
        this._bms.removeItem(node.itemId);
        break;
      case node.RESULT_TYPE_FOLDER:
        this._bms.removeFolder(node.itemId);
        break;
      case node.RESULT_TYPE_SEPARATOR:
        this._bms.removeItem(node.itemId);
        break;
      default:
        LOG("removeCommand: Unknown item type: " + node.type);
        break;
      }
    } else if (command["path"].shift() == "children") {
      if (command["path"].length == 0) {
        LOG("invalid command?");
        return;
      }

      var index = command["path"].shift();

      node.QueryInterface(Ci.nsINavHistoryQueryResultNode);
      var openState = node.containerOpen;
      node.containerOpen = true;
      this._removeCommand(node.getChild(index), command);
      node.containerOpen = openState;
    }
  },

  _editCommand: function BSS__editCommand(node, command) {
    switch (command["path"].shift()) {
    case "children":
      node.QueryInterface(Ci.nsINavHistoryQueryResultNode);
      var openState = node.containerOpen;
      node.containerOpen = true;
      this._editCommand(node.getChild(command["path"].shift()), command);
      node.containerOpen = openState;
      break;
    case "type":
      LOG("Can't change item type!"); 
      break;
    case "title":
      this._bms.setItemTitle(node.itemId, command["value"]);
      break;
    case "uri":
      this._bms.changeBookmarkURI(node.itemId, makeURI(command["value"]));
      break;
    }
  },

  
  _sanitizeCommands: function BSS__sanitizeCommands(hashes) {
    var commands = [];
    for (var i = 0; i < hashes.length; i++) {
      commands.push(new this._sync.Command(hashes[i]["action"],
                                           hashes[i]["path"],
                                           hashes[i]["value"]));
    }
    return commands;
  },

  _getLocalBookmarks: function BMS__getLocalBookmarks() {
    var query = this._hsvc.getNewQuery();
    query.setFolders([this._bms.bookmarksRoot], 1);
    return this._hsvc.executeQuery(query, this._hsvc.getNewQueryOptions()).root;
  },

  
  
  
  
  
  
  

  _doSync: function BSS__doSync() {
    var generator = yield;
    var handlers = this._handlersForGenerator(generator);

    LOG("Beginning sync");

    try {
      
      
      var data;

      var localBookmarks = this._getLocalBookmarks();
      var localJson = this._wrapNode(localBookmarks);

      
      asyncRun(bind2(this, this._getServerData), handlers['complete'], localJson);
      var server = yield;

      if (server['status'] == 2) {
        LOG("Sync complete");
        return;
      } else if (server['status'] != 0 && server['status'] != 1) {
        LOG("Sync error");
        return;
      }

      LOG("Local snapshot version: " + this._snapshotVersion);
      LOG("Latest server version: " + server['version']);

      

      LOG("Generating local updates");
      var localUpdates = this._sanitizeCommands(this._sync.detectUpdates(this._snapshot, localJson));
      if (!(server['status'] == 1 || localUpdates.length > 0)) {
        LOG("Sync complete (1): no changes needed on client or server");
        return;
      }
	  
      

      var propagations = [server['updates'], localUpdates];
      var conflicts;

      if (server['status'] == 1 && localUpdates.length > 0) {
        LOG("Reconciling updates");
        var ret = this._sync.reconcile([localUpdates, server['updates']]);
        propagations = ret.propagations;
        conflicts = ret.conflicts;
      }

      LOG("\n" + uneval(conflicts) + "\n");
	  
      this._snapshotVersion = server['version'];

      LOG(uneval(propagations));
      if (!((propagations[0] && propagations[0].length > 0) ||
            (propagations[1] && propagations[1].length > 0))) {
        this._snapshot = this._wrapNode(localBookmarks);
        LOG("Sync complete (2): no changes needed on client or server");
        return;
      }

      
      if (propagations[0] && propagations[0].length > 0) {
        LOG("Applying changes locally");
        localBookmarks = this._getLocalBookmarks(); 
        this._snapshot = this._wrapNode(localBookmarks);
        
        this._sync.applyCommands(this._snapshot, eval(uneval(propagations[0])));
        var combinedCommands = this._combineCommands(propagations[0]);
        this._applyCommands(combinedCommands);
        this._snapshot = this._wrapNode(localBookmarks);
      }

      
      if (propagations[1] && propagations[1].length) {
        LOG("Uploading changes to server");
        this._snapshot = this._wrapNode(localBookmarks);
        this._snapshotVersion++;
        server['deltas'][this._snapshotVersion] = propagations[1];
        this._dav.PUT("bookmarks.delta", uneval(server['deltas']), handlers);
        data = yield;

        if (data.target.status >= 200 || data.target.status < 300)
          LOG("Successfully updated deltas on server");
        else
          LOG("Error: could not update deltas on server");
      }
      LOG("Sync complete");
    } finally {
      
      
    }
  },


  














  _getServerData: function BSS__getServerData(onComplete, localJson) {
    var generator = yield;
    var handlers = this._handlersForGenerator(generator);

    var ret = {status: -1, version: -1, deltas: null, updates: null};

    LOG("Getting bookmarks delta from server");
    this._dav.GET("bookmarks.delta", handlers);
    var data = yield;

    switch (data.target.status) {
    case 200:
      LOG("Got bookmarks delta from server");

      ret.deltas = eval(data.target.responseText);
      var tmp = eval(uneval(this._snapshot)); 

      if (ret.deltas[this._snapshotVersion + 1]) {
        
        var keys = [];
        for (var v in ret.deltas) {
          if (v > this._snapshotVersion)
            keys.push(v);
          if (v > ret.version)
            ret.version = v;
        }
        keys = keys.sort();
        for (var i = 0; i < keys.length; i++) {
          this._sync.applyCommands(tmp, this._sanitizeCommands(ret.deltas[keys[i]]));
        }
        ret.status = 1;
        ret.updates = this._sync.detectUpdates(this._snapshot, tmp);

      } else if (ret.deltas[this._snapshotVersion]) {
        LOG("No changes from server");
        ret.status = 0;
        ret.version = this._snapshotVersion;
        ret.updates = [];

      } else {
        LOG("Server delta can't update from our snapshot version, getting full file");
        
        asyncRun(bind2(this, this._getServerUpdatesFull), handlers['complete'], localJson);
        data = yield;
        if (data.status == 2) {
          
          
          LOG("Error: Delta file on server, but snapshot file missing.  New snapshot uploaded, may be inconsistent with deltas!");
        }

        var tmp = eval(uneval(this._snapshot)); 
        this._sync.applyCommands(tmp, this._sanitizeCommands(data.updates));

        

        var keys = [];
        for (var v in ret.deltas) {
          if (v > this._snapshotVersion)
            keys.push(v);
          if (v > ret.version)
            ret.version = v;
        }
        keys = keys.sort();
        for (var i = 0; i < keys.length; i++) {
          this._sync.applyCommands(tmp, this._sanitizeCommands(ret.deltas[keys[i]]));
        }

        ret.status = data.status;
        ret.updates = this._sync.detectUpdates(this._snapshot, tmp);
        ret.version = data.version;
        var keys = [];
        for (var v in ret.deltas) {
          if (v > ret.version)
            ret.version = v;
        }
      }
      break;
    case 404:
      LOG("Server has no delta file.  Getting full bookmarks file from server");
      
      asyncRun(bind2(this, this._getServerUpdatesFull), handlers['complete'], localJson);
      ret = yield;
      ret.deltas = {};
      break;
    default:
      LOG("Could not get bookmarks.delta: unknown HTTP status code " + data.target.status);
      break;
    }
    onComplete(ret);
  },

  _getServerUpdatesFull: function BSS__getServerUpdatesFull(onComplete, localJson) {
    var generator = yield;
    var handlers = this._handlersForGenerator(generator);

    var ret = {status: -1, version: -1, updates: null};

    this._dav.GET("bookmarks.json", handlers);
    data = yield;

    switch (data.target.status) {
    case 200:
      LOG("Got full bookmarks file from server");
      var tmp = eval(data.target.responseText);
      ret.status = 1;
      ret.updates = this._sync.detectUpdates(this._snapshot, tmp.snapshot);
      ret.version = tmp.version;
      break;
    case 404:
      LOG("No bookmarks on server.  Starting initial sync to server");

      this._snapshot = localJson;
      this._snapshotVersion = 1;
      this._dav.PUT("bookmarks.json", uneval({version: 1, snapshot: this._snapshot}), handlers);
      data = yield;

      if (data.target.status >= 200 || data.target.status < 300) {
        LOG("Initial sync to server successful");
        ret.status = 2;
      } else {
        LOG("Initial sync to server failed");
      }
      break;
    default:
      LOG("Could not get bookmarks.json: unknown HTTP status code " + data.target.status);
      break;
    }
    onComplete(ret);
  },

  _handlersForGenerator: function BSS__handlersForGenerator(generator) {
    var h = {load: bind2(this, function(event) { handleEvent(generator, event); }),
             error: bind2(this, function(event) { LOG("Request failed: " + uneval(event)); })};
    h['complete'] = h['load'];
    return h;
  },

  
  interfaces: [Ci.nsIBookmarksSyncService, Ci.nsISupports],

  

  
  classDescription: "Bookmarks Sync Service",
  contractID: "@mozilla.org/places/sync-service;1",
  classID: Components.ID("{6efd73bf-6a5a-404f-9246-f70a1286a3d6}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIBookmarksSyncService, Ci.nsISupports]),

  

  sync: function BSS_sync() { asyncRun(bind2(this, this._doSync)); }
};

function asyncRun(func, handler, data) {
  var generator = func(handler, data);
  generator.next();
  generator.send(generator);
}

function handleEvent(generator, data) {
  try { generator.send(data); }
  catch (e) {
    if (e instanceof StopIteration)
      generator = null;
    else
	throw e;
  }
}

function EventListener(handler) {
  this._handler = handler;
}
EventListener.prototype = {
  handleEvent: function EL_handleEvent(event) {
    this._handler(event);
  }
};

function DAVCollection(baseUrl) {
  this._baseUrl = baseUrl;
}
DAVCollection.prototype = {
  _addHandler: function DC__addHandler(request, handlers, eventName) {
    if (handlers[eventName])
      request.addEventListener(eventName, new EventListener(handlers[eventName]), false);
  },
  _makeRequest: function DC__makeRequest(op, path, handlers, headers) {
    var request = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"].createInstance();
    request = request.QueryInterface(Ci.nsIDOMEventTarget);
  
    if (!handlers)
      handlers = {};
    this._addHandler(request, handlers, "load");
    this._addHandler(request, handlers, "error");
  
    request = request.QueryInterface(Ci.nsIXMLHttpRequest);
    request.open(op, this._baseUrl + path, true);
  
    if (headers) {
      for (var key in headers) {
        request.setRequestHeader(key, headers[key]);
      }
    }

    return request;
  },
  GET: function DC_GET(path, handlers, headers) {
    if (!headers)
      headers = {'Content-type': 'text/plain'};
    var request = this._makeRequest("GET", path, handlers, headers);
    request.send(null);
  },
  PUT: function DC_PUT(path, data, handlers, headers) {
    if (!headers)
      headers = {'Content-type': 'text/plain'};
    var request = this._makeRequest("PUT", path, handlers, headers);
    request.send(data);
  },
  _runLockHandler: function DC__runLockHandler(name, event) {
    if (this._lockHandlers && this._lockHandlers[name])
      this._lockHandlers[name](event);
  },
  
  lock: function DC_lock(handlers) {
    this._lockHandlers = handlers;
    internalHandlers = {load: bind2(this, this._onLock),
                        error: bind2(this, this._onLockError)};
    headers = {'Content-Type': 'text/xml; charset="utf-8"'};
    var request = this._makeRequest("LOCK", "", internalHandlers, headers);
    request.send("<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n" +
                 "<D:lockinfo xmlns:D=\"DAV:\">\n" +
                 "  <D:locktype><D:write/></D:locktype>\n" +
                 "  <D:lockscope><D:exclusive/></D:lockscope>\n" +
                 "</D:lockinfo>");
  },
  _onLock: function DC__onLock(event) {
    LOG("acquired lock (" + event.target.status + "):\n" + event.target.responseText + "\n");
    this._token = "woo";
    this._runLockHandler("load", event);
  },
  _onLockError: function DC__onLockError(event) {
    LOG("lock failed (" + event.target.status + "):\n" + event.target.responseText + "\n");
    this._runLockHandler("error", event);
  },
  
  unlock: function DC_unlock(handlers) {
    this._lockHandlers = handlers;
    internalHandlers = {load: bind2(this, this._onUnlock),
                        error: bind2(this, this._onUnlockError)};
    headers = {'Lock-Token': "<" + this._token + ">"};
    var request = this._makeRequest("UNLOCK", "", internalHandlers, headers);
    request.send(null);
  },
  _onUnlock: function DC__onUnlock(event) {
    LOG("removed lock (" + event.target.status + "):\n" + event.target.responseText + "\n");
    this._token = null;
    this._runLockHandler("load", event);
  },
  _onUnlockError: function DC__onUnlockError(event) {
    LOG("unlock failed (" + event.target.status + "):\n" + event.target.responseText + "\n");
    this._runLockHandler("error", event);
  },
};

function makeFile(path) {
  var file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
  file.initWithPath(path);
  return file;
}

function makeURI(uriString) {
  var ioservice = Cc["@mozilla.org/network/io-service;1"].
                  getService(Ci.nsIIOService);
  return ioservice.newURI(uriString, null, null);
}

function bind2(object, method) {
  return function innerBind() { return method.apply(object, arguments); }
}

function LOG(aText) {
  dump(aText + "\n");
  var consoleService = Cc["@mozilla.org/consoleservice;1"].
                       getService(Ci.nsIConsoleService);
  consoleService.logStringMessage(aText);
}

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([BookmarksSyncService]);
}
