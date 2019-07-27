















'use strict';

var Cc = require('chrome').Cc;
var Ci = require('chrome').Ci;
var URL = require('sdk/url').URL;

var Task = require('resource://gre/modules/Task.jsm').Task;

var Promise = require('../util/promise').Promise;
var util = require('./util');

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
  
};

Highlighter.prototype._unhighlightNode = function(node) {
  
};

exports.Highlighter = Highlighter;




exports.spawn = function(context, spawnSpec) {
  throw new Error('Not supported');
};




exports.exec = function(task) {
  return Task.spawn(task);
};




exports.createUrl = function(uristr, base) {
  return URL(uristr, base);
};





exports.toDom = function(document, html) {
  var div = util.createElement(document, 'div');
  util.setContents(div, html);
  return div.children[0];
};






var resourceDirName = function(path) {
  var index = path.lastIndexOf('/');
  if (index == -1) {
    return '.';
  }
  while (index >= 0 && path[index] == '/') {
    --index;
  }
  return path.slice(0, index + 1);
};





exports.staticRequire = function(requistingModule, name) {
  if (name.match(/\.css$/)) {
    return Promise.resolve('');
  }
  else {
    return new Promise(function(resolve, reject) {
      var filename = resourceDirName(requistingModule.id) + '/' + name;
      filename = filename.replace(/\/\.\//g, '/');
      filename = 'resource://gre/modules/devtools/' + filename;

      var xhr = Cc['@mozilla.org/xmlextras/xmlhttprequest;1']
                  .createInstance(Ci.nsIXMLHttpRequest);

      xhr.onload = function onload() {
        resolve(xhr.responseText);
      }.bind(this);

      xhr.onabort = xhr.onerror = xhr.ontimeout = function(err) {
        reject(err);
      }.bind(this);

      xhr.open('GET', filename);
      xhr.send();
    }.bind(this));
  }
};





var client;
var target;
var consoleActor;
var webConsoleClient;

exports.script = { };

exports.script.onOutput = util.createEvent('Script.onOutput');




exports.script.useTarget = function(tgt) {
  target = tgt;

  
  var targetPromise = target.isRemote ?
                      Promise.resolve(target) :
                      target.makeRemote();

  return targetPromise.then(function() {
    return new Promise(function(resolve, reject) {
      client = target._client;

      client.addListener('pageError', function(packet) {
        if (packet.from === consoleActor) {
          
          exports.script.onOutput({
            level: 'exception',
            message: packet.exception.class
          });
        }
      });

      client.addListener('consoleAPICall', function(type, packet) {
        if (packet.from === consoleActor) {
          var data = packet.message;

          var ev = {
            level: data.level,
            arguments: data.arguments,
          };

          if (data.filename !== 'debugger eval code') {
            ev.source = {
              filename: data.filename,
              lineNumber: data.lineNumber,
              functionName: data.functionName
            };
          }

          exports.script.onOutput(ev);
        }
      });

      consoleActor = target._form.consoleActor;

      var onAttach = function(response, wcc) {
        webConsoleClient = wcc;

        if (response.error != null) {
          reject(response);
        }
        else {
          resolve(response);
        }

        
      };

      var listeners = [ 'PageError', 'ConsoleAPI' ];
      client.attachConsole(consoleActor, listeners, onAttach);
    }.bind(this));
  });
};




exports.script.evaluate = function(javascript) {
  return new Promise(function(resolve, reject) {
    var onResult = function(response) {
      var output = response.result;
      if (typeof output === 'object' && output.type === 'undefined') {
        output = undefined;
      }

      resolve({
        input: response.input,
        output: output,
        exception: response.exception
      });
    };

    webConsoleClient.evaluateJS(javascript, onResult, {});
  }.bind(this));
};
