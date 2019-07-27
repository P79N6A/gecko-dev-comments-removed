





"use strict";

const { Cc, Ci, Cu } = require("chrome");
const { TextEncoder, TextDecoder } = require('sdk/io/buffer');
const { Class } = require("sdk/core/heritage");
const { EventTarget } = require("sdk/event/target");
const { emit } = require("sdk/event/core");
const URL = require("sdk/url");
const promise = require("projecteditor/helpers/promise");
const { OS } = Cu.import("resource://gre/modules/osfile.jsm", {});
const { FileUtils } = Cu.import("resource://gre/modules/FileUtils.jsm", {});
const mimeService = Cc["@mozilla.org/mime;1"].getService(Ci.nsIMIMEService);
const { Task } = Cu.import("resource://gre/modules/Task.jsm", {});

const gDecoder = new TextDecoder();
const gEncoder = new TextEncoder();













var Resource = Class({
  extends: EventTarget,

  refresh: function() { return promise.resolve(this); },
  destroy: function() { },
  delete: function() { },

  setURI: function(uri) {
    if (typeof(uri) === "string") {
      uri = URL.URL(uri);
    }
    this.uri = uri;
  },

  


  get basename() { return this.uri.path.replace(/\/+$/, '').replace(/\\/g,'/').replace( /.*\//, '' ); },

  


  get hasChildren() { return this.children && this.children.size > 0; },

  


  get isRoot() {
    return !this.parent
  },

  


  get childrenSorted() {
    if (!this.hasChildren) {
      return [];
    }

    return [...this.children].sort((a, b)=> {
      
      if (a.isDir !== b.isDir) {
        return b.isDir;
      }
      return a.basename.toLowerCase() > b.basename.toLowerCase();
    });
  },

  



  setChildren: function(newChildren) {
    let oldChildren = this.children || new Set();
    let change = false;

    for (let child of oldChildren) {
      if (!newChildren.has(child)) {
        change = true;
        child.parent = null;
        this.store.notifyRemove(child);
      }
    }

    for (let child of newChildren) {
      if (!oldChildren.has(child)) {
        change = true;
        child.parent = this;
        this.store.notifyAdd(child);
      }
    }

    this.children = newChildren;
    if (change) {
      emit(this, "children-changed", this);
    }
  },

  




  addChild: function(resource) {
    this.children = this.children || new Set();

    resource.parent = this;
    this.children.add(resource);
    this.store.notifyAdd(resource);
    emit(this, "children-changed", this);
    return resource;
  },

  




  removeChild: function(resource) {
    resource.parent = null;
    this.children.remove(resource);
    this.store.notifyRemove(resource);
    emit(this, "children-changed", this);
    return resource;
  },

  





  allDescendants: function() {
    let set = new Set();

    function addChildren(item) {
      if (!item.children) {
        return;
      }

      for (let child of item.children) {
        set.add(child);
      }
    }

    addChildren(this);
    for (let item of set) {
      addChildren(item);
    }

    return set;
  },
});





var FileResource = Class({
  extends: Resource,

  





  initialize: function(store, path, info) {
    this.store = store;
    this.path = path;

    this.setURI(URL.URL(URL.fromFilename(path)));
    this._lastReadModification = undefined;

    this.info = info;
    this.parent = null;
  },

  toString: function() {
    return "[FileResource:" + this.path + "]";
  },

  destroy: function() {
    if (this._refreshDeferred) {
      this._refreshDeferred.reject();
    }
    this._refreshDeferred = null;
  },

  






  refresh: function() {
    if (this._refreshDeferred) {
      return this._refreshDeferred.promise;
    }
    this._refreshDeferred = promise.defer();
    OS.File.stat(this.path).then(info => {
      this.info = info;
      if (this._refreshDeferred) {
        this._refreshDeferred.resolve(this);
        this._refreshDeferred = null;
      }
    });
    return this._refreshDeferred.promise;
  },

  


  get displayName() {
    return this.basename + (this.isDir ? "/" : "")
  },

  





  get isDir() {
    if (!this.info) { return false; }
    return this.info.isDir && !this.info.isSymLink;
  },

  





  load: function() {
    return OS.File.read(this.path).then(bytes => {
      return gDecoder.decode(bytes);
    });
  },

  





  delete: function() {
    emit(this, "deleted", this);
    if (this.isDir) {
      return OS.File.removeDir(this.path);
    } else {
      return OS.File.remove(this.path);
    }
  },

  












  createChild: function(name, initial="") {
    if (!this.isDir) {
      return promise.reject(new Error("Cannot add child to a regular file"));
    }

    let newPath = OS.Path.join(this.path, name);

    let buffer = initial ? gEncoder.encode(initial) : "";
    return OS.File.writeAtomic(newPath, buffer, {
      noOverwrite: true
    }).then(() => {
      return this.store.refresh();
    }).then(() => {
      let resource = this.store.resources.get(newPath);
      if (!resource) {
        throw new Error("Error creating " + newPath);
      }
      return resource;
    });
  },

  







  save: function(content) {
    let buffer = gEncoder.encode(content);
    let path = this.path;

    
    

    return Task.spawn(function*() {
        let pfh = yield OS.File.open(path, {truncate: true});
        yield pfh.write(buffer);
        yield pfh.close();
    });
  },

  


  get contentType() {
    if (this._contentType) {
      return this._contentType;
    }
    if (this.isDir) {
      return "x-directory/normal";
    }
    try {
      this._contentType = mimeService.getTypeFromFile(new FileUtils.File(this.path));
    } catch(ex) {
      if (ex.name !== "NS_ERROR_NOT_AVAILABLE" &&
          ex.name !== "NS_ERROR_FAILURE") {
        console.error(ex, this.path);
      }
      this._contentType = null;
    }
    return this._contentType;
  },

  



  get contentCategory() {
    const NetworkHelper = require("devtools/toolkit/webconsole/network-helper");
    let category = NetworkHelper.mimeCategoryMap[this.contentType];
    
    if (!category && this.basename === "manifest.webapp") {
      return "json";
    }
    return category || "txt";
  }
});

exports.FileResource = FileResource;
