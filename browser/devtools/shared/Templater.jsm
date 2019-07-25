






































var EXPORTED_SYMBOLS = [ "Templater" ];

Components.utils.import("resource://gre/modules/Services.jsm");
const Node = Components.interfaces.nsIDOMNode;






function Templater() {
  this.stack = [];
}







Templater.prototype.processNode = function(node, data) {
  if (typeof node === 'string') {
    node = document.getElementById(node);
  }
  if (data == null) {
    data = {};
  }
  this.stack.push(node.nodeName + (node.id ? '#' + node.id : ''));
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
      
      data.__element = node;
      
      
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
          } else if (name.substring(0, 2) === 'on') {
            
            value = this._stripBraces(value);
            var func = this._property(value, data);
            if (typeof func !== 'function') {
              this._handleError('Expected ' + value +
                ' to resolve to a function, but got ' + typeof func);
            }
            node.removeAttribute(name);
            var capture = node.hasAttribute('capture' + name.substring(2));
            node.addEventListener(name.substring(2), func, capture);
            if (capture) {
              node.removeAttribute('capture' + name.substring(2));
            }
          } else {
            
            var newValue = value.replace(/\$\{[^}]*\}/g, function(path) {
              return this._envEval(path.slice(2, -1), data, value);
            }.bind(this));
            
            
            if (name.charAt(0) === '_') {
              node.removeAttribute(name);
              node.setAttribute(name.substring(1), newValue);
            } else if (value !== newValue) {
              attrs[i].value = newValue;
            }
          }
        } finally {
          this.stack.pop();
        }
      }
    }

    
    
    var childNodes = Array.prototype.slice.call(node.childNodes);
    for (var j = 0; j < childNodes.length; j++) {
      this.processNode(childNodes[j], data);
    }

    if (node.nodeType === Node.TEXT_NODE) {
      this._processTextNode(node, data);
    }
  } finally {
    delete data.__element;
    this.stack.pop();
  }
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
    } catch (ex) {
      this._handleError('Error with \'' + value + '\'', ex);
      recurse = false;
    }
    if (!recurse) {
      node.parentNode.removeChild(node);
    }
    node.removeAttribute('if');
    return recurse;
  } finally {
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
    } else {
      
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
    } catch (ex) {
      this._handleError('Error with \'' + value + '\'', ex);
    }
  } finally {
    this.stack.pop();
  }
};












Templater.prototype._processForEachLoop = function(set, template, sibling, data, paramName) {
  if (Array.isArray(set)) {
    set.forEach(function(member, i) {
      this._processForEachMember(member, template, sibling, data, paramName, '' + i);
    }, this);
  } else {
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
      if (template.nodeName.toLowerCase() === 'loop') {
        for (var i = 0; i < template.childNodes.length; i++) {
          var clone = template.childNodes[i].cloneNode(true);
          node.parentNode.insertBefore(clone, node);
          this.processNode(clone, data);
        }
      } else {
        var clone = template.cloneNode(true);
        clone.removeAttribute('foreach');
        node.parentNode.insertBefore(clone, node);
        this.processNode(clone, data);
      }
      delete data[paramName];
    }.bind(this));
  } finally {
    this.stack.pop();
  }
};








Templater.prototype._processTextNode = function(node, data) {
  
  var value = node.data;
  
  
  
  
  
  
  
  
  
  value = value.replace(/\$\{([^}]*)\}/g, '\uF001$$$1\uF002');
  var parts = value.split(/\uF001|\uF002/);
  if (parts.length > 1) {
    parts.forEach(function(part) {
      if (part === null || part === undefined || part === '') {
        return;
      }
      if (part.charAt(0) === '$') {
        part = this._envEval(part.slice(1), data, node.data);
      }
      this._handleAsync(part, node, function(reply, siblingNode) {
        reply = this._toNode(reply, siblingNode.ownerDocument);
        siblingNode.parentNode.insertBefore(reply, siblingNode);
      }.bind(this));
    }, this);
    node.parentNode.removeChild(node);
  }
};








Templater.prototype._toNode = function(thing, document) {
  if (thing == null) {
    thing = '' + thing;
  }
  
  if (typeof thing.cloneNode !== 'function') {
    thing = document.createTextNode(thing.toString());
  }
  return thing;
};











Templater.prototype._handleAsync = function(thing, siblingNode, inserter) {
  if (typeof thing.then === 'function') {
    
    var tempNode = siblingNode.ownerDocument.createElement('span');
    siblingNode.parentNode.insertBefore(tempNode, siblingNode);
    thing.then(function(delayed) {
      inserter(delayed, tempNode);
      tempNode.parentNode.removeChild(tempNode);
    }.bind(this));
  }
  else {
    inserter(thing, siblingNode);
  }
};






Templater.prototype._stripBraces = function(str) {
  if (!str.match(/\$\{.*\}/g)) {
    this._handleError('Expected ' + str + ' to match ${...}');
    return str;
  }
  return str.slice(2, -1);
};


















Templater.prototype._property = function(path, data, newValue) {
  this.stack.push(path);
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
      this._handleError('Can\'t find path=' + path);
      return null;
    }
    return this._property(path.slice(1), value, newValue);
  } finally {
    this.stack.pop();
  }
};














Templater.prototype._envEval = function(script, data, frame) {
  with (data) {
    try {
      this.stack.push(frame);
      return eval(script);
    } catch (ex) {
      this._handleError('Template error evaluating \'' + script + '\'' +
          ' environment=' + Object.keys(data).join(', '), ex);
      return script;
    } finally {
      this.stack.pop();
    }
  }
};







Templater.prototype._handleError = function(message, ex) {
  this._logError(message);
  this._logError('In: ' + this.stack.join(' > '));
  if (ex) {
    this._logError(ex);
  }
};







Templater.prototype._logError = function(message) {
  Services.console.logStringMessage(message);
};
