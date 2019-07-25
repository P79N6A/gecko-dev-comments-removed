



































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

const MODE_RDONLY   = 0x01;
const MODE_WRONLY   = 0x02;
const MODE_CREATE   = 0x08;
const MODE_APPEND   = 0x10;
const MODE_TRUNCATE = 0x20;

const PERMS_FILE      = 0644;
const PERMS_DIRECTORY = 0755;

const STORAGE_FORMAT_VERSION = 0;

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

  __ts: null,
  get _ts() {
    if (!this.__ts)
      this.__ts = Cc["@mozilla.org/browser/tagging-service;1"].
                  getService(Ci.nsITaggingService);
    return this.__ts;
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

  __os: null,
  get _os() {
    if (!this.__os)
      this.__os = Cc["@mozilla.org/observer-service;1"]
        .getService(Ci.nsIObserverService);
    return this.__os;
  },

  __console: null,
  get _console() {
    if (!this.__console)
      this.__console = Cc["@mozilla.org/consoleservice;1"]
        .getService(Ci.nsIConsoleService);
    return this.__console;
  },

  __dirSvc: null,
  get _dirSvc() {
    if (!this.__dirSvc)
      this.__dirSvc = Cc["@mozilla.org/file/directory_service;1"].
        getService(Ci.nsIProperties);
    return this.__dirSvc;
  },

  
  _log: null,

  
  _timer: null,

  
  _dav: null,

  
  
  _snapshot: {},
  _snapshotVersion: 0,

  __snapshotGUID: null,
  get _snapshotGUID() {
    if (!this.__snapshotGUID) {
      let uuidgen = Cc["@mozilla.org/uuid-generator;1"].
        getService(Ci.nsIUUIDGenerator);
      this.__snapshotGUID = uuidgen.generateUUID().toString().replace(/[{}]/g, '');
    }
    return this.__snapshotGUID;
  },
  set _snapshotGUID(GUID) {
    this.__snapshotGUID = GUID;
  },

  get currentUser() {
    return this._dav.currentUser;
  },

  _init: function BSS__init() {
    this._initLog();
    let serverURL = 'https://dotmoz.mozilla.org/';
    try {
      let branch = Cc["@mozilla.org/preferences-service;1"].
        getService(Ci.nsIPrefBranch);
      serverURL = branch.getCharPref("browser.places.sync.serverURL");
    }
    catch (ex) {  }
    this.notice("Bookmarks login server: " + serverURL);
    this._dav = new DAVCollection(serverURL);
    this._readSnapshot();
  },

  _initLog: function BSS__initLog() {
    let dirSvc = Cc["@mozilla.org/file/directory_service;1"].
    getService(Ci.nsIProperties);

    let file = dirSvc.get("ProfD", Ci.nsIFile);
    file.append("bm-sync.log");
    file.QueryInterface(Ci.nsILocalFile);

    if (file.exists())
      file.remove(false);
    file.create(file.NORMAL_FILE_TYPE, PERMS_FILE);

    this._log = Cc["@mozilla.org/network/file-output-stream;1"]
      .createInstance(Ci.nsIFileOutputStream);
    let flags = MODE_WRONLY | MODE_CREATE | MODE_APPEND;
    this._log.init(file, flags, PERMS_FILE, 0);

    let str = "Bookmarks Sync Log\n------------------\n\n";
    this._log.write(str, str.length);
    this._log.flush();
  },

  _saveSnapshot: function BSS__saveSnapshot() {
    this.notice("Saving snapshot to disk");

    let dirSvc = Cc["@mozilla.org/file/directory_service;1"].
      getService(Ci.nsIProperties);

    let file = dirSvc.get("ProfD", Ci.nsIFile);
    file.append("bm-sync-snapshot.json");
    file.QueryInterface(Ci.nsILocalFile);

    if (!file.exists())
      file.create(file.NORMAL_FILE_TYPE, PERMS_FILE);

    let fos = Cc["@mozilla.org/network/file-output-stream;1"].
      createInstance(Ci.nsIFileOutputStream);
    let flags = MODE_WRONLY | MODE_CREATE | MODE_TRUNCATE;
    fos.init(file, flags, PERMS_FILE, 0);

    let out = {version: this._snapshotVersion,
               GUID: this._snapshotGUID,
               snapshot: this._snapshot};
    out = uneval(out);
    fos.write(out, out.length);
    fos.close();
  },

  _readSnapshot: function BSS__readSnapshot() {
    let dirSvc = Cc["@mozilla.org/file/directory_service;1"].
      getService(Ci.nsIProperties);

    let file = dirSvc.get("ProfD", Ci.nsIFile);
    file.append("bm-sync-snapshot.json");

    if (!file.exists())
      return;

    let fis = Cc["@mozilla.org/network/file-input-stream;1"].
      createInstance(Ci.nsIFileInputStream);
    fis.init(file, MODE_RDONLY, PERMS_FILE, 0);
    fis.QueryInterface(Ci.nsILineInputStream);

    let json = "";
    while (fis.available()) {
      let ret = {};
      fis.readLine(ret);
      json += ret.value;
    }
    fis.close();
    json = eval(json);

    if (json && json.snapshot && json.version && json.GUID) {
      this._snapshot = json.snapshot;
      this._snapshotVersion = json.version;
      this._snapshotGUID = json.GUID;
    }
  },

  _wrapNode: function BSS__wrapNode(node) {
    var items = {};
    this._wrapNodeInternal(node, items, null, null);
    return items;
  },

  _wrapNodeInternal: function BSS__wrapNodeInternal(node, items, parentGUID, index) {
    let GUID = this._bms.getItemGUID(node.itemId);
    let item = {parentGUID: parentGUID,
                index: index};

    if (node.type == node.RESULT_TYPE_FOLDER) {
      if (this._ls.isLivemark(node.itemId)) {
        item.type = "livemark";
        item.siteURI = this._ls.getSiteURI(node.itemId).spec;
        item.feedURI = this._ls.getFeedURI(node.itemId).spec;
      } else {
        item.type = "folder";
        node.QueryInterface(Ci.nsINavHistoryQueryResultNode);
        node.containerOpen = true;
        for (var i = 0; i < node.childCount; i++) {
          this._wrapNodeInternal(node.getChild(i), items, GUID, i);
        }
      }
      item.title = node.title;
    } else if (node.type == node.RESULT_TYPE_URI) {
      if (this._ms.hasMicrosummary(node.itemId)) {
        item.type = "microsummary";
        let micsum = this._ms.getMicrosummary(node.itemId);
        item.generatorURI = micsum.generator.uri.spec; 
      } else {
        item.type = "bookmark";
        item.title = node.title;
      }
      item.URI = node.uri;
      item.tags = this._ts.getTagsForURI(makeURI(node.uri));
      let keyword = this._bms.getKeywordForBookmark(node.itemId);
      if (keyword)
        item.keyword = keyword;
    } else if (node.type == node.RESULT_TYPE_SEPARATOR) {
      item.type = "separator";
    } else {
      this.notice("Warning: unknown item type, cannot serialize: " + node.type);
      return;
    }

    items[GUID] = item;
  },

  
  _getEdits: function BSS__getEdits(a, b) {
    
    if (a.type != b.type)
      return -1;

    let ret = {numProps: 0, props: {}};
    for (prop in a) {
      if (!this._deepEquals(a[prop], b[prop])) {
        ret.numProps++;
        ret.props[prop] = b[prop];
      }
    }

    

    return ret;
  },

  _nodeParents: function BSS__nodeParents(GUID, tree) {
    return this._nodeParentsInt(GUID, tree, []);
  },

  _nodeParentsInt: function BSS__nodeParentsInt(GUID, tree, parents) {
    if (!tree[GUID] || !tree[GUID].parentGUID)
      return parents;
    parents.push(tree[GUID].parentGUID);
    return this._nodeParentsInt(tree[GUID].parentGUID, tree, parents);
  },

  _detectUpdates: function BSS__detectUpdates(a, b) {
    let cmds = [];
    for (let GUID in a) {
      if (GUID in b) {
        let edits = this._getEdits(a[GUID], b[GUID]);
        if (edits == -1) 
          continue;
        if (edits.numProps == 0) 
          continue;
        let parents = this._nodeParents(GUID, b);
        cmds.push({action: "edit", GUID: GUID,
                   depth: parents.length, parents: parents,
                   data: edits.props});
      } else {
        let parents = this._nodeParents(GUID, a); 
        cmds.push({action: "remove", GUID: GUID,
                   depth: parents.length, parents: parents});
      }
    }
    for (let GUID in b) {
      if (GUID in a)
        continue;
      let parents = this._nodeParents(GUID, b);
      cmds.push({action: "create", GUID: GUID,
                 depth: parents.length, parents: parents,
                 data: b[GUID]});
    }
    cmds.sort(function(a, b) {
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
    return cmds;
  },

  _conflicts: function BSS__conflicts(a, b) {
    if ((a.depth < b.depth) &&
        (b.parents.indexOf(a.GUID) >= 0) &&
        a.action == "remove")
      return true;
    if ((a.GUID == b.GUID) && !this._deepEquals(a, b))
      return true;
    return false;
  },

  
  
  
  _commandLike: function BSS__commandLike(a, b) {

    
    
    
    
    
    
    
    
    if (!a || !b ||
       a.action != b.action ||
       a.data.type != b.data.type ||
       a.parentGUID != b.parentGUID ||
       a.GUID == b.GUID)
      return false;

    switch (a.data.type) {
    case "bookmark":
      if (a.data.URI == b.data.URI &&
          a.data.title == b.data.title)
        return true;
      return false;
    case "microsummary":
      if (a.data.URI == b.data.URI &&
          a.data.generatorURI == b.data.generatorURI)
        return true;
      return false;
    case "folder":
      if (a.index == b.index &&
          a.data.title == b.data.title)
        return true;
      return false;
    case "livemark":
      if (a.data.title == b.data.title &&
          a.data.siteURI == b.data.siteURI &&
          a.data.feedURI == b.data.feedURI)
        return true;
      return false;
    case "separator":
      if (a.index == b.index)
        return true;
      return false;
    default:
      this.notice("_commandLike: Unknown item type: " + uneval(a));
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

  
  
  
  _fixParents: function BSS__fixParents(list, oldGUID, newGUID) {
    for (let i = 0; i < list.length; i++) {
      if (!list[i])
        continue;
      if (list[i].parentGUID == oldGUID)
        list[i].parentGUID = newGUID;
      for (let j = 0; j < list[i].parents.length; j++) {
        if (list[i].parents[j] == oldGUID)
          list[i].parents[j] = newGUID;
      }
    }
  },

  
  
  
  

  
  

  
  
  

  
  

  _reconcile: function BSS__reconcile(onComplete, listA, listB) {
    let generator = yield;
    this._timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    let handlers = generatorHandlers(generator);
    let listener = new EventListener(handlers['complete']);

    let propagations = [[], []];
    let conflicts = [[], []];

    for (let i = 0; i < listA.length; i++) {

      this._timer.initWithCallback(listener, 0, this._timer.TYPE_ONE_SHOT);
      yield; 

      for (let j = 0; j < listB.length; j++) {
        if (this._deepEquals(listA[i], listB[j])) {
          delete listA[i];
          delete listB[j];
        } else if (this._commandLike(listA[i], listB[j])) {
          
          if (this._bms.getItemIdForGUID(listB[j].GUID) >= 0)
            continue;
          this._fixParents(listA, listA[i].GUID, listB[j].GUID);
          listB[j].data = {GUID: listB[j].GUID};
          listB[j].GUID = listA[i].GUID;
          listB[j].action = "edit";
          delete listA[i];
        }
      }
    }

    listA = listA.filter(function(elt) { return elt });
    listB = listB.filter(function(elt) { return elt });

    for (let i = 0; i < listA.length; i++) {

      this._timer.initWithCallback(listener, 0, this._timer.TYPE_ONE_SHOT);
      yield; 

      for (let j = 0; j < listB.length; j++) {
        if (this._conflicts(listA[i], listB[j]) ||
            this._conflicts(listB[j], listA[i])) {
          if (!conflicts[0].some(
            function(elt) { return elt.GUID == listA[i].GUID }))
            conflicts[0].push(listA[i]);
          if (!conflicts[1].some(
            function(elt) { return elt.GUID == listB[j].GUID }))
            conflicts[1].push(listB[j]);
        }
      }
    }
    for (let i = 0; i < listA.length; i++) {
      
      if (!conflicts[0].some(
        function(elt) { return elt.GUID == listA[i].GUID }))
        propagations[1].push(listA[i]);
    }
    for (let j = 0; j < listB.length; j++) {
      
      if (!conflicts[1].some(
        function(elt) { return elt.GUID == listB[j].GUID }))
        propagations[0].push(listB[j]);
    }

    this._timer = null;
    let ret = {propagations: propagations, conflicts: conflicts};
    generatorDone(this, onComplete, ret);

    
    let cookie = yield;
    if (cookie != "generator shutdown")
      this.notice("_reconcile: Error: generator not properly shut down.")
  },

  _applyCommandsToObj: function BSS__applyCommandsToObj(commands, obj) {
    for (let i = 0; i < commands.length; i++) {
      this.notice("Applying cmd to obj: " + uneval(commands[i]));
      switch (commands[i].action) {
      case "create":
        obj[commands[i].GUID] = eval(uneval(commands[i].data));
        break;
      case "edit":
        for (let prop in commands[i].data) {
          obj[commands[i].GUID][prop] = commands[i].data[prop];
        }
        break;
      case "remove":
        delete obj[commands[i].GUID];
        break;
      }
    }
    return obj;
  },

  
  _applyCommands: function BSS__applyCommands(commandList) {
    for (var i = 0; i < commandList.length; i++) {
      var command = commandList[i];
      this.notice("Processing command: " + uneval(command));
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
        this.notice("unknown action in command: " + command["action"]);
        break;
      }
    }
  },

  _createCommand: function BSS__createCommand(command) {
    let newId;
    let parentId = this._bms.getItemIdForGUID(command.data.parentGUID);

    if (parentId <= 0) {
      this.notice("Warning: creating node with unknown parent -> reparenting to root");
      parentId = this._bms.bookmarksRoot;
    }

    this.notice(" -> creating item");

    switch (command.data.type) {
    case "bookmark":
    case "microsummary":
      let URI = makeURI(command.data.URI);
      newId = this._bms.insertBookmark(parentId,
                                       URI,
                                       command.data.index,
                                       command.data.title);
      this._ts.untagURI(URI, null);
      this._ts.tagURI(URI, command.data.tags);
      if (command.data.keyword)
        this._bms.setKeywordForBookmark(newId, command.data.keyword);

      if (command.data.type == "microsummary") {
        let genURI = makeURI(command.data.generatorURI);
        let micsum = this._ms.createMicrosummary(URI, genURI);
        this._ms.setMicrosummary(newId, micsum);
      }
      break;
    case "folder":
      newId = this._bms.createFolder(parentId,
                                     command.data.title,
                                     command.data.index);
      break;
    case "livemark":
      newId = this._ls.createLivemark(parentId,
                                      command.data.title,
                                      makeURI(command.data.siteURI),
                                      makeURI(command.data.feedURI),
                                      command.data.index);
      break;
    case "separator":
      newId = this._bms.insertSeparator(parentId, command.data.index);
      break;
    default:
      this.notice("_createCommand: Unknown item type: " + command.data.type);
      break;
    }
    if (newId)
      this._bms.setItemGUID(newId, command.GUID);
  },

  _removeCommand: function BSS__removeCommand(command) {
    var itemId = this._bms.getItemIdForGUID(command.GUID);
    var type = this._bms.getItemType(itemId);

    switch (type) {
    case this._bms.TYPE_BOOKMARK:
      this.notice("  -> removing bookmark " + command.GUID);
      this._bms.removeItem(itemId);
      break;
    case this._bms.TYPE_FOLDER:
      this.notice("  -> removing folder " + command.GUID);
      this._bms.removeFolder(itemId);
      break;
    case this._bms.TYPE_SEPARATOR:
      this.notice("  -> removing separator " + command.GUID);
      this._bms.removeItem(itemId);
      break;
    default:
      this.notice("removeCommand: Unknown item type: " + type);
      break;
    }
  },

  _editCommand: function BSS__editCommand(command) {
    var itemId = this._bms.getItemIdForGUID(command.GUID);
    if (itemId == -1) {
      this.notice("Warning: item for GUID " + command.GUID + " not found.  Skipping.");
      return;
    }

    for (let key in command.data) {
      switch (key) {
      case "GUID":
        var existing = this._bms.getItemIdForGUID(command.data.GUID);
        if (existing == -1)
          this._bms.setItemGUID(itemId, command.data.GUID);
        else
          this.notice("Warning: can't change GUID " + command.GUID +
              " to " + command.data.GUID + ": GUID already exists.");
        break;
      case "title":
        this._bms.setItemTitle(itemId, command.data.title);
        break;
      case "URI":
        this._bms.changeBookmarkURI(itemId, makeURI(command.data.URI));
        break;
      case "index":
        this._bms.moveItem(itemId, this._bms.getFolderIdForItem(itemId),
                           command.data.index);
        break;
      case "parentGUID":
        let index = -1;
        if (command.data.index && command.data.index >= 0)
          index = command.data.index;
        this._bms.moveItem(
          itemId, this._bms.getItemIdForGUID(command.data.parentGUID), index);
        break;
      case "tags":
        let tagsURI = this._bms.getBookmarkURI(itemId);
        this._ts.untagURI(URI, null);
        this._ts.tagURI(tagsURI, command.data.tags);
        break;
      case "keyword":
        this._bms.setKeywordForBookmark(itemId, command.data.keyword);
        break;
      case "generatorURI":
        let micsumURI = makeURI(this._bms.getBookmarkURI(itemId));
        let genURI = makeURI(command.data.generatorURI);
        let micsum = this._ms.createMicrosummary(micsumURI, genURI);
        this._ms.setMicrosummary(itemId, micsum);
        break;
      case "siteURI":
        this._ls.setSiteURI(itemId, makeURI(command.data.siteURI));
        break;
      case "feedURI":
        this._ls.setFeedURI(itemId, makeURI(command.data.feedURI));
        break;
      default:
        this.notice("Warning: Can't change item property: " + key);
        break;
      }
    }
  },

  _getBookmarks: function BMS__getBookmarks(folder) {
    if (!folder)
      folder = this._bms.bookmarksRoot;
    var query = this._hsvc.getNewQuery();
    query.setFolders([folder], 1);
    let root = this._hsvc.executeQuery(query, this._hsvc.getNewQueryOptions()).root;
    return this._wrapNode(root);
  },

  _mungeNodes: function BSS__mungeNodes(nodes) {
    let json = uneval(nodes);
    json = json.replace(/:{type/g, ":\n\t{type");
    json = json.replace(/}, /g, "},\n  ");
    json = json.replace(/, parentGUID/g, ",\n\t parentGUID");
    json = json.replace(/, index/g, ",\n\t index");
    json = json.replace(/, title/g, ",\n\t title");
    json = json.replace(/, URI/g, ",\n\t URI");
    json = json.replace(/, tags/g, ",\n\t tags");
    json = json.replace(/, keyword/g, ",\n\t keyword");
    return json;
  },

  _mungeCommands: function BSS__mungeCommands(commands) {
    let json = uneval(commands);
    json = json.replace(/ {action/g, "\n {action");
    
    return json;
  },

  _mungeConflicts: function BSS__mungeConflicts(conflicts) {
    let json = uneval(conflicts);
    json = json.replace(/ {action/g, "\n {action");
    
    return json;
  },

  
  
  
  
  
  
  

  _doSync: function BSS__doSync() {
    var generator = yield;
    var handlers = generatorHandlers(generator);

    this._os.notifyObservers(null, "bookmarks-sync:start", "");
    this.notice("Beginning sync");

    try {
      if (!asyncRun(this._dav, this._dav.lock, handlers.complete))
        return;
      let locked = yield;

      if (locked)
        this.notice("Lock acquired");
      else {
        this.notice("Could not acquire lock, aborting sync");
        return;
      }

      var localJson = this._getBookmarks();
      

      
      if (asyncRun(this, this._getServerData, handlers.complete, localJson))
        let server = yield;
      else
        return

      this.notice("Local snapshot version: " + this._snapshotVersion);
      this.notice("Server status: " + server.status);

      if (server['status'] != 0) {
        this._os.notifyObservers(null, "bookmarks-sync:end", "");
        this.notice("Sync error: could not get server status, " +
                    "or initial upload failed.");
        return;
      }

      this.notice("Server maxVersion: " + server.maxVersion);
      this.notice("Server snapVersion: " + server.snapVersion);
      this.notice("Server updates: " + this._mungeCommands(server.updates));

      

      let localUpdates = this._detectUpdates(this._snapshot, localJson);
      this.notice("Local updates: " + this._mungeCommands(localUpdates));

      if (server.updates.length == 0 && localUpdates.length == 0) {
        this._snapshotVersion = server.maxVersion;
        this._os.notifyObservers(null, "bookmarks-sync:end", "");
        this.notice("Sync complete (1): no changes needed on client or server");
        return;
      }
	  
      

      this.notice("Reconciling client/server updates");
      if (asyncRun(this, this._reconcile, handlers.complete,
                   localUpdates, server.updates))
        let ret = yield;
      else
        return
      
      

      let clientChanges = [];
      let serverChanges = [];
      let clientConflicts = [];
      let serverConflicts = [];

      if (ret.propagations[0])
        clientChanges = ret.propagations[0];
      if (ret.propagations[1])
        serverChanges = ret.propagations[1];

      if (ret.conflicts && ret.conflicts[0])
        clientConflicts = ret.conflicts[0];
      if (ret.conflicts && ret.conflicts[1])
        serverConflicts = ret.conflicts[1];

      this.notice("Changes for client: " + this._mungeCommands(clientChanges));
      this.notice("Changes for server: " + this._mungeCommands(serverChanges));
      this.notice("Client conflicts: " + this._mungeConflicts(clientConflicts));
      this.notice("Server conflicts: " + this._mungeConflicts(serverConflicts));

      if (!(clientChanges.length || serverChanges.length ||
            clientConflicts.length || serverConflicts.length)) {
        this._os.notifyObservers(null, "bookmarks-sync:end", "");
        this.notice("Sync complete (2): no changes needed on client or server");
        this._snapshot = localJson;
        this._snapshotVersion = server.maxVersion;
        this._saveSnapshot();
        return;
      }

      if (clientConflicts.length || serverConflicts.length) {
        this.notice("\nWARNING: Conflicts found, but we don't resolve conflicts yet!\n");
      }

      
      if (clientChanges.length) {
        this.notice("Applying changes locally");
        
        

        this._snapshot = this._applyCommandsToObj(clientChanges, localJson);
        this._snapshotVersion = server.maxVersion;
        this._applyCommands(clientChanges);

        let newSnapshot = this._getBookmarks();
        let diff = this._detectUpdates(this._snapshot, newSnapshot);
        if (diff.length != 0) {
          this.notice("Error: commands did not apply correctly.  Diff:\n" +
                      uneval(diff));
          this._snapshot = newSnapshot;
          
        }
        this._saveSnapshot();
      }

      
      if (serverChanges.length) {
        this.notice("Uploading changes to server");
        this._snapshot = this._getBookmarks();
        this._snapshotVersion = ++server.maxVersion;
        server.deltas.push(serverChanges);

        this._dav.PUT("bookmarks-deltas.json", uneval(server.deltas), handlers);
        let deltasPut = yield;

        
        
        this._dav.PUT("bookmarks-status.json",
                      uneval({GUID: this._snapshotGUID,
                              formatVersion: STORAGE_FORMAT_VERSION,
                              snapVersion: server.snapVersion,
                              maxVersion: this._snapshotVersion}), handlers);
        let statusPut = yield;

        if (deltasPut.target.status >= 200 && deltasPut.target.status < 300 &&
            statusPut.target.status >= 200 && statusPut.target.status < 300) {
          this.notice("Successfully updated deltas and status on server");
          this._saveSnapshot();
        } else {
          
          
          this.notice("Error: could not update deltas on server");
        }
      }
      this._os.notifyObservers(null, "bookmarks-sync:end", "");
      this.notice("Sync complete");
    } finally {
      if (!asyncRun(this._dav, this._dav.unlock, handlers.complete))
        return;
      let unlocked = yield;
      if (unlocked)
        this.notice("Lock released");
      else
        this.notice("Error: could not unlock DAV collection");
      this._os.notifyObservers(null, "bookmarks-sync:end", "");
    }
  },

  



















  _getServerData: function BSS__getServerData(onComplete, localJson) {
    let generator = yield;
    let handlers = generatorHandlers(generator);

    let ret = {status: -1,
               formatVersion: null, maxVersion: null, snapVersion: null,
               snapshot: null, deltas: null, updates: null};

    this.notice("Getting bookmarks status from server");
    this._dav.GET("bookmarks-status.json", handlers);
    let statusResp = yield;

    switch (statusResp.target.status) {
    case 200:
      this.notice("Got bookmarks status from server");

      let status = eval(statusResp.target.responseText);
      let snap, deltas, allDeltas;

      
      if (status.formatVersion > STORAGE_FORMAT_VERSION) {
        this.notice("Error: server uses storage format v" + status.formatVersion +
                    ", this client understands up to v" + STORAGE_FORMAT_VERSION);
        generatorDone(this, onComplete, ret)
        return;
      }

      if (status.GUID != this._snapshotGUID) {
        this.notice("Remote/local sync GUIDs do not match.  " +
                    "Forcing initial sync.");
        this._snapshot = {};
        this._snapshotVersion = -1;
        this._snapshotGUID = status.GUID;
      }

      if (this._snapshotVersion < status.snapVersion) {
        if (this._snapshotVersion >= 0)
          this.notice("Local snapshot is out of date");

        this.notice("Downloading server snapshot");
        this._dav.GET("bookmarks-snapshot.json", handlers);
        let snapResp = yield;
        if (snapResp.target.status < 200 || snapResp.target.status >= 300) {
          this.notice("Error: could not download server snapshot");
          generatorDone(this, onComplete, ret)
          return;
        }
        snap = eval(snapResp.target.responseText);

        this.notice("Downloading server deltas");
        this._dav.GET("bookmarks-deltas.json", handlers);
        let deltasResp = yield;
        if (deltasResp.target.status < 200 || deltasResp.target.status >= 300) {
          this.notice("Error: could not download server deltas");
          generatorDone(this, onComplete, ret)
          return;
        }
        allDeltas = eval(deltasResp.target.responseText);
        deltas = eval(uneval(allDeltas));

      } else if (this._snapshotVersion >= status.snapVersion &&
                 this._snapshotVersion < status.maxVersion) {
        snap = eval(uneval(this._snapshot));

        this.notice("Downloading server deltas");
        this._dav.GET("bookmarks-deltas.json", handlers);
        let deltasResp = yield;
        if (deltasResp.target.status < 200 || deltasResp.target.status >= 300) {
          this.notice("Error: could not download server deltas");
          generatorDone(this, onComplete, ret)
          return;
        }
        allDeltas = eval(deltasResp.target.responseText);
        deltas = allDeltas.slice(this._snapshotVersion - status.snapVersion);

      } else if (this._snapshotVersion == status.maxVersion) {
        snap = eval(uneval(this._snapshot));

        
        this.notice("Downloading server deltas");
        this._dav.GET("bookmarks-deltas.json", handlers);
        let deltasResp = yield;
        if (deltasResp.target.status < 200 || deltasResp.target.status >= 300) {
          this.notice("Error: could not download server deltas");
          generatorDone(this, onComplete, ret)
          return;
        }
        allDeltas = eval(deltasResp.target.responseText);
        deltas = [];

      } else { 
        this.notice("Error: server snapshot is older than local snapshot");
        generatorDone(this, onComplete, ret)
        return;
      }

      for (var i = 0; i < deltas.length; i++) {
        snap = this._applyCommandsToObj(deltas[i], snap);
      }

      ret.status = 0;
      ret.formatVersion = status.formatVersion;
      ret.maxVersion = status.maxVersion;
      ret.snapVersion = status.snapVersion;
      ret.snapshot = snap;
      ret.deltas = allDeltas;
      ret.updates = this._detectUpdates(this._snapshot, snap);
      break;

    case 404:
      this.notice("Server has no status file, Initial upload to server");

      this._snapshot = localJson;
      this._snapshotVersion = 0;
      this._snapshotGUID = null; 

      this._dav.PUT("bookmarks-snapshot.json",
                    uneval(this._snapshot), handlers);
      let snapPut = yield;
      if (snapPut.target.status < 200 || snapPut.target.status >= 300) {
        this.notice("Error: could not upload snapshot to server, error code: " +
                    snapPut.target.status);
        generatorDone(this, onComplete, ret)
        return;
      }

      this._dav.PUT("bookmarks-deltas.json", uneval([]), handlers);
      let deltasPut = yield;
      if (deltasPut.target.status < 200 || deltasPut.target.status >= 300) {
        this.notice("Error: could not upload deltas to server, error code: " +
                   deltasPut.target.status);
        generatorDone(this, onComplete, ret)
        return;
      }

      this._dav.PUT("bookmarks-status.json",
                    uneval({GUID: this._snapshotGUID,
                            formatVersion: STORAGE_FORMAT_VERSION,
                            snapVersion: this._snapshotVersion,
                            maxVersion: this._snapshotVersion}), handlers);
      let statusPut = yield;
      if (statusPut.target.status < 200 || statusPut.target.status >= 300) {
        this.notice("Error: could not upload status file to server, error code: " +
                   statusPut.target.status);
        generatorDone(this, onComplete, ret)
        return;
      }

      this.notice("Initial upload to server successful");
      this._saveSnapshot();

      ret.status = 0;
      ret.formatVersion = STORAGE_FORMAT_VERSION;
      ret.maxVersion = this._snapshotVersion;
      ret.snapVersion = this._snapshotVersion;
      ret.snapshot = eval(uneval(this._snapshot));
      ret.deltas = [];
      ret.updates = [];
      break;

    default:
      this.notice("Could not get bookmarks.status: unknown HTTP status code " +
                  statusResp.target.status);
      break;
    }
    generatorDone(this, onComplete, ret)
  },

  _onLogin: function BSS__onLogin(event) {
    this.notice("Bookmarks sync server: " + this._dav.baseURL);
    this._os.notifyObservers(null, "bookmarks-sync:login", "");
  },

  _onLoginError: function BSS__onLoginError(event) {
    this._os.notifyObservers(null, "bookmarks-sync:login-error", "");
  },

  
  interfaces: [Ci.nsIBookmarksSyncService, Ci.nsISupports],

  
  classDescription: "Bookmarks Sync Service",
  contractID: "@mozilla.org/places/sync-service;1",
  classID: Components.ID("{6efd73bf-6a5a-404f-9246-f70a1286a3d6}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIBookmarksSyncService, Ci.nsISupports]),

  

  

  sync: function BSS_sync() {
    asyncRun(this, this._doSync);
  },

  login: function BSS_login() {
    this._dav.login({load: bind2(this, this._onLogin),
                     error: bind2(this, this._onLoginError)});
  },

  logout: function BSS_logout() {
    this._dav.logout();
    this._os.notifyObservers(null, "bookmarks-sync:logout", "");
  },

  log: function BSS_log(line) {
  },

  notice: function BSS_notice(line) {
    let fullLine = line + "\n";
    dump(fullLine);
    this._console.logStringMessage(line);
    this._log.write(fullLine, fullLine.length);
    this._log.flush();
  },

  error: function BSS_error(line) {
  }
};

function makeFile(path) {
  var file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
  file.initWithPath(path);
  return file;
}

function makeURI(URIString) {
  var ioservice = Cc["@mozilla.org/network/io-service;1"].
                  getService(Ci.nsIIOService);
  return ioservice.newURI(URIString, null, null);
}

function bind2(object, method) {
  return function innerBind() { return method.apply(object, arguments); }
}

function asyncRun(object, method, onComplete, extra) {
  let args = Array.prototype.slice.call(arguments, 2); 
  let ret = false;
  try {
    let gen = method.apply(object, args);
    gen.next(); 
    gen.send(gen);
    ret = true;
  } catch (e) {
    if (e instanceof StopIteration)
      dump("Warning: generator stopped unexpectedly");
    else
      throw e;
  }
  return ret;
}

function continueGenerator(generator, data) {
  try { generator.send(data); }
  catch (e) {
    if (e instanceof StopIteration)
      generator = null;
    else
      throw e;
  }
}

function generatorHandlers(generator) {
  let cb = function(data) {
    continueGenerator(generator, data);
  };
  return {load: cb, error: cb, complete: cb};
}







function generatorDone(object, callback, retval) {
  if (object._timer)
    throw "Called generatorDone when there is a timer already set."

  let cb = bind2(object, function(event) {
    object._timer = null;
    callback(retval);
  });

  object._timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
  object._timer.initWithCallback(new EventListener(cb),
                                 0, object._timer.TYPE_ONE_SHOT);
}

function EventListener(handler) {
  this._handler = handler;
}
EventListener.prototype = {
  QueryInterface: function(iid) {
    if (iid.Equals(Components.interfaces.nsITimerCallback) ||
        iid.Equals(Components.interfaces.nsISupports))
      return this;
    throw Components.results.NS_ERROR_NO_INTERFACE;
  },

  

  handleEvent: function EL_handleEvent(event) {
    this._handler(event);
  },

  

  notify: function EL_notify(timer) {
    this._handler(timer);
  }
};

function xpath(xmlDoc, xpathString) {
  let root = xmlDoc.ownerDocument == null ?
    xmlDoc.documentElement : xmlDoc.ownerDocument.documentElement
  let nsResolver = xmlDoc.createNSResolver(root);

  return xmlDoc.evaluate(xpathString, xmlDoc, nsResolver,
                         Ci.nsIDOMXPathResult.ANY_TYPE, null);
}

function DAVCollection(baseURL) {
  this._baseURL = baseURL;
  this._authProvider = new DummyAuthProvider();
}
DAVCollection.prototype = {
  __dp: null,
  get _dp() {
    if (!this.__dp)
      this.__dp = Cc["@mozilla.org/xmlextras/domparser;1"].
        createInstance(Ci.nsIDOMParser);
    return this.__dp;
  },

  __base64: {},
  __vase64loaded: false,
  get _base64() {
    if (!this.__base64loaded) {
      let jsLoader = Cc["@mozilla.org/moz/jssubscript-loader;1"].
        getService(Ci.mozIJSSubScriptLoader);
      jsLoader.loadSubScript("chrome://sync/content/base64.js", this.__base64);
      this.__base64loaded = true;
    }
    return this.__base64;
  },

  get baseURL() {
    return this._baseURL;
  },

  _loggedIn: false,
  _authString: null,
  _currentUserPath: "nobody",

  _currentUser: "nobody@mozilla.com",
  get currentUser() {
    return this._currentUser;
  },

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
    request.open(op, this._baseURL + path, true);
  
    if (headers) {
      for (var key in headers) {
        request.setRequestHeader(key, headers[key]);
      }
    }

    request.channel.notificationCallbacks = this._authProvider;

    return request;
  },

  GET: function DC_GET(path, handlers, headers) {
    if (!headers)
      headers = {'Content-type': 'text/plain'};
    if (this._authString)
      headers['Authorization'] = this._authString;
    if (this._token)
      headers['If'] = "<" + this._bashURL + "> (<" + this._token + ">)";

    var request = this._makeRequest("GET", path, handlers, headers);
    request.send(null);
  },

  PUT: function DC_PUT(path, data, handlers, headers) {
    if (!headers)
      headers = {'Content-type': 'text/plain'};
    if (this._authString)
      headers['Authorization'] = this._authString;
    if (this._token)
      headers['If'] = "<" + this._bashURL + "> (<" + this._token + ">)";

    var request = this._makeRequest("PUT", path, handlers, headers);
    request.send(data);
  },

  

  login: function DC_login(handlers) {
    this._loginHandlers = handlers;
    internalHandlers = {load: bind2(this, this._onLogin),
                        error: bind2(this, this._onLoginError)};

    try {
      let URI = makeURI(this._baseURL);
      let username = 'nobody@mozilla.com';
      let password;

      let branch = Cc["@mozilla.org/preferences-service;1"].
        getService(Ci.nsIPrefBranch);
      username = branch.getCharPref("browser.places.sync.username");

      
      let lm = Cc["@mozilla.org/login-manager;1"].getService(Ci.nsILoginManager);
      let logins = lm.findLogins({}, URI.hostPort, null,
                                 'Use your ldap username/password - dotmoz');
      

          for (let i = 0; i < logins.length; i++) {
        if (logins[i].username == username) {
          password = logins[i].password;
          break;
        }
      }

      
      this._authString = "Basic " +
        this._base64.Base64.encode(username + ":" + password);
      headers = {'Authorization': this._authString};
    } catch (e) {
      
      
    }
     this._authProvider._authFailed = false;
 
    let request = this._makeRequest("GET", "", internalHandlers, headers);
    request.send(null);
  },

  logout: function DC_logout() {
    this._authString = null;
  },

  _onLogin: function DC__onLogin(event) {


 
    if (this._authProvider._authFailed || event.target.status >= 400) {
      this._onLoginError(event);
      return;
    }

    









      
    let branch = Cc["@mozilla.org/preferences-service;1"].
      getService(Ci.nsIPrefBranch);
    this._currentUser = branch.getCharPref("browser.places.sync.username");

    
    let path = this._currentUser.split("@");
    this._currentUserPath = path[0];

    let serverURL = branch.getCharPref("browser.places.sync.serverURL");
    this._baseURL = serverURL + this._currentUserPath + "/";
    
    this._loggedIn = true;

    if (this._loginHandlers && this._loginHandlers.load)
      this._loginHandlers.load(event);
  },
  _onLoginError: function DC__onLoginError(event) {
    
    

    this._loggedIn = false;

    if (this._loginHandlers && this._loginHandlers.error)
      this._loginHandlers.error(event);
  },

  

  _getActiveLock: function DC__getActiveLock(onComplete) {
    let generator = yield;
    let handlers = generatorHandlers(generator);

    let headers = {'Content-Type': 'text/xml; charset="utf-8"',
                   'Depth': '0'};
    if (this._authString)
      headers['Authorization'] = this._authString;

    let request = this._makeRequest("PROPFIND", "", handlers, headers);
    request.send("<?xml version=\"1.0\" encoding=\"utf-8\" ?>" +
                 "<D:propfind xmlns:D='DAV:'>" +
                 "  <D:prop><D:lockdiscovery/></D:prop>" +
                 "</D:propfind>");
    let event = yield;
    let tokens = xpath(event.target.responseXML, '//D:locktoken/D:href');
    let token = tokens.iterateNext();
    if (token)
      generatorDone(this, onComplete, token.textContent);
    else
      generatorDone(this, onComplete, null);
  },

  lock: function DC_lock(onComplete) {
    let generator = yield;
    let handlers = generatorHandlers(generator);

    headers = {'Content-Type': 'text/xml; charset="utf-8"',
               'Timeout': 'Second-600',
               'Depth': 'infinity'};
    if (this._authString)
      headers['Authorization'] = this._authString;

    let request = this._makeRequest("LOCK", "", handlers, headers);
    request.send("<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n" +
                 "<D:lockinfo xmlns:D=\"DAV:\">\n" +
                 "  <D:locktype><D:write/></D:locktype>\n" +
                 "  <D:lockscope><D:exclusive/></D:lockscope>\n" +
                 "</D:lockinfo>");
    let event = yield;

    if (event.target.status < 200 || event.target.status >= 300) {
      generatorDone(this, onComplete, false);
      return;
    }

    let tokens = xpath(event.target.responseXML, '//D:locktoken/D:href');
    let token = tokens.iterateNext();
    if (token) {
      this._token = token.textContent;
      generatorDone(this, onComplete, true);
    }

    generatorDone(this, onComplete, false);
  },

  unlock: function DC_unlock(onComplete) {
    let generator = yield;
    let handlers = generatorHandlers(generator);

    if (!this._token) {
      generatorDone(this, onComplete, false);
      return;
    }

    headers = {'Lock-Token': "<" + this._token + ">"};
    if (this._authString)
      headers['Authorization'] = this._authString;

    let request = this._makeRequest("UNLOCK", "", handlers, headers);
    request.send(null);
    let event = yield;

    if (event.target.status < 200 || event.target.status >= 300) {
      generatorDone(this, onComplete, false);
      return;
    }

    this._token = null;

    generatorDone(this, onComplete, true);
  },

  stealLock: function DC_stealLock(onComplete) {
    let generator = yield;
    let handlers = generatorHandlers(generator);
    let stolen = false;

    try {
      if (!asyncRun(this, this._getActiveLock, handlers.complete))
        return;
      this._token = yield;
  
      if (!asyncRun(this, this.unlock, handlers.complete))
        return;
      let unlocked = yield;

      if (!unlocked)
        return;
  
      if (!asyncRun(this, this.lock, handlers.complete))
        return;
      stolen = yield;

    } finally {
      generatorDone(this, onComplete, stolen);
    }
  }
};



function DummyAuthProvider() {}
DummyAuthProvider.prototype = {
  
  
  
  
  interfaces: [Ci.nsIBadCertListener,
               Ci.nsIAuthPromptProvider,
               Ci.nsIAuthPrompt,
               Ci.nsIPrompt,
               Ci.nsIProgressEventSink,
               Ci.nsIInterfaceRequestor,
               Ci.nsISupports],

  
  
  
  get _authFailed()         { return this.__authFailed; },
  set _authFailed(newValue) { return this.__authFailed = newValue },

  

  QueryInterface: function DAP_QueryInterface(iid) {
    if (!this.interfaces.some( function(v) { return iid.equals(v) } ))
      throw Cr.NS_ERROR_NO_INTERFACE;

    
    
    
    switch(iid) {
    case Ci.nsIAuthPrompt:
      return this.authPrompt;
    case Ci.nsIPrompt:
      return this.prompt;
    default:
      return this;
    }
  },

  
  
  getInterface: function DAP_getInterface(iid) {
    return this.QueryInterface(iid);
  },

  

  
  
  confirmUnknownIssuer: function DAP_confirmUnknownIssuer(socketInfo, cert, certAddType) {
    return false;
  },

  confirmMismatchDomain: function DAP_confirmMismatchDomain(socketInfo, targetURL, cert) {
    return false;
  },

  confirmCertExpired: function DAP_confirmCertExpired(socketInfo, cert) {
    return false;
  },

  notifyCrlNextupdate: function DAP_notifyCrlNextupdate(socketInfo, targetURL, cert) {
  },

  
  
  getAuthPrompt: function(aPromptReason, aIID) {
    this._authFailed = true;
    throw Cr.NS_ERROR_NOT_AVAILABLE;
  },

  
  
  

  

  get authPrompt() {
    var resource = this;
    return {
      QueryInterface: XPCOMUtils.generateQI([Ci.nsIPrompt]),
      prompt: function(dialogTitle, text, passwordRealm, savePassword, defaultText, result) {
        resource._authFailed = true;
        return false;
      },
      promptUsernameAndPassword: function(dialogTitle, text, passwordRealm, savePassword, user, pwd) {
        resource._authFailed = true;
        return false;
      },
      promptPassword: function(dialogTitle, text, passwordRealm, savePassword, pwd) {
        resource._authFailed = true;
        return false;
      }
    };
  },

  

  get prompt() {
    var resource = this;
    return {
      QueryInterface: XPCOMUtils.generateQI([Ci.nsIPrompt]),
      alert: function(dialogTitle, text) {
        throw Cr.NS_ERROR_NOT_IMPLEMENTED;
      },
      alertCheck: function(dialogTitle, text, checkMessage, checkValue) {
        throw Cr.NS_ERROR_NOT_IMPLEMENTED;
      },
      confirm: function(dialogTitle, text) {
        throw Cr.NS_ERROR_NOT_IMPLEMENTED;
      },
      confirmCheck: function(dialogTitle, text, checkMessage, checkValue) {
        throw Cr.NS_ERROR_NOT_IMPLEMENTED;
      },
      confirmEx: function(dialogTitle, text, buttonFlags, button0Title, button1Title, button2Title, checkMsg, checkValue) {
        throw Cr.NS_ERROR_NOT_IMPLEMENTED;
      },
      prompt: function(dialogTitle, text, value, checkMsg, checkValue) {
        throw Cr.NS_ERROR_NOT_IMPLEMENTED;
      },
      promptPassword: function(dialogTitle, text, password, checkMsg, checkValue) {
        resource._authFailed = true;
        return false;
      },
      promptUsernameAndPassword: function(dialogTitle, text, username, password, checkMsg, checkValue) {
        resource._authFailed = true;
        return false;
      },
      select: function(dialogTitle, text, count, selectList, outSelection) {
        throw Cr.NS_ERROR_NOT_IMPLEMENTED;
      }
    };
  },

  

  onProgress: function DAP_onProgress(aRequest, aContext,
                                      aProgress, aProgressMax) {
  },

  onStatus: function DAP_onStatus(aRequest, aContext,
                                  aStatus, aStatusArg) {
  }
};

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([BookmarksSyncService]);
}
