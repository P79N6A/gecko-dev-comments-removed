











































const NOT_FOUND_STRING = "n/a";












function Template(root, store, l10nResolver) {
  this._store = store;
  this._rootResolver = new Resolver(this._store.object);
  this._l10n = l10nResolver;

  
  
  

  this._nodeListeners = new Map();
  this._loopListeners = new Map();
  this._forListeners = new Map();
  this._root = root;
  this._doc = this._root.ownerDocument;

  this._storeChanged = this._storeChanged.bind(this);
  this._store.on("set", this._storeChanged);
}

Template.prototype = {
  start: function() {
    this._processTree(this._root);
  },

  destroy: function() {
    this._store.off("set", this._storeChanged);
    this._root = null;
    this._doc = null;
  },

  _storeChanged: function(event, path, value) {

    
    

    let strpath = path.join(".");
    this._invalidate(strpath);

    for (let [registeredPath, set] of this._nodeListeners) {
      if (strpath != registeredPath &&
          registeredPath.indexOf(strpath) > -1) {
        this._invalidate(registeredPath);
      }
    }
  },

  _invalidate: function(path) {
    
    let set = this._loopListeners.get(path);
    if (set) {
      for (let elt of set) {
        this._processLoop(elt);
      }
    }

    
    set = this._forListeners.get(path);
    if (set) {
      for (let elt of set) {
        this._processFor(elt);
      }
    }

    
    set = this._nodeListeners.get(path);
    if (set) {
      for (let elt of set) {
        this._processNode(elt);
      }
    }
  },

  _registerNode: function(path, element) {

    
    
    
    

    if (!this._nodeListeners.has(path)) {
      this._nodeListeners.set(path, new Set());
    }
    let set = this._nodeListeners.get(path);
    set.add(element);
  },

  _unregisterNodes: function(nodes) {
    for (let e of nodes) {
      for (let registeredPath of e.registeredPaths) {
        let set = this._nodeListeners.get(registeredPath);
        set.delete(e);
        if (set.size === 0) {
          this._nodeListeners.delete(registeredPath);
        }
      }
      e.registeredPaths = null;
    }
  },

  _registerLoop: function(path, element) {
    if (!this._loopListeners.has(path)) {
      this._loopListeners.set(path, new Set());
    }
    let set = this._loopListeners.get(path);
    set.add(element);
  },

  _registerFor: function(path, element) {
    if (!this._forListeners.has(path)) {
      this._forListeners.set(path, new Set());
    }
    let set = this._forListeners.get(path);
    set.add(element);
  },

  _processNode: function(element, resolver=this._rootResolver) {
    
    
    
    
    
    
    

    let e = element;
    let str = e.getAttribute("template");

    try {
      let json = JSON.parse(str);
      
      if (!("type" in json)) {
        throw new Error("missing property");
      }
      if (json.rootPath) {
        
        
        resolver = this._rootResolver.descend(json.rootPath);
      }

      
      
      

      let paths = [];

      switch (json.type) {
        case "attribute": {
          if (!("name" in json) ||
              !("path" in json)) {
            throw new Error("missing property");
          }
          e.setAttribute(json.name, resolver.get(json.path, NOT_FOUND_STRING));
          paths.push(resolver.rootPathTo(json.path));
          break;
        }
        case "textContent": {
          if (!("path" in json)) {
            throw new Error("missing property");
          }
          e.textContent = resolver.get(json.path, NOT_FOUND_STRING);
          paths.push(resolver.rootPathTo(json.path));
          break;
        }
        case "localizedContent": {
          if (!("property" in json) ||
              !("paths" in json)) {
            throw new Error("missing property");
          }
          let params = json.paths.map((p) => {
            paths.push(resolver.rootPathTo(p));
            let str = resolver.get(p, NOT_FOUND_STRING);
            return str;
          });
          e.textContent = this._l10n(json.property, params);
          break;
        }
      }
      if (resolver !== this._rootResolver) {
        
        json.rootPath = resolver.path;
        e.setAttribute("template", JSON.stringify(json));
      }
      if (paths.length > 0) {
        for (let path of paths) {
          this._registerNode(path, e);
        }
      }
      
      e.registeredPaths = paths;
    } catch(exception) {
      console.error("Invalid template: " + e.outerHTML + " (" + exception + ")");
    }
  },

  _processLoop: function(element, resolver=this._rootResolver) {
    
    
    
    
    
    let e = element;
    try {
      let template, count;
      let str = e.getAttribute("template-loop");
      let json = JSON.parse(str);
      if (!("arrayPath" in json)     ||
          !("childSelector" in json)) {
        throw new Error("missing property");
      }
      let descendedResolver = resolver.descend(json.arrayPath);
      let templateParent = this._doc.querySelector(json.childSelector);
      if (!templateParent) {
        throw new Error("can't find child");
      }
      template = this._doc.createElement("div");
      template.innerHTML = templateParent.innerHTML;
      template = template.firstElementChild;
      let array = descendedResolver.get("", []);
      if (!Array.isArray(array)) {
        console.error("referenced array is not an array");
      }
      count = array.length;

      let fragment = this._doc.createDocumentFragment();
      for (let i = 0; i < count; i++) {
        let node = template.cloneNode(true);
        this._processTree(node, descendedResolver.descend(i));
        fragment.appendChild(node);
      }
      this._registerLoop(descendedResolver.path, e);
      this._registerLoop(descendedResolver.rootPathTo("length"), e);
      this._unregisterNodes(e.querySelectorAll("[template]"));
      e.innerHTML = "";
      e.appendChild(fragment);
    } catch(exception) {
      console.error("Invalid template: " + e.outerHTML + " (" + exception + ")");
    }
  },

  _processFor: function(element, resolver=this._rootResolver) {
    let e = element;
    try {
      let template;
      let str = e.getAttribute("template-for");
      let json = JSON.parse(str);
      if (!("path" in json) ||
          !("childSelector" in json)) {
        throw new Error("missing property");
      }

      if (!json.path) {
        
        this._unregisterNodes(e.querySelectorAll("[template]"));
        e.innerHTML = "";
        return;
      }

      let descendedResolver = resolver.descend(json.path);
      let templateParent = this._doc.querySelector(json.childSelector);
      if (!templateParent) {
        throw new Error("can't find child");
      }
      let content = this._doc.createElement("div");
      content.innerHTML = templateParent.innerHTML;
      content = content.firstElementChild;

      this._processTree(content, descendedResolver);

      this._unregisterNodes(e.querySelectorAll("[template]"));
      this._registerFor(descendedResolver.path, e);

      e.innerHTML = "";
      e.appendChild(content);

    } catch(exception) {
      console.error("Invalid template: " + e.outerHTML + " (" + exception + ")");
    }
  },

  _processTree: function(parent, resolver=this._rootResolver) {
    let loops = parent.querySelectorAll(":not(template) [template-loop]");
    let fors = parent.querySelectorAll(":not(template) [template-for]");
    let nodes = parent.querySelectorAll(":not(template) [template]");
    for (let i = 0; i < loops.length; i++) {
      this._processLoop(loops[i], resolver);
    }
    for (let i = 0; i < fors.length; i++) {
      this._processFor(fors[i], resolver);
    }
    for (let i = 0; i < nodes.length; i++) {
      this._processNode(nodes[i], resolver);
    }
    if (parent.hasAttribute("template")) {
      this._processNode(parent, resolver);
    }
  },
};

function Resolver(object, path = "") {
  this._object = object;
  this.path = path;
}

Resolver.prototype = {

  get: function(path, defaultValue = null) {
    let obj = this._object;
    if (path === "") {
      return obj;
    }
    let chunks = path.toString().split(".");
    for (let i = 0; i < chunks.length; i++) {
      let word = chunks[i];
      if ((typeof obj) == "object" &&
          (word in obj)) {
        obj = obj[word];
      } else {
        return defaultValue;
      }
    }
    return obj;
  },

  rootPathTo: function(path) {
    return this.path ? this.path + "." + path : path;
  },

  descend: function(path) {
    return new Resolver(this.get(path), this.rootPathTo(path));
  }

};
