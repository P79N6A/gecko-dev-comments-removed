


"use strict";

this.EXPORTED_SYMBOLS = ["gDevToolsExtensions"];

Components.utils.import("resource://gre/modules/Services.jsm");

let globalsCache = {};

this.gDevToolsExtensions = {
  addContentGlobal: function(options) {
    if (!options || !options.global || !options['inner-window-id']) {
      throw Error('Invalid arguments');
    }
    let cache = getGlobalCache(options['inner-window-id']);
    cache.push(options.global);
    return undefined;
  },
  getContentGlobals: function(options) {
    if (!options || !options['inner-window-id']) {
      throw Error('Invalid arguments');
    }
    return Array.slice(globalsCache[options['inner-window-id']] || []);
  },
  removeContentGlobal: function(options) {
    if (!options || !options.global || !options['inner-window-id']) {
      throw Error('Invalid arguments');
    }
    let cache = getGlobalCache(options['inner-window-id']);
    let index = cache.indexOf(options.global);
    cache.splice(index, 1);
    return undefined;
  }
};

function getGlobalCache(aInnerWindowID) {
  return globalsCache[aInnerWindowID] = globalsCache[aInnerWindowID] || [];
}


Services.obs.addObserver(function observer(subject, topic, data) {
  let id = subject.QueryInterface(Components.interfaces.nsISupportsPRUint64).data;
  delete globalsCache[id];
}, 'inner-window-destroyed', false);
