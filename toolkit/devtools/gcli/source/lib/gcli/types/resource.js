















'use strict';

var Promise = require('../util/promise').Promise;

exports.clearResourceCache = function() {
  ResourceCache.clear();
};







function Resource(name, type, inline, element) {
  this.name = name;
  this.type = type;
  this.inline = inline;
  this.element = element;
}





Resource.prototype.loadContents = function() {
  throw new Error('not implemented');
};

Resource.TYPE_SCRIPT = 'text/javascript';
Resource.TYPE_CSS = 'text/css';





function CssResource(domSheet) {
  this.name = domSheet.href;
  if (!this.name) {
    this.name = domSheet.ownerNode && domSheet.ownerNode.id ?
            'css#' + domSheet.ownerNode.id :
            'inline-css';
  }

  this.inline = (domSheet.href == null);
  this.type = Resource.TYPE_CSS;
  this.element = domSheet;
}

CssResource.prototype = Object.create(Resource.prototype);

CssResource.prototype.loadContents = function() {
  return new Promise(function(resolve, reject) {
    resolve(this.element.ownerNode.innerHTML);
  }.bind(this));
};

CssResource._getAllStyles = function(context) {
  var resources = [];
  if (context.environment.window == null) {
    return resources;
  }

  var doc = context.environment.window.document;
  Array.prototype.forEach.call(doc.styleSheets, function(domSheet) {
    CssResource._getStyle(domSheet, resources);
  });

  dedupe(resources, function(clones) {
    for (var i = 0; i < clones.length; i++) {
      clones[i].name = clones[i].name + '-' + i;
    }
  });

  return resources;
};

CssResource._getStyle = function(domSheet, resources) {
  var resource = ResourceCache.get(domSheet);
  if (!resource) {
    resource = new CssResource(domSheet);
    ResourceCache.add(domSheet, resource);
  }
  resources.push(resource);

  
  try {
    Array.prototype.forEach.call(domSheet.cssRules, function(domRule) {
      if (domRule.type == CSSRule.IMPORT_RULE && domRule.styleSheet) {
        CssResource._getStyle(domRule.styleSheet, resources);
      }
    }, this);
  }
  catch (ex) {
    
  }
};





function ScriptResource(scriptNode) {
  this.name = scriptNode.src;
  if (!this.name) {
    this.name = scriptNode.id ?
            'script#' + scriptNode.id :
            'inline-script';
  }

  this.inline = (scriptNode.src === '' || scriptNode.src == null);
  this.type = Resource.TYPE_SCRIPT;
  this.element = scriptNode;
}

ScriptResource.prototype = Object.create(Resource.prototype);

ScriptResource.prototype.loadContents = function() {
  return new Promise(function(resolve, reject) {
    if (this.inline) {
      resolve(this.element.innerHTML);
    }
    else {
      
      var xhr = new XMLHttpRequest();
      xhr.onreadystatechange = function() {
        if (xhr.readyState !== xhr.DONE) {
          return;
        }
        resolve(xhr.responseText);
      };
      xhr.open('GET', this.element.src, true);
      xhr.send();
    }
  }.bind(this));
};

ScriptResource._getAllScripts = function(context) {
  if (context.environment.window == null) {
    return [];
  }

  var doc = context.environment.window.document;
  var scriptNodes = doc.querySelectorAll('script');
  var resources = Array.prototype.map.call(scriptNodes, function(scriptNode) {
    var resource = ResourceCache.get(scriptNode);
    if (!resource) {
      resource = new ScriptResource(scriptNode);
      ResourceCache.add(scriptNode, resource);
    }
    return resource;
  });

  dedupe(resources, function(clones) {
    for (var i = 0; i < clones.length; i++) {
      clones[i].name = clones[i].name + '-' + i;
    }
  });

  return resources;
};




function dedupe(resources, onDupe) {
  
  var names = {};
  resources.forEach(function(scriptResource) {
    if (names[scriptResource.name] == null) {
      names[scriptResource.name] = [];
    }
    names[scriptResource.name].push(scriptResource);
  });

  
  Object.keys(names).forEach(function(name) {
    var clones = names[name];
    if (clones.length > 1) {
      onDupe(clones);
    }
  });
}







var ResourceCache = {
  _cached: [],

  


  get: function(node) {
    for (var i = 0; i < ResourceCache._cached.length; i++) {
      if (ResourceCache._cached[i].node === node) {
        return ResourceCache._cached[i].resource;
      }
    }
    return null;
  },

  


  add: function(node, resource) {
    ResourceCache._cached.push({ node: node, resource: resource });
  },

  


  clear: function() {
    ResourceCache._cached = [];
  }
};




exports.items = [
  {
    item: 'type',
    name: 'resource',
    parent: 'selection',
    cacheable: false,
    include: null,

    constructor: function() {
      if (this.include !== Resource.TYPE_SCRIPT &&
          this.include !== Resource.TYPE_CSS &&
          this.include != null) {
        throw new Error('invalid include property: ' + this.include);
      }
    },

    lookup: function(context) {
      var resources = [];
      if (this.include !== Resource.TYPE_SCRIPT) {
        Array.prototype.push.apply(resources,
                                   CssResource._getAllStyles(context));
      }
      if (this.include !== Resource.TYPE_CSS) {
        Array.prototype.push.apply(resources,
                                   ScriptResource._getAllScripts(context));
      }

      return Promise.resolve(resources.map(function(resource) {
        return { name: resource.name, value: resource };
      }));
    }
  }
];
