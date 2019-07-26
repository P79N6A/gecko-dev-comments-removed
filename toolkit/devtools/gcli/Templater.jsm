















this.EXPORTED_SYMBOLS = [ "template" ];
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "console",
                                  "resource://gre/modules/devtools/Console.jsm");

























function template(node, data, options) {
  var template = new Templater(options || {});
  template.processNode(node, data);
  return template;
}





function Templater(options) {
  if (options == null) {
    options = { allowEval: true };
  }
  this.options = options;
  if (options.stack && Array.isArray(options.stack)) {
    this.stack = options.stack;
  }
  else if (typeof options.stack === 'string') {
    this.stack = [ options.stack ];
  }
  else {
    this.stack = [];
  }
  this.nodes = [];
}








Templater.prototype._templateRegion = /\$\{([^}]*)\}/g;





Templater.prototype._splitSpecial = /\uF001|\uF002/;





Templater.prototype._isPropertyScript = /^[_a-zA-Z0-9.]*$/;







Templater.prototype.processNode = function(node, data) {
  if (typeof node === 'string') {
    node = document.getElementById(node);
  }
  if (data == null) {
    data = {};
  }
  this.stack.push(node.nodeName + (node.id ? '#' + node.id : ''));
  var pushedNode = false;
  try {
    
    if (node.attributes && node.attributes.length) {
      
      
      
      if (node.hasAttribute('foreach')) {
        this._processForEach(node, data);
        return;
      }
      if (node.hasAttribute('if')) {
        if (!this._processIf(node, data)) {
          return;
        }
      }
      
      this.nodes.push(data.__element);
      data.__element = node;
      pushedNode = true;
      
      
      var attrs = Array.prototype.slice.call(node.attributes);
      for (var i = 0; i < attrs.length; i++) {
        var value = attrs[i].value;
        var name = attrs[i].name;

        this.stack.push(name);
        try {
          if (name === 'save') {
            
            value = this._stripBraces(value);
            this._property(value, data, node);
            node.removeAttribute('save');
          }
          else if (name.substring(0, 2) === 'on') {
            
            if (value.substring(0, 2) === '${' && value.slice(-1) === '}' &&
                    value.indexOf('${', 2) === -1) {
              value = this._stripBraces(value);
              var func = this._property(value, data);
              if (typeof func === 'function') {
                node.removeAttribute(name);
                var capture = node.hasAttribute('capture' + name.substring(2));
                node.addEventListener(name.substring(2), func, capture);
                if (capture) {
                  node.removeAttribute('capture' + name.substring(2));
                }
              }
              else {
                
                node.setAttribute(name, func);
              }
            }
            else {
              
              node.setAttribute(name, this._processString(value, data));
            }
          }
          else {
            node.removeAttribute(name);
            
            
            if (name.charAt(0) === '_') {
              name = name.substring(1);
            }

            
            var replacement;
            if (value.indexOf('${') === 0 && value.charAt(value.length - 1) === '}') {
              replacement = this._envEval(value.slice(2, -1), data, value);
              if (replacement && typeof replacement.then === 'function') {
                node.setAttribute(name, '');
                replacement.then(function(newValue) {
                  node.setAttribute(name, newValue);
                }.bind(this)).then(null, console.error);
              }
              else {
                if (this.options.blankNullUndefined && replacement == null) {
                  replacement = '';
                }
                node.setAttribute(name, replacement);
              }
            }
            else {
              node.setAttribute(name, this._processString(value, data));
            }
          }
        }
        finally {
          this.stack.pop();
        }
      }
    }

    
    
    var childNodes = Array.prototype.slice.call(node.childNodes);
    for (var j = 0; j < childNodes.length; j++) {
      this.processNode(childNodes[j], data);
    }

    if (node.nodeType === 3 ) {
      this._processTextNode(node, data);
    }
  }
  finally {
    if (pushedNode) {
      data.__element = this.nodes.pop();
    }
    this.stack.pop();
  }
};




Templater.prototype._processString = function(value, data) {
  return value.replace(this._templateRegion, function(path) {
    var insert = this._envEval(path.slice(2, -1), data, value);
    return this.options.blankNullUndefined && insert == null ? '' : insert;
  }.bind(this));
};







Templater.prototype._processIf = function(node, data) {
  this.stack.push('if');
  try {
    var originalValue = node.getAttribute('if');
    var value = this._stripBraces(originalValue);
    var recurse = true;
    try {
      var reply = this._envEval(value, data, originalValue);
      recurse = !!reply;
    }
    catch (ex) {
      this._handleError('Error with \'' + value + '\'', ex);
      recurse = false;
    }
    if (!recurse) {
      node.parentNode.removeChild(node);
    }
    node.removeAttribute('if');
    return recurse;
  }
  finally {
    this.stack.pop();
  }
};











Templater.prototype._processForEach = function(node, data) {
  this.stack.push('foreach');
  try {
    var originalValue = node.getAttribute('foreach');
    var value = originalValue;

    var paramName = 'param';
    if (value.charAt(0) === '$') {
      
      value = this._stripBraces(value);
    }
    else {
      
      var nameArr = value.split(' in ');
      paramName = nameArr[0].trim();
      value = this._stripBraces(nameArr[1].trim());
    }
    node.removeAttribute('foreach');
    try {
      var evaled = this._envEval(value, data, originalValue);
      this._handleAsync(evaled, node, function(reply, siblingNode) {
        this._processForEachLoop(reply, node, siblingNode, data, paramName);
      }.bind(this));
      node.parentNode.removeChild(node);
    }
    catch (ex) {
      this._handleError('Error with \'' + value + '\'', ex);
    }
  }
  finally {
    this.stack.pop();
  }
};












Templater.prototype._processForEachLoop = function(set, template, sibling, data, paramName) {
  if (Array.isArray(set)) {
    set.forEach(function(member, i) {
      this._processForEachMember(member, template, sibling, data, paramName, '' + i);
    }, this);
  }
  else {
    for (var member in set) {
      if (set.hasOwnProperty(member)) {
        this._processForEachMember(member, template, sibling, data, paramName, member);
      }
    }
  }
};













Templater.prototype._processForEachMember = function(member, template, siblingNode, data, paramName, frame) {
  this.stack.push(frame);
  try {
    this._handleAsync(member, siblingNode, function(reply, node) {
      data[paramName] = reply;
      if (node.parentNode != null) {
        if (template.nodeName.toLowerCase() === 'loop') {
          for (var i = 0; i < template.childNodes.length; i++) {
            var clone = template.childNodes[i].cloneNode(true);
            node.parentNode.insertBefore(clone, node);
            this.processNode(clone, data);
          }
        }
        else {
          var clone = template.cloneNode(true);
          clone.removeAttribute('foreach');
          node.parentNode.insertBefore(clone, node);
          this.processNode(clone, data);
        }
      }
      delete data[paramName];
    }.bind(this));
  }
  finally {
    this.stack.pop();
  }
};








Templater.prototype._processTextNode = function(node, data) {
  
  var value = node.data;
  
  
  
  
  
  
  
  
  
  value = value.replace(this._templateRegion, '\uF001$$$1\uF002');
  var parts = value.split(this._splitSpecial);
  if (parts.length > 1) {
    parts.forEach(function(part) {
      if (part === null || part === undefined || part === '') {
        return;
      }
      if (part.charAt(0) === '$') {
        part = this._envEval(part.slice(1), data, node.data);
      }
      this._handleAsync(part, node, function(reply, siblingNode) {
        var doc = siblingNode.ownerDocument;
        if (reply == null) {
          reply = this.options.blankNullUndefined ? '' : '' + reply;
        }
        if (typeof reply.cloneNode === 'function') {
          
          reply = this._maybeImportNode(reply, doc);
          siblingNode.parentNode.insertBefore(reply, siblingNode);
        }
        else if (typeof reply.item === 'function' && reply.length) {
          
          
          
          var list = Array.prototype.slice.call(reply, 0);
          list.forEach(function(child) {
            var imported = this._maybeImportNode(child, doc);
            siblingNode.parentNode.insertBefore(imported, siblingNode);
          }.bind(this));
        }
        else {
          
          reply = doc.createTextNode(reply.toString());
          siblingNode.parentNode.insertBefore(reply, siblingNode);
        }
      }.bind(this));
    }, this);
    node.parentNode.removeChild(node);
  }
};







Templater.prototype._maybeImportNode = function(node, doc) {
  return node.ownerDocument === doc ? node : doc.importNode(node, true);
};











Templater.prototype._handleAsync = function(thing, siblingNode, inserter) {
  if (thing != null && typeof thing.then === 'function') {
    
    var tempNode = siblingNode.ownerDocument.createElement('span');
    siblingNode.parentNode.insertBefore(tempNode, siblingNode);
    thing.then(function(delayed) {
      inserter(delayed, tempNode);
      if (tempNode.parentNode != null) {
        tempNode.parentNode.removeChild(tempNode);
      }
    }.bind(this)).then(null, function(error) {
      console.error(error.stack);
    });
  }
  else {
    inserter(thing, siblingNode);
  }
};






Templater.prototype._stripBraces = function(str) {
  if (!str.match(this._templateRegion)) {
    this._handleError('Expected ' + str + ' to match ${...}');
    return str;
  }
  return str.slice(2, -1);
};


















Templater.prototype._property = function(path, data, newValue) {
  try {
    if (typeof path === 'string') {
      path = path.split('.');
    }
    var value = data[path[0]];
    if (path.length === 1) {
      if (newValue !== undefined) {
        data[path[0]] = newValue;
      }
      if (typeof value === 'function') {
        return value.bind(data);
      }
      return value;
    }
    if (!value) {
      this._handleError('"' + path[0] + '" is undefined');
      return null;
    }
    return this._property(path.slice(1), value, newValue);
  }
  catch (ex) {
    this._handleError('Path error with \'' + path + '\'', ex);
    return '${' + path + '}';
  }
};














Templater.prototype._envEval = function(script, data, frame) {
  try {
    this.stack.push(frame.replace(/\s+/g, ' '));
    if (this._isPropertyScript.test(script)) {
      return this._property(script, data);
    }
    else {
      if (!this.options.allowEval) {
        this._handleError('allowEval is not set, however \'' + script + '\'' +
            ' can not be resolved using a simple property path.');
        return '${' + script + '}';
      }
      with (data) {
        return eval(script);
      }
    }
  }
  catch (ex) {
    this._handleError('Template error evaluating \'' + script + '\'', ex);
    return '${' + script + '}';
  }
  finally {
    this.stack.pop();
  }
};







Templater.prototype._handleError = function(message, ex) {
  this._logError(message + ' (In: ' + this.stack.join(' > ') + ')');
  if (ex) {
    this._logError(ex);
  }
};






Templater.prototype._logError = function(message) {
  console.log(message);
};


this.template = template;



