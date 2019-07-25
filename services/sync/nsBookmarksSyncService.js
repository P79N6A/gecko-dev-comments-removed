



































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
  },

  _wrapNode: function BSS__wrapNode(node) {
    var items = {};
    this._wrapNodeInternal(node, items, null, null);
    return items;
  },

  _wrapNodeInternal: function BSS__wrapNodeInternal(node, items, parentGuid, index) {
    var guid = this._bms.getItemGUID(node.itemId);
    var item = {type: node.type,
                parentGuid: parentGuid,
                index: index};

    if (node.type == node.RESULT_TYPE_FOLDER) {
      item.title = node.title;

      node.QueryInterface(Ci.nsINavHistoryQueryResultNode);
      node.containerOpen = true;

      for (var i = 0; i < node.childCount; i++) {
        this._wrapNodeInternal(node.getChild(i), items, guid, i);
      }
    } else if (node.type == node.RESULT_TYPE_SEPARATOR) {
    } else if (node.type == node.RESULT_TYPE_URI) {
      
      item.title = node.title;
      item.uri = node.uri;
    } else {
      
    }

    items[guid] = item;
  },

  
  
  
  
  _combineCommands: function BSS__combineCommands(commandList) {
    var newList = [];

    for (var i = 0; i < commandList.length; i++) {
      LOG("Combining command: " + uneval(commandList[i]));

      var action = commandList[i].action;
      var value = commandList[i].value;
      var path = commandList[i].path;

      
      if (path.length <= 1)
        continue;

      var guid = path.shift();
      var key = path.pop();

      if (path.length) {
        LOG("Warning: editing deep property - dropping");
        continue;
      }

      if (!newList.length ||
          newList[newList.length - 1].guid != guid ||
          newList[newList.length - 1].action != action)
        newList.push({action: action, guid: guid, data: {}});

      newList[newList.length - 1].data[key] = value;
    }
    return newList;
  },

  _nodeDepth: function BSS__nodeDepth(guid, depth) {
    let parent = this._snapshot[guid].parentGuid;
    if (parent == null)
      return depth;
    return this._nodeDepth(parent, ++depth);
  },

  
  
  
  
  
  
  _sortCommands: function BSS__sortCommands(commandList) {
    for (let i = 0; i < commandList.length; i++) {
      commandList[i].depth = this._nodeDepth(commandList[i].guid, 0);
    }
    commandList.sort(function compareNodes(a, b) {
      if (a.depth > b.depth)
        return 1;
      if (a.depth < b.depth)
        return -1;
      if (a.index > b.index)
        return -1;
      if (a.index < b.index)
        return 1;
      return 0; 
    });
    return commandList;
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

  _compareItems: function BSS__compareItems(node, data) {
    switch (data.type) {
    case 0:
      if (node &&
          node.type == node.RESULT_TYPE_URI &&
          node.uri == data.uri && node.title == data.title)
        return true;
      return false;
    case 6:
      if (node &&
          node.type == node.RESULT_TYPE_FOLDER &&
          node.title == data.title)
        return true;
      return false;
    case 7:
      if (node && node.type == node.RESULT_TYPE_SEPARATOR)
        return true;
      return false;
    default:
      LOG("_compareItems: Unknown item type: " + data.type);
      return false;
    }
  },

  
  
  
  _createCommand: function BSS__createCommand(command) {
    let newId;

    
    if (command.data.parentGuid == null) {
      this._bms.setItemGUID(this._bms.bookmarksRoot, command.guid);
      return;
    }

    let parentId = this._bms.getItemIdForGUID(command.data.parentGuid);
    if (parentId <= 0) {
      LOG("Warning: creating node with unknown parent -> reparenting to root");
      parentId = this._bms.bookmarksRoot;
    }
    let parent = this._getBookmarks(parentId);
    parent.QueryInterface(Ci.nsINavHistoryQueryResultNode);
    parent.containerOpen = true;

    let curItem;
    if (parent.childCount > command.data.index)
      curItem = parent.getChild(command.data.index);

    if (this._compareItems(curItem, command.data)) {
      LOG(" -> FIXME - skipping item (already exists)");
      this._bms.setItemGUID(curItem.itemId, command.guid);
      return;
    }

    LOG(" -> creating item");

    switch (command.data.type) {
    case 0:
      newId = this._bms.insertBookmark(parentId,
                                       makeURI(command.data.uri),
                                       command.data.index,
                                       command.data.title);
      break;
    case 6:
      newId = this._bms.createFolder(parentId,
                                     command.data.title,
                                     command.data.index);
      break;
    case 7:
      newId = this._bms.insertSeparator(parentId, command.data.index);
      break;
    default:
      LOG("_createCommand: Unknown item type: " + command.data.type);
      break;
    }
    if (newId)
      this._bms.setItemGUID(newId, command.guid);
  },

  _removeCommand: function BSS__removeCommand(command) {
    var itemId = this._bms.getItemIdForGUID(command.guid);
    var type = this._bms.getItemType(itemId);

    switch (type) {
    case this._bms.TYPE_BOOKMARK:
      
      LOG("  -> removing bookmark " + command.guid);
      this._bms.removeItem(itemId);
      break;
    case this._bms.TYPE_FOLDER:
      LOG("  -> removing folder " + command.guid);
      this._bms.removeFolder(itemId);
      break;
    case this._bms.TYPE_SEPARATOR:
      LOG("  -> removing separator " + command.guid);
      this._bms.removeItem(itemId);
      break;
    default:
      LOG("removeCommand: Unknown item type: " + type);
      break;
    }
  },

  _editCommand: function BSS__editCommand(command) {
    var itemId = this._bms.getItemIdForGUID(command.guid);
    var type = this._bms.getItemType(itemId);

    for (let key in command.data) {
      switch (key) {
      case "title":
        this._bms.setItemTitle(itemId, command.data.title);
        break;
      case "uri":
        this._bms.changeBookmarkURI(itemId, makeURI(command.data.uri));
        break;
      case "index":
        this._bms.moveItem(itemId, this._bms.getFolderIdForItem(itemId),
                           command.data.index);
        break;
      case "parentGuid":
        this._bms.moveItem(
          itemId, this._bms.getItemIdForGUID(command.data.parentGuid), -1);
        break;
      default:
        LOG("Warning: Can't change item property: " + key);
        break;
      }
    }
  },

  _getEdits: function BSS__getEdits(a, b) {
    
    if (a.type != b.type)
      return -1;

    let ret = {};
    for (prop in a) {
      if (a[prop] != b[prop])
        ret[prop] = b[prop];
    }

    

    return ret;
  },

  _nodeParents: function BSS__nodeParents(guid, tree) {
    return this._nodeParentsInt(guid, tree, []);
  },

  _nodeParentsInt: function BSS__nodeParentsInt(guid, tree, parents) {
    if (tree[guid].parentGuid == null)
      return parents;
    parents.push(tree[guid].parentGuid);
    return this._nodeParentsInt(tree[guid].parentGuid, tree, parents);
  },

  _detectUpdates: function BSS__detectUpdates(a, b) {
    let cmds = [];
    for (let guid in a) {
      if (guid in b) {
        let edits = this._getEdits(a[guid], b[guid]);
        if (edits == -1) 
          continue;
        if (edits == {}) 
          continue;
        let parents = this._nodeParents(guid, b);
        cmds.push({action: "edit", guid: guid,
                   depth: parents.length, parents: parents,
                   data: edits});
      } else {
        let parents = this._nodeParents(guid, a); 
        cmds.push({action: "remove", guid: guid,
                   depth: parents.length, parents: parents});
      }
    }
    for (let guid in b) {
      if (guid in a)
        continue;
      let parents = this._nodeParents(guid, b);
      cmds.push({action: "create", guid: guid,
                 depth: parents.length, parents: parents,
                 data: b[guid]});
    }
    return cmds;
  },

  _conflicts: function BSS__conflicts(a, b) {
    if ((a.depth < b.depth) &&
        (b.parents.indexOf(a.guid) >= 0) &&
        a.action == "remove")
      return true;
    if ((a.guid == b.guid) && a != b)
      return true;

    return false;
  },

  
  
  _commandLike: function BSS__commandLike(a, b) {
    if (!a || !b)
      return false;

    if (a.action != b.action)
      return false;

    switch (a.data.type) {
    case 0:
      if (b.data.type == a.data.type &&
          b.data.uri == a.data.uri &&
          b.data.title == a.data.title)
        return true;
      return false;
    case 6:
      if (b.data.type == a.data.type &&
          b.data.title == a.data.title)
        return true;
      return false;
    case 7:
      


      return false;
    default:
      LOG("_commandLike: Unknown item type: " + uneval(a));
      return false;
    }
  },

  _deepEquals: function BSS__commandEquals(a, b) {
    if (!a && !b)
      return true;
    if (!a || !b)
      return false;

    for (let key in a) {
      if (typeof(a[key]) == "object") {
        if (!typeof(b[key]) == "object")
          return false;
        if (!this._deepEquals(a[key], b[key]))
          return false;
      } else {
        if (a[key] != b[key])
          return false;
      }
    }
    return true;
  },

  _reconcile: function BSS__reconcile(listA, listB) {
    let propagations = [[], []];
    let conflicts = [[], []];

    for (let i = 0; i < listA.length; i++) {
      for (let j = 0; j < listB.length; j++) {
        if (this._commandLike(listA[i], listB[j]) ||
            this._deepEquals(listA[i], listB[j])) {
          delete listA[i];
          delete listB[j];
        }
      }
    }

    listA = listA.filter(function(elt) { return elt });
    listB = listB.filter(function(elt) { return elt });

    for (let i = 0; i < listA.length; i++) {
      for (let j = 0; j < listB.length; j++) {
        if (this._conflicts(listA[i], listB[j]) ||
            this._conflicts(listB[j], listA[i])) {
          if (conflicts[0].some(
            function(elt) { return elt.guid == listA[i].guid }))
            conflicts[0].push(listA[i]);
          if (conflicts[1].some(
            function(elt) { return elt.guid == listB[j].guid }))
            conflicts[1].push(listB[j]);
        }
      }
    }
    for (let i = 0; i < listA.length; i++) {
      
      if (!conflicts[0].some(
        function(elt) { return elt.guid == listA[i].guid }))
        propagations[1].push(listA[i]);
    }
    for (let j = 0; j < listB.length; j++) {
      
      if (!conflicts[1].some(
        function(elt) { return elt.guid == listB[j].guid }))
        propagations[0].push(listB[j]);
    }
    return {propagations: propagations, conflicts: conflicts};
  },

  _applyCommandsToObj: function BSS__applyCommandsToObj(commands, obj) {
    for (let i = 0; i < commands.length; i++) {
      switch (command.action) {
      case "create":
        obj[command.guid] = eval(uneval(command.data));
      case "edit":
        for (let prop in command.data) {
          obj[command.guid][prop] = command.data[prop];
        }
      case "remove":
        delete obj[command.guid];
        break;
      }
    }
  },

  _getBookmarks: function BMS__getBookmarks(folder) {
    if (!folder)
      folder = this._bms.bookmarksRoot;
    var query = this._hsvc.getNewQuery();
    query.setFolders([folder], 1);
    return this._hsvc.executeQuery(query, this._hsvc.getNewQueryOptions()).root;
  },

  
  
  
  
  
  
  

  _doSync: function BSS__doSync() {
    var generator = yield;
    var handlers = this._handlersForGenerator(generator);

    LOG("Beginning sync");

    try {
      
      
      var data;

      var localBookmarks = this._getBookmarks();
      var localJson = this._wrapNode(localBookmarks);
      LOG("local json: " + uneval(localJson));

      
      asyncRun(bind2(this, this._getServerData), handlers['complete'], localJson);
      var server = yield;

      LOG("server: " + uneval(server));
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
      var localUpdates = this._detectUpdates(this._snapshot, localJson);
      LOG("updates: " + uneval(localUpdates));
      if (!(server['status'] == 1 || localUpdates.length > 0)) {
        LOG("Sync complete (1): no changes needed on client or server");
        return;
      }
	  
      

      var propagations = [server['updates'], localUpdates];
      var conflicts;

      if (server['status'] == 1 && localUpdates.length > 0) {
        LOG("Reconciling updates");
        var ret = this._reconcile(localUpdates, server['updates']);
        propagations = ret.propagations;
        conflicts = ret.conflicts;
      }

      LOG("Propagations: " + uneval(propagations) + "\n");
      LOG("Conflicts: " + uneval(conflicts) + "\n");
	  
      this._snapshotVersion = server['version'];

      if (!((propagations[0] && propagations[0].length) ||
            (propagations[1] && propagations[1].length) ||
            (conflicts &&
             (conflicts[0] && conflicts[0].length) ||
             (conflicts[1] && conflicts[1].length)))) {
        this._snapshot = this._wrapNode(localBookmarks);
        LOG("Sync complete (2): no changes needed on client or server");
        return;
      }

      if (conflicts && conflicts[0] && conflicts[0].length) {
        
        LOG("\nWARNING: Conflicts found, but we don't resolve conflicts yet!\n");
        LOG("Conflicts(1) " + uneval(this._combineCommands(conflicts[0])));
      }

      if (conflicts && conflicts[1] && conflicts[1].length) {
        
        LOG("\nWARNING: Conflicts found, but we don't resolve conflicts yet!\n");
        LOG("Conflicts(2) " + uneval(this._combineCommands(conflicts[1])));
      }

      
      if (propagations[0] && propagations[0].length) {
        LOG("Applying changes locally");
        
        this._snapshot = this._wrapNode(localBookmarks);
        this._applyCommandsToObj(this._snapshot, propagations[0]);
        
        
        
        
        
        this._applyCommands(propagations[0]);
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

      
      LOG("[sync bowels] local version: " + this._snapshotVersion);
      for (var z in ret.deltas) {
        LOG("[sync bowels] remote version: " + z);
      }
      LOG("foo: " + uneval(ret.deltas[this._snapshotVersion + 1]));
      if (ret.deltas[this._snapshotVersion + 1])
        LOG("-> is true");
      else
        LOG("-> is false");

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
          this._applyCommandsToObj(tmp, ret.deltas[keys[i]]);
        }
        ret.status = 1;
        ret.updates = this._detectUpdates(this._snapshot, tmp);

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
        this._applyCommandsToObj(tmp, data.updates);

        

        var keys = [];
        for (var v in ret.deltas) {
          if (v > this._snapshotVersion)
            keys.push(v);
          if (v > ret.version)
            ret.version = v;
        }
        keys = keys.sort();
        for (var i = 0; i < keys.length; i++) {
          this._applyCommandsToObj(tmp, ret.deltas[keys[i]]);
        }

        ret.status = data.status;
        ret.updates = this._detectUpdates(this._snapshot, tmp);
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
      ret.updates = this._detectUpdates(this._snapshot, tmp.snapshot);
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
