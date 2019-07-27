





"use strict";

const { noop } = require("devtools/toolkit/DevToolsUtils");


















function ScriptStore() {
  
  this._scripts = new NoDeleteSet;
}

module.exports = ScriptStore;

ScriptStore.prototype = {
  

  




  addScript(script) {
    this._scripts.add(script);
  },

  





  addScripts(scripts) {
    for (var i = 0, len = scripts.length; i < len; i++) {
      this.addScript(scripts[i]);
    }
  },

  

  




  getSources() {
    return [...new Set(this._scripts.items.map(s => s.source))];
  },

  







  getAllScripts() {
    return this._scripts.items;
  },

  getScriptsBySourceActor(sourceActor) {
    return sourceActor.source ?
           this.getScriptsBySource(sourceActor.source) :
           this.getScriptsByURL(sourceActor._originalUrl);
  },

  getScriptsBySourceActorAndLine(sourceActor, line) {
    return sourceActor.source ?
           this.getScriptsBySourceAndLine(sourceActor.source, line) :
           this.getScriptsByURLAndLine(sourceActor._originalUrl, line);
  },

  





  getScriptsBySource(source) {
    var results = [];
    var scripts = this._scripts.items;
    var length = scripts.length;
    for (var i = 0; i < length; i++) {
      if (scripts[i].source === source) {
        results.push(scripts[i]);
      }
    }
    return results;
  },

  







  getScriptsBySourceAndLine(source, line) {
    var results = [];
    var scripts = this._scripts.items;
    var length = scripts.length;
    for (var i = 0; i < length; i++) {
      var script = scripts[i];
      if (script.source === source &&
          script.startLine <= line &&
          (script.startLine + script.lineCount) > line) {
        results.push(script);
      }
    }
    return results;
  },

  





  getScriptsByURL(url) {
    var results = [];
    var scripts = this._scripts.items;
    var length = scripts.length;
    for (var i = 0; i < length; i++) {
      if (scripts[i].url === url) {
        results.push(scripts[i]);
      }
    }
    return results;
  },

  







  getScriptsByURLAndLine(url, line) {
    var results = [];
    var scripts = this._scripts.items;
    var length = scripts.length;
    for (var i = 0; i < length; i++) {
      var script = scripts[i];
      if (script.url === url &&
          script.startLine <= line &&
          (script.startLine + script.lineCount) > line) {
        results.push(script);
      }
    }
    return results;
  },
};









function NoDeleteSet() {
  this._set = new Set();
  this.items = [];
}

NoDeleteSet.prototype = {
  




  items: null,

  




  add(item) {
    if (!this._set.has(item)) {
      this._set.add(item);
      this.items.push(item);
    }
  },

  





  has(item) {
    return this._set.has(item);
  }
};
