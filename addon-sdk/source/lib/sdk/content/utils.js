


'use strict';

module.metadata = {
  'stability': 'unstable'
};

let { merge } = require('../util/object');
let assetsURI = require('../self').data.url();
let isArray = Array.isArray;
let method = require('method/core');

function isAddonContent({ contentURL }) {
  return typeof(contentURL) === 'string' && contentURL.indexOf(assetsURI) === 0;
}
exports.isAddonContent = isAddonContent;

function hasContentScript({ contentScript, contentScriptFile }) {
  return (isArray(contentScript) ? contentScript.length > 0 :
         !!contentScript) ||
         (isArray(contentScriptFile) ? contentScriptFile.length > 0 :
         !!contentScriptFile);
}
exports.hasContentScript = hasContentScript;

function requiresAddonGlobal(model) {
  return model.injectInDocument || (isAddonContent(model) && !hasContentScript(model));
}
exports.requiresAddonGlobal = requiresAddonGlobal;

function getAttachEventType(model) {
  if (!model) return null;
  let when = model.contentScriptWhen;
  return requiresAddonGlobal(model) ? 'document-element-inserted' :
         when === 'start' ? 'document-element-inserted' :
         when === 'end' ? 'load' :
         when === 'ready' ? 'DOMContentLoaded' :
         null;
}
exports.getAttachEventType = getAttachEventType;

let attach = method('worker-attach');
exports.attach = attach;

let detach = method('worker-detach');
exports.detach = detach;

let destroy = method('worker-destroy');
exports.destroy = destroy;

function WorkerHost (workerFor) {
  
  return ['postMessage', 'port', 'url', 'tab'].reduce(function(proto, name) {
    
    
    
    let descriptorProp = {
      value: function (...args) {
        let worker = workerFor(this);
        return worker[name].apply(worker, args);
      }
    };
    
    let accessorProp = {
      get: function () { return workerFor(this)[name]; },
      set: function (value) { workerFor(this)[name] = value; }
    };

    Object.defineProperty(proto, name, merge({
      enumerable: true,
      configurable: false,
    }, isDescriptor(name) ? descriptorProp : accessorProp));
    return proto;
  }, {});
  
  function isDescriptor (prop) {
    return ~['postMessage'].indexOf(prop);
  }
}
exports.WorkerHost = WorkerHost;
