















'use strict';




var childProcess = require('child_process');
var fs = require('fs');
var path = require('path');

var promise = require('./promise');
var util = require('./util');

var ATTR_NAME = '__gcli_border';
var HIGHLIGHT_STYLE = '1px dashed black';

function Highlighter(document) {
  this._document = document;
  this._nodes = util.createEmptyNodeList(this._document);
}

Object.defineProperty(Highlighter.prototype, 'nodelist', {
  set: function(nodes) {
    Array.prototype.forEach.call(this._nodes, this._unhighlightNode, this);
    this._nodes = (nodes == null) ?
        util.createEmptyNodeList(this._document) :
        nodes;
    Array.prototype.forEach.call(this._nodes, this._highlightNode, this);
  },
  get: function() {
    return this._nodes;
  },
  enumerable: true
});

Highlighter.prototype.destroy = function() {
  this.nodelist = null;
};

Highlighter.prototype._highlightNode = function(node) {
  if (node.hasAttribute(ATTR_NAME)) {
    return;
  }

  var styles = this._document.defaultView.getComputedStyle(node);
  node.setAttribute(ATTR_NAME, styles.border);
  node.style.border = HIGHLIGHT_STYLE;
};

Highlighter.prototype._unhighlightNode = function(node) {
  var previous = node.getAttribute(ATTR_NAME);
  node.style.border = previous;
  node.removeAttribute(ATTR_NAME);
};

exports.Highlighter = Highlighter;




exports.exec = function(execSpec) {
  var deferred = promise.defer();

  var output = { data: '' };
  var child = childProcess.spawn(execSpec.cmd, execSpec.args, {
    env: execSpec.env,
    cwd: execSpec.cwd
  });

  child.stdout.on('data', function(data) {
    output.data += data;
  });

  child.stderr.on('data', function(data) {
    output.data += data;
  });

  child.on('close', function(code) {
    output.code = code;
    if (code === 0) {
      deferred.resolve(output);
    }
    else {
      deferred.reject(output);
    }
  });

  return deferred.promise;
};









exports.staticRequire = function(requistingModule, name) {
  var deferred = promise.defer();

  var parent = path.dirname(requistingModule.id);
  var filename = parent + '/' + name;

  fs.readFile(filename, { encoding: 'utf8' }, function(err, data) {
    if (err) {
      deferred.reject(err);
    }

    deferred.resolve(data);
  });

  return deferred.promise;
};





exports.script = {
  onOutput: util.createEvent('Script.onOutput'),

  
  useTarget: function(tgt) { },

  
  eval: function(javascript) {
    try {
      return promise.resolve({
        input: javascript,
        output: eval(javascript),
        exception: null
      });
    }
    catch (ex) {
      return promise.resolve({
        input: javascript,
        output: null,
        exception: ex
      });
    }
  }
};
