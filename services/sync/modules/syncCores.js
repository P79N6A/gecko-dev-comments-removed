



































const EXPORTED_SYMBOLS = ['SyncCore', 'BookmarksSyncCore', 'HistorySyncCore'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/constants.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/async.js");

Function.prototype.async = Async.sugar;









function SyncCore() {
  this._init();
}
SyncCore.prototype = {
  _logName: "Sync",

  _init: function SC__init() {
    this._log = Log4Moz.Service.getLogger("Service." + this._logName);
  },

  
  _getEdits: function SC__getEdits(a, b) {
    let ret = {numProps: 0, props: {}};
    for (prop in a) {
      if (!Utils.deepEquals(a[prop], b[prop])) {
        ret.numProps++;
        ret.props[prop] = b[prop];
      }
    }
    return ret;
  },

  _nodeParents: function SC__nodeParents(GUID, tree) {
    return this._nodeParentsInt(GUID, tree, []);
  },

  _nodeParentsInt: function SC__nodeParentsInt(GUID, tree, parents) {
    if (!tree[GUID] || !tree[GUID].parentGUID)
      return parents;
    parents.push(tree[GUID].parentGUID);
    return this._nodeParentsInt(tree[GUID].parentGUID, tree, parents);
  },

  _detectUpdates: function SC__detectUpdates(a, b) {
    let self = yield;
    let listener = new Utils.EventListener(self.cb);
    let timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);

    let cmds = [];

    try {
      for (let GUID in a) {

        timer.initWithCallback(listener, 0, timer.TYPE_ONE_SHOT);
        yield; 

        if (GUID in b) {
          let edits = this._getEdits(a[GUID], b[GUID]);
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

        timer.initWithCallback(listener, 0, timer.TYPE_ONE_SHOT);
        yield; 

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

    } catch (e) {
      throw e;

    } finally {
      timer = null;
      self.done(cmds);
    }
  },

  _commandLike: function SC__commandLike(a, b) {
    this._log.error("commandLike needs to be subclassed");

    
    
    if (!a || !b || a.GUID == b.GUID)
      return false;

    
    
    for (let key in a) {
      if (key != "GUID" && !Utils.deepEquals(a[key], b[key]))
        return false;
    }
    for (let key in b) {
      if (key != "GUID" && !Utils.deepEquals(a[key], b[key]))
        return false;
    }
    return true;
  },

  
  
  
  _fixParents: function SC__fixParents(list, oldGUID, newGUID) {
    for (let i = 0; i < list.length; i++) {
      if (!list[i])
        continue;
      if (list[i].data.parentGUID == oldGUID)
        list[i].data.parentGUID = newGUID;
      for (let j = 0; j < list[i].parents.length; j++) {
        if (list[i].parents[j] == oldGUID)
          list[i].parents[j] = newGUID;
      }
    }
  },

  _conflicts: function SC__conflicts(a, b) {
    if ((a.GUID == b.GUID) && !Utils.deepEquals(a, b))
      return true;
    return false;
  },

  _getPropagations: function SC__getPropagations(commands, conflicts, propagations) {
    for (let i = 0; i < commands.length; i++) {
      let alsoConflicts = function(elt) {
        return (elt.action == "create" || elt.action == "remove") &&
          commands[i].parents.indexOf(elt.GUID) >= 0;
      };
      if (conflicts.some(alsoConflicts))
        conflicts.push(commands[i]);

      let cmdConflicts = function(elt) {
        return elt.GUID == commands[i].GUID;
      };
      if (!conflicts.some(cmdConflicts))
        propagations.push(commands[i]);
    }
  },

  _itemExists: function SC__itemExists(GUID) {
    this._log.error("itemExists needs to be subclassed");
    return false;
  },

  _reconcile: function SC__reconcile(listA, listB) {
    let self = yield;
    let listener = new Utils.EventListener(self.cb);
    let timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);

    let propagations = [[], []];
    let conflicts = [[], []];
    let ret = {propagations: propagations, conflicts: conflicts};
    this._log.debug("Reconciling " + listA.length +
		    " against " + listB.length + "commands");

    try {
      let guidChanges = [];
      for (let i = 0; i < listA.length; i++) {
	let a = listA[i];
        timer.initWithCallback(listener, 0, timer.TYPE_ONE_SHOT);
        yield; 

	

	let skip = false;
	listB = listB.filter(function(b) {
	  
	  if (skip)
	    return true;

          if (Utils.deepEquals(a, b)) {
            delete listA[i]; 
	    skip = true;
	    return false; 

          } else if (this._commandLike(a, b)) {
            this._fixParents(listA, a.GUID, b.GUID);
	    guidChanges.push({action: "edit",
			      GUID: a.GUID,
			      data: {GUID: b.GUID}});
            delete listA[i]; 
	    skip = true;
	    return false; 
          }
  
          
          if (b.action == "create" && this._itemExists(b.GUID)) {
            this._log.error("Remote command has GUID that already exists " +
                            "locally. Dropping command.");
	    return false; 
          }
	  return true; 
        }, this);
      }
  
      listA = listA.filter(function(elt) { return elt });
      listB = guidChanges.concat(listB);
  
      for (let i = 0; i < listA.length; i++) {
        for (let j = 0; j < listB.length; j++) {

          timer.initWithCallback(listener, 0, timer.TYPE_ONE_SHOT);
          yield; 
  
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
  
      this._getPropagations(listA, conflicts[0], propagations[1]);
  
      timer.initWithCallback(listener, 0, timer.TYPE_ONE_SHOT);
      yield; 
  
      this._getPropagations(listB, conflicts[1], propagations[0]);
      ret = {propagations: propagations, conflicts: conflicts};

    } catch (e) {
      this._log.error("Exception caught: " + (e.message? e.message : e) +
                      " - " + (e.location? e.location : "_reconcile"));

    } finally {
      timer = null;
      self.done(ret);
    }
  },

  

  detectUpdates: function SC_detectUpdates(onComplete, a, b) {
    return this._detectUpdates.async(this, onComplete, a, b);
  },

  reconcile: function SC_reconcile(onComplete, listA, listB) {
    return this._reconcile.async(this, onComplete, listA, listB);
  }
};

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
       a.data.type != b.data.type ||
       a.data.parentGUID != b.data.parentGUID ||
       a.GUID == b.GUID)
      return false;

    
    
    
    switch (a.data.type) {
    case "bookmark":
      if (this._comp(a.data, b.data, 'URI') &&
          this._comp(a.data, b.data, 'title'))
        return true;
      return false;
    case "query":
      if (this._comp(a.data, b.data, 'URI') &&
          this._comp(a.data, b.data, 'title'))
        return true;
      return false;
    case "microsummary":
      if (this._comp(a.data, b.data, 'URI') &&
          this._comp(a.data, b.data, 'generatorURI'))
        return true;
      return false;
    case "folder":
      if (this._comp(a, b, 'index') &&
          this._comp(a.data, b.data, 'title'))
        return true;
      return false;
    case "livemark":
      if (this._comp(a.data, b.data, 'title') &&
          this._comp(a.data, b.data, 'siteURI') &&
          this._comp(a.data, b.data, 'feedURI'))
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

function HistorySyncCore() {
  this._init();
}
HistorySyncCore.prototype = {
  _logName: "HistSync",

  _itemExists: function BSC__itemExists(GUID) {
    
    return false;
  },

  _commandLike: function BSC_commandLike(a, b) {
    
    
    
    
    return false;
  }
};
HistorySyncCore.prototype.__proto__ = new SyncCore();
