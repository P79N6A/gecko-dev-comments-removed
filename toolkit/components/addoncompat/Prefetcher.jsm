



this.EXPORTED_SYMBOLS = ["Prefetcher"];

const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Preferences",
                                  "resource://gre/modules/Preferences.jsm");


let PrefetcherRules = {};



































































const PREF_PREFETCHING_ENABLED = "extensions.interposition.prefetching";

function isPrimitive(v) {
  if (!v)
    return true;
  let type = typeof(v);
  return type !== "object" && type !== "function";
}

function objAddr(obj)
{
  if (!isPrimitive(obj)) {
    return String(obj) + "[" + Cu.getJSTestingFunctions().objectAddress(obj) + "]";
  }
  return String(obj);
}

function log()
{







}

function logPrefetch()
{



}
















function PropertyOp(outputTable, inputTable, prop)
{
  this.outputTable = outputTable;
  this.inputTable = inputTable;
  this.prop = prop;
}

PropertyOp.prototype.addObject = function(database, obj)
{
  let has = false, propValue;
  try {
    if (this.prop in obj) {
      has = true;
      propValue = obj[this.prop];
    }
  } catch (e) {
    
    return;
  }

  logPrefetch("prop", obj, this.prop, propValue);
  database.cache(this.index, obj, has, propValue);
  if (has && !isPrimitive(propValue) && this.outputTable) {
    database.add(this.outputTable, propValue);
  }
}

PropertyOp.prototype.makeCacheEntry = function(item, cache)
{
  let [index, obj, has, propValue] = item;

  let desc = { configurable: false, enumerable: true, writable: false, value: propValue };

  if (!cache.has(obj)) {
    cache.set(obj, new Map());
  }
  let propMap = cache.get(obj);
  propMap.set(this.prop, desc);
}

function MethodOp(outputTable, inputTable, method, ...args)
{
  this.outputTable = outputTable;
  this.inputTable = inputTable;
  this.method = method;
  this.args = args;
}

MethodOp.prototype.addObject = function(database, obj)
{
  let result;
  try {
    result = obj[this.method].apply(obj, this.args);
  } catch (e) {
    
    return;
  }

  logPrefetch("method", obj, this.method + "(" + this.args + ")", result);
  database.cache(this.index, obj, result);
  if (!isPrimitive(result) && this.outputTable) {
    database.add(this.outputTable, result);
  }
}

MethodOp.prototype.makeCacheEntry = function(item, cache)
{
  let [index, obj, result] = item;

  if (!cache.has(obj)) {
    cache.set(obj, new Map());
  }
  let propMap = cache.get(obj);
  let fallback = propMap.get(this.method);

  let method = this.method;
  let selfArgs = this.args;
  let methodImpl = function(...args) {
    if (args.length == selfArgs.length && args.every((v, i) => v === selfArgs[i])) {
      return result;
    }

    if (fallback) {
      return fallback.value(...args);
    } else {
      return obj[method](...args);
    }
  };

  let desc = { configurable: false, enumerable: true, writable: false, value: methodImpl };
  propMap.set(this.method, desc);
}

function CollectionOp(outputTable, inputTable)
{
  this.outputTable = outputTable;
  this.inputTable = inputTable;
}

CollectionOp.prototype.addObject = function(database, obj)
{
  let elements = [];
  try {
    let len = obj.length;
    for (let i = 0; i < len; i++) {
      logPrefetch("index", obj, i, obj[i]);
      elements.push(obj[i]);
    }
  } catch (e) {
    
    return;
  }

  database.cache(this.index, obj, ...elements);
  for (let i = 0; i < elements.length; i++) {
    if (!isPrimitive(elements[i]) && this.outputTable) {
      database.add(this.outputTable, elements[i]);
    }
  }
}

CollectionOp.prototype.makeCacheEntry = function(item, cache)
{
  let [index, obj, ...elements] = item;

  if (!cache.has(obj)) {
    cache.set(obj, new Map());
  }
  let propMap = cache.get(obj);

  let lenDesc = { configurable: false, enumerable: true, writable: false, value: elements.length };
  propMap.set("length", lenDesc);

  for (let i = 0; i < elements.length; i++) {
    let desc = { configurable: false, enumerable: true, writable: false, value: elements[i] };
    propMap.set(i, desc);
  }
}

function CopyOp(outputTable, inputTable)
{
  this.outputTable = outputTable;
  this.inputTable = inputTable;
}

CopyOp.prototype.addObject = function(database, obj)
{
  database.add(this.outputTable, obj);
}

function Database(trigger, addons)
{
  
  
  this.rules = new Map();
  for (let addon of addons) {
    let addonRules = PrefetcherRules[addon] || {};
    let triggerRules = addonRules[trigger] || [];
    for (let rule of triggerRules) {
      let inTable = rule.inputTable;
      if (!this.rules.has(inTable)) {
        this.rules.set(inTable, new Set());
      }
      let set = this.rules.get(inTable);
      set.add(rule);
    }
  }

  
  this.tables = new Map();

  
  
  this.todo = [];

  
  this.cached = [];
}

Database.prototype = {
  
  add: function(table, obj) {
    if (!this.tables.has(table)) {
      this.tables.set(table, new Set());
    }
    let tableSet = this.tables.get(table);
    if (tableSet.has(obj)) {
      return;
    }
    tableSet.add(obj);

    this.todo.push([table, obj]);
  },

  cache: function(...args) {
    this.cached.push(args);
  },

  
  
  process: function() {
    while (this.todo.length) {
      let [table, obj] = this.todo.pop();
      let rules = this.rules.get(table);
      if (!rules) {
        continue;
      }
      for (let rule of rules) {
        rule.addObject(this, obj);
      }
    }
  },
};

let Prefetcher = {
  init: function() {
    
    
    
    let counter = 0;
    this.ruleMap = new Map();
    for (let addon in PrefetcherRules) {
      for (let trigger in PrefetcherRules[addon]) {
        for (let rule of PrefetcherRules[addon][trigger]) {
          rule.index = counter++;
          this.ruleMap.set(rule.index, rule);
        }
      }
    }

    this.prefetchingEnabled = Preferences.get(PREF_PREFETCHING_ENABLED, false);
    Services.prefs.addObserver(PREF_PREFETCHING_ENABLED, this, false);
    Services.obs.addObserver(this, "xpcom-shutdown", false);
  },

  observe: function(subject, topic, data) {
    if (topic == "xpcom-shutdown") {
      Services.prefs.removeObserver(PREF_PREFETCHING_ENABLED, this);
      Services.obs.removeObserver(this, "xpcom-shutdown");
    } else if (topic == PREF_PREFETCHING_ENABLED) {
      this.prefetchingEnabled = Preferences.get(PREF_PREFETCHING_ENABLED, false);
    }
  },

  
  
  
  
  prefetch: function(trigger, addons, args) {
    if (!this.prefetchingEnabled) {
      return [[], []];
    }

    let db = new Database(trigger, addons);
    for (let table in args) {
      log("root", table, "=", objAddr(args[table]));
      db.add(table, args[table]);
    }

    
    db.process();

    
    
    
    
    let cpowIndexes = new Map();
    let prefetched = [];
    let cpows = [];
    for (let item of db.cached) {
      item = item.map((elt) => {
        if (!isPrimitive(elt)) {
          if (!cpowIndexes.has(elt)) {
            let index = cpows.length;
            cpows.push(elt);
            cpowIndexes.set(elt, index);
          }
          return {cpow: cpowIndexes.get(elt)};
        } else {
          return elt;
        }
      });

      prefetched.push(item);
    }

    return [prefetched, cpows];
  },

  cache: null,

  
  
  generateCache: function(prefetched, cpows) {
    let cache = new Map();
    for (let item of prefetched) {
      
      
      item = item.map((elt) => {
        if (!isPrimitive(elt)) {
          return cpows[elt.cpow];
        }
        return elt;
      });

      let index = item[0];
      let op = this.ruleMap.get(index);
      op.makeCacheEntry(item, cache);
    }
    return cache;
  },

  
  
  withPrefetching: function(prefetched, cpows, func) {
    if (!this.prefetchingEnabled) {
      return func();
    }

    this.cache = this.generateCache(prefetched, cpows);

    try {
      log("Prefetching on");
      return func();
    } finally {
      
      
      log("Prefetching off");
      this.cache = null;
    }
  },

  
  
  lookupInCache: function(addon, target, prop) {
    if (!this.cache || !Cu.isCrossProcessWrapper(target)) {
      return null;
    }

    let propMap = this.cache.get(target);
    if (!propMap) {
      return null;
    }

    return propMap.get(prop);
  },
};

let AdblockId = "{d10d0bf8-f5b5-c8b4-a8b2-2b9879e08c5d}";
let AdblockRules = {
  "ContentPolicy.shouldLoad": [
    new MethodOp("Node", "InitNode", "QueryInterface", Ci.nsISupports),
    new PropertyOp("Document", "Node", "ownerDocument"),
    new PropertyOp("Window", "Node", "defaultView"),
    new PropertyOp("Window", "Document", "defaultView"),
    new PropertyOp("TopWindow", "Window", "top"),
    new PropertyOp("WindowLocation", "Window", "location"),
    new PropertyOp(null, "WindowLocation", "href"),
    new PropertyOp("Window", "Window", "parent"),
    new PropertyOp(null, "Window", "name"),
    new PropertyOp("Document", "Window", "document"),
    new PropertyOp("TopDocumentElement", "Document", "documentElement"),
    new MethodOp(null, "TopDocumentElement", "getAttribute", "data-adblockkey"),
  ]
};
PrefetcherRules[AdblockId] = AdblockRules;

let LastpassId = "support@lastpass.com";
let LastpassRules = {
  "EventTarget.handleEvent": [
    new PropertyOp("EventTarget", "Event", "target"),
    new PropertyOp("EventOriginalTarget", "Event", "originalTarget"),
    new PropertyOp("Window", "EventOriginalTarget", "defaultView"),

    new CopyOp("Frame", "Window"),
    new PropertyOp("FrameCollection", "Window", "frames"),
    new CollectionOp("Frame", "FrameCollection"),
    new PropertyOp("FrameCollection", "Frame", "frames"),
    new PropertyOp("FrameDocument", "Frame", "document"),
    new PropertyOp(null, "Frame", "window"),
    new PropertyOp(null, "FrameDocument", "defaultView"),

    new PropertyOp("FrameDocumentLocation", "FrameDocument", "location"),
    new PropertyOp(null, "FrameDocumentLocation", "href"),
    new PropertyOp("FrameLocation", "Frame", "location"),
    new PropertyOp(null, "FrameLocation", "href"),

    new MethodOp("FormCollection", "FrameDocument", "getElementsByTagName", "form"),
    new MethodOp("FormCollection", "FrameDocument", "getElementsByTagName", "FORM"),
    new CollectionOp("Form", "FormCollection"),
    new PropertyOp("FormElementCollection", "Form", "elements"),
    new CollectionOp("FormElement", "FormElementCollection"),
    new PropertyOp("Style", "Form", "style"),

    new PropertyOp(null, "FormElement", "type"),
    new PropertyOp(null, "FormElement", "name"),
    new PropertyOp(null, "FormElement", "value"),
    new PropertyOp(null, "FormElement", "tagName"),
    new PropertyOp(null, "FormElement", "id"),
    new PropertyOp("Style", "FormElement", "style"),

    new PropertyOp(null, "Style", "visibility"),

    new MethodOp("MetaElementsCollection", "EventOriginalTarget", "getElementsByTagName", "meta"),
    new CollectionOp("MetaElement", "MetaElementsCollection"),
    new PropertyOp(null, "MetaElement", "httpEquiv"),

    new MethodOp("InputElementCollection", "FrameDocument", "getElementsByTagName", "input"),
    new MethodOp("InputElementCollection", "FrameDocument", "getElementsByTagName", "INPUT"),
    new CollectionOp("InputElement", "InputElementCollection"),
    new PropertyOp(null, "InputElement", "type"),
    new PropertyOp(null, "InputElement", "name"),
    new PropertyOp(null, "InputElement", "tagName"),
    new PropertyOp(null, "InputElement", "form"),

    new PropertyOp("BodyElement", "FrameDocument", "body"),
    new PropertyOp("BodyInnerText", "BodyElement", "innerText"),

    new PropertyOp("DocumentFormCollection", "FrameDocument", "forms"),
    new CollectionOp("DocumentForm", "DocumentFormCollection"),
  ]
};
PrefetcherRules[LastpassId] = LastpassRules;
