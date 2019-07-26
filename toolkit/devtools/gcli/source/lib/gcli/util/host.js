















'use strict';

var Cc = require('chrome').Cc;
var Ci = require('chrome').Ci;

var Task = require('resource://gre/modules/Task.jsm').Task;

var promise = require('./promise');
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




exports.spawn = function(spawnSpec) {
  throw new Error('Not supported');
};




exports.exec = function(task) {
  return Task.spawn(task);
};






let resourceDirName = function(path) {
  let index = path.lastIndexOf("/");
  if (index == -1) {
    return ".";
  }
  while (index >= 0 && path[index] == "/") {
    --index;
  }
  return path.slice(0, index + 1);
};





exports.staticRequire = function(requistingModule, name) {
  var deferred = promise.defer();

  if (name.match(/\.css$/)) {
    deferred.resolve('');
  }
  else {
    var filename = resourceDirName(requistingModule.id) + '/' + name;
    filename = filename.replace(/\/\.\//g, '/');
    filename = 'resource://gre/modules/devtools/' + filename;

    var xhr = Cc['@mozilla.org/xmlextras/xmlhttprequest;1']
                .createInstance(Ci.nsIXMLHttpRequest);

    xhr.onload = function onload() {
      deferred.resolve(xhr.responseText);
    }.bind(this);

    xhr.onabort = xhr.onerror = xhr.ontimeout = function(err) {
      deferred.reject(err);
    }.bind(this);

    try {
      xhr.open('GET', filename);
      xhr.send();
    }
    catch (ex) {
      deferred.reject(ex);
    }
  }

  return deferred.promise;
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
                      promise.resolve(target) :
                      target.makeRemote();

  return targetPromise.then(function() {
    var deferred = promise.defer();

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
        deferred.reject(response);
      }
      else {
        deferred.resolve(response);
      }

      
    };

    var listeners = [ 'PageError', 'ConsoleAPI' ];
    client.attachConsole(consoleActor, listeners, onAttach);

    return deferred.promise;
  });
};




exports.script.eval = function(javascript) {
  var deferred = promise.defer();

  var onResult = function(response) {
    var output = response.result;
    if (typeof output === 'object' && output.type === 'undefined') {
      output = undefined;
    }

    deferred.resolve({
      input: response.input,
      output: output,
      exception: response.exception
    });
  };

  webConsoleClient.evaluateJS(javascript, onResult, {});
  return deferred.promise;
};
