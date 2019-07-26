











































const NOT_FOUND_STRING = "n/a";












function Template(root, store, l10nResolver) {
  this._store = store;
  this._l10n = l10nResolver;

  
  
  

  this._nodeListeners = new Map();
  this._loopListeners = new Map();
  this._forListeners = new Map();
  this._root = root;
  this._doc = this._root.ownerDocument;

  this._store.on("set", (event,path,value) => this._storeChanged(path,value));
}

Template.prototype = {
  start: function() {
    this._processTree(this._root);
  },

  _resolvePath: function(path, defaultValue=null) {

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    let chunks = path.split(".");
    let obj = this._store.object;
    for (let word of chunks) {
      if ((typeof obj) == "object" &&
          (word in obj)) {
        obj = obj[word];
      } else {
        return defaultValue;
      }
    }
    return obj;
  },

  _storeChanged: function(path, value) {

    
    

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
    for (let [registeredPath, set] of this._nodeListeners) {
      for (let e of nodes) {
        set.delete(e);
      }
      if (set.size == 0) {
        this._nodeListeners.delete(registeredPath);
      }
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

  _processNode: function(element, rootPath="") {
    
    
    
    
    

    let e = element;
    let str = e.getAttribute("template");

    if (rootPath) {
      
      
      rootPath = rootPath + ".";
    }

    try {
      let json = JSON.parse(str);
      
      if (!("type" in json)) {
        throw new Error("missing property");
      }
      if (json.rootPath) {
        
        
        rootPath = json.rootPath;
      }

      
      
      

      let paths = [];

      switch (json.type) {
        case "attribute": {
          if (!("name" in json) ||
              !("path" in json)) {
            throw new Error("missing property");
          }
          e.setAttribute(json.name, this._resolvePath(rootPath + json.path, NOT_FOUND_STRING));
          paths.push(rootPath + json.path);
          break;
        }
        case "textContent": {
          if (!("path" in json)) {
            throw new Error("missing property");
          }
          e.textContent = this._resolvePath(rootPath + json.path, NOT_FOUND_STRING);
          paths.push(rootPath + json.path);
          break;
        }
        case "localizedContent": {
          if (!("property" in json) ||
              !("paths" in json)) {
            throw new Error("missing property");
          }
          let params = json.paths.map((p) => {
            paths.push(rootPath + p);
            let str = this._resolvePath(rootPath + p, NOT_FOUND_STRING);
            return str;
          });
          e.textContent = this._l10n(json.property, params);
          break;
        }
      }
      if (rootPath) {
        
        json.rootPath = rootPath;
        e.setAttribute("template", JSON.stringify(json));
      }
      if (paths.length > 0) {
        for (let path of paths) {
          this._registerNode(path, e);
        }
      }
    } catch(exception) {
      console.error("Invalid template: " + e.outerHTML + " (" + exception + ")");
    }
  },

  _processLoop: function(element, rootPath="") {
    
    
    
    
    
    let e = element;
    try {
      let template, count;
      let str = e.getAttribute("template-loop");
      let json = JSON.parse(str);
      if (!("arrayPath" in json)     ||
          !("childSelector" in json)) {
        throw new Error("missing property");
      }
      if (rootPath) {
        json.arrayPath = rootPath + "." + json.arrayPath;
      }
      let templateParent = this._doc.querySelector(json.childSelector);
      if (!templateParent) {
        throw new Error("can't find child");
      }
      template = this._doc.createElement("div");
      template.innerHTML = templateParent.innerHTML;
      template = template.firstElementChild;
      let array = this._resolvePath(json.arrayPath, []);
      if (!Array.isArray(array)) {
        console.error("referenced array is not an array");
      }
      count = array.length;

      let fragment = this._doc.createDocumentFragment();
      for (let i = 0; i < count; i++) {
        let node = template.cloneNode(true);
        this._processTree(node, json.arrayPath + "." + i);
        fragment.appendChild(node);
      }
      this._registerLoop(json.arrayPath, e);
      this._registerLoop(json.arrayPath + ".length", e);
      this._unregisterNodes(e.querySelectorAll("[template]"));
      e.innerHTML = "";
      e.appendChild(fragment);
    } catch(exception) {
      console.error("Invalid template: " + e.outerHTML + " (" + exception + ")");
    }
  },

  _processFor: function(element, rootPath="") {
    let e = element;
    try {
      let template;
      let str = e.getAttribute("template-for");
      let json = JSON.parse(str);
      if (!("path" in json) ||
          !("childSelector" in json)) {
        throw new Error("missing property");
      }

      if (rootPath) {
        json.path = rootPath + "." + json.path;
      }

      if (!json.path) {
        
        this._unregisterNodes(e.querySelectorAll("[template]"));
        e.innerHTML = "";
        return;
      }

      let templateParent = this._doc.querySelector(json.childSelector);
      if (!templateParent) {
        throw new Error("can't find child");
      }
      let content = this._doc.createElement("div");
      content.innerHTML = templateParent.innerHTML;
      content = content.firstElementChild;

      this._processTree(content, json.path);

      this._unregisterNodes(e.querySelectorAll("[template]"));
      this._registerFor(json.path, e);

      e.innerHTML = "";
      e.appendChild(content);

    } catch(exception) {
      console.error("Invalid template: " + e.outerHTML + " (" + exception + ")");
    }
  },

  _processTree: function(parent, rootPath="") {
    let loops = parent.querySelectorAll(":not(template) [template-loop]");
    let fors = parent.querySelectorAll(":not(template) [template-for]");
    let nodes = parent.querySelectorAll(":not(template) [template]");
    for (let e of loops) {
      this._processLoop(e, rootPath);
    }
    for (let e of fors) {
      this._processFor(e, rootPath);
    }
    for (let e of nodes) {
      this._processNode(e, rootPath);
    }
    if (parent.hasAttribute("template")) {
      this._processNode(parent, rootPath);
    }
  },
}
