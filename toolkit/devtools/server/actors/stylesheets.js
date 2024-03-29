



"use strict";

let { components, Cc, Ci, Cu } = require("chrome");
let Services = require("Services");

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");
Cu.import("resource://gre/modules/devtools/SourceMap.jsm");
Cu.import("resource://gre/modules/Task.jsm");

const {Promise: promise} = Cu.import("resource://gre/modules/Promise.jsm", {});
const events = require("sdk/event/core");
const protocol = require("devtools/server/protocol");
const {Arg, Option, method, RetVal, types} = protocol;
const {LongStringActor, ShortLongString} = require("devtools/server/actors/string");
const {fetch} = require("devtools/toolkit/DevToolsUtils");
const {listenOnce} = require("devtools/async-utils");

loader.lazyGetter(this, "CssLogic", () => require("devtools/styleinspector/css-logic").CssLogic);

let TRANSITION_CLASS = "moz-styleeditor-transitioning";
let TRANSITION_DURATION_MS = 500;
let TRANSITION_BUFFER_MS = 1000;
let TRANSITION_RULE_SELECTOR =
".moz-styleeditor-transitioning:root, .moz-styleeditor-transitioning:root *";
let TRANSITION_RULE = TRANSITION_RULE_SELECTOR + " {\
transition-duration: " + TRANSITION_DURATION_MS + "ms !important; \
transition-delay: 0ms !important;\
transition-timing-function: ease-out !important;\
transition-property: all !important;\
}";

let LOAD_ERROR = "error-load";

types.addActorType("stylesheet");
types.addActorType("originalsource");





let StyleSheetsActor = exports.StyleSheetsActor = protocol.ActorClass({
  typeName: "stylesheets",

  


  get window() {
    return this.parentActor.window;
  },

  


  get document() {
    return this.window.document;
  },

  form: function()
  {
    return { actor: this.actorID };
  },

  initialize: function (conn, tabActor) {
    protocol.Actor.prototype.initialize.call(this, null);

    this.parentActor = tabActor;
  },

  



  getStyleSheets: method(Task.async(function* () {
    
    
    let windows = [this.window];
    let actors = [];

    for (let win of windows) {
      let sheets = yield this._addStyleSheets(win);
      actors = actors.concat(sheets);

      
      for (let iframe of win.document.querySelectorAll("iframe, browser, frame")) {
        if (iframe.contentDocument && iframe.contentWindow) {
          
          
          windows.push(iframe.contentWindow);
        }
      }
    }
    return actors;
  }), {
    request: {},
    response: { styleSheets: RetVal("array:stylesheet") }
  }),

  










  _shouldListSheet: function(doc, sheet) {
    
    
    
    if (sheet.href && sheet.href.toLowerCase() == "about:preferencestylesheet") {
      return false;
    }

    return true;
  },

  









  _addStyleSheets: function(win)
  {
    return Task.spawn(function*() {
      let doc = win.document;
      
      
      if (doc.readyState === "loading" || doc.readyState === "uninitialized") {
        
        yield listenOnce(win, "DOMContentLoaded", true);

        
        
        
        doc = win.document;
      }

      let isChrome = Services.scriptSecurityManager.isSystemPrincipal(doc.nodePrincipal);
      let styleSheets = isChrome ? DOMUtils.getAllStyleSheets(doc) : doc.styleSheets;
      let actors = [];
      for (let i = 0; i < styleSheets.length; i++) {
        let sheet = styleSheets[i];
        if (!this._shouldListSheet(doc, sheet)) {
          continue;
        }

        let actor = this.parentActor.createStyleSheetActor(sheet);
        actors.push(actor);

        
        let imports = yield this._getImported(doc, actor);
        actors = actors.concat(imports);
      }
      return actors;
    }.bind(this));
  },

  









  _getImported: function(doc, styleSheet) {
    return Task.spawn(function*() {
      let rules = yield styleSheet.getCSSRules();
      let imported = [];

      for (let i = 0; i < rules.length; i++) {
        let rule = rules[i];
        if (rule.type == Ci.nsIDOMCSSRule.IMPORT_RULE) {
          
          
          if (!rule.styleSheet || !this._shouldListSheet(doc, rule.styleSheet)) {
            continue;
          }
          let actor = this.parentActor.createStyleSheetActor(rule.styleSheet);
          imported.push(actor);

          
          let children = yield this._getImported(doc, actor);
          imported = imported.concat(children);
        }
        else if (rule.type != Ci.nsIDOMCSSRule.CHARSET_RULE) {
          
          break;
        }
      }

      return imported;
    }.bind(this));
  },


  








  addStyleSheet: method(function(text) {
    let parent = this.document.documentElement;
    let style = this.document.createElementNS("http://www.w3.org/1999/xhtml", "style");
    style.setAttribute("type", "text/css");

    if (text) {
      style.appendChild(this.document.createTextNode(text));
    }
    parent.appendChild(style);

    let actor = this.parentActor.createStyleSheetActor(style.sheet);
    return actor;
  }, {
    request: { text: Arg(0, "string") },
    response: { styleSheet: RetVal("stylesheet") }
  })
});




let StyleSheetsFront = protocol.FrontClass(StyleSheetsActor, {
  initialize: function(client, tabForm) {
    protocol.Front.prototype.initialize.call(this, client);
    this.actorID = tabForm.styleSheetsActor;
    this.manage(this);
  }
});





let MediaRuleActor = protocol.ActorClass({
  typeName: "mediarule",

  events: {
    "matches-change" : {
      type: "matchesChange",
      matches: Arg(0, "boolean"),
    }
  },

  get window() {
    return this.parentActor.window;
  },

  get document() {
    return this.window.document;
  },

  get matches() {
    return this.mql ? this.mql.matches : null;
  },

  initialize: function(aMediaRule, aParentActor) {
    protocol.Actor.prototype.initialize.call(this, null);

    this.rawRule = aMediaRule;
    this.parentActor = aParentActor;
    this.conn = this.parentActor.conn;

    this._matchesChange = this._matchesChange.bind(this);

    this.line = DOMUtils.getRuleLine(aMediaRule);
    this.column = DOMUtils.getRuleColumn(aMediaRule);

    try {
      this.mql = this.window.matchMedia(aMediaRule.media.mediaText);
    } catch(e) {
    }

    if (this.mql) {
      this.mql.addListener(this._matchesChange);
    }
  },

  destroy: function()
  {
    if (this.mql) {
      this.mql.removeListener(this._matchesChange);
    }

    protocol.Actor.prototype.destroy.call(this);
  },

  form: function(detail) {
    if (detail === "actorid") {
      return this.actorID;
    }

    let form = {
      actor: this.actorID,  
      mediaText: this.rawRule.media.mediaText,
      conditionText: this.rawRule.conditionText,
      matches: this.matches,
      line: this.line,
      column: this.column,
      parentStyleSheet: this.parentActor.actorID
    };

    return form;
  },

  _matchesChange: function() {
    events.emit(this, "matches-change", this.matches);
  }
});




let MediaRuleFront = protocol.FrontClass(MediaRuleActor, {
  initialize: function(client, form) {
    protocol.Front.prototype.initialize.call(this, client, form);

    this._onMatchesChange = this._onMatchesChange.bind(this);
    events.on(this, "matches-change", this._onMatchesChange);
  },

  _onMatchesChange: function(matches) {
    this._form.matches = matches;
  },

  form: function(form, detail) {
    if (detail === "actorid") {
      this.actorID = form;
      return;
    }
    this.actorID = form.actor;
    this._form = form;
  },

  get mediaText() {
    return this._form.mediaText;
  },
  get conditionText() {
    return this._form.conditionText;
  },
  get matches() {
    return this._form.matches;
  },
  get line() {
    return this._form.line || -1;
  },
  get column() {
    return this._form.column || -1;
  },
  get parentStyleSheet() {
    return this.conn.getActor(this._form.parentStyleSheet);
  }
});




let StyleSheetActor = protocol.ActorClass({
  typeName: "stylesheet",

  events: {
    "property-change" : {
      type: "propertyChange",
      property: Arg(0, "string"),
      value: Arg(1, "json")
    },
    "style-applied" : {
      type: "styleApplied"
    },
    "media-rules-changed" : {
      type: "mediaRulesChanged",
      rules: Arg(0, "array:mediarule")
    }
  },

  
  _originalSources: null,

  toString: function() {
    return "[StyleSheetActor " + this.actorID + "]";
  },

  


  get window() {
    return this._window || this.parentActor.window;
  },

  


  get document() {
    return this.window.document;
  },

  get ownerNode() {
    return this.rawSheet.ownerNode;
  },

  


  get href() {
    return this.rawSheet.href;
  },

  




  get styleSheetIndex()
  {
    if (this._styleSheetIndex == -1) {
      for (let i = 0; i < this.document.styleSheets.length; i++) {
        if (this.document.styleSheets[i] == this.rawSheet) {
          this._styleSheetIndex = i;
          break;
        }
      }
    }
    return this._styleSheetIndex;
  },

  initialize: function(aStyleSheet, aParentActor, aWindow) {
    protocol.Actor.prototype.initialize.call(this, null);

    this.rawSheet = aStyleSheet;
    this.parentActor = aParentActor;
    this.conn = this.parentActor.conn;

    this._window = aWindow;

    
    this.text = null;
    this._styleSheetIndex = -1;

    this._transitionRefCount = 0;
  },

  





  getCSSRules: function() {
    let rules;
    try {
      rules = this.rawSheet.cssRules;
    }
    catch (e) {
      
    }

    if (rules) {
      return promise.resolve(rules);
    }

    if (!this.ownerNode) {
      return promise.resolve([]);
    }

    if (this._cssRules) {
      return this._cssRules;
    }

    let deferred = promise.defer();

    let onSheetLoaded = (event) => {
      this.ownerNode.removeEventListener("load", onSheetLoaded, false);

      deferred.resolve(this.rawSheet.cssRules);
    };

    this.ownerNode.addEventListener("load", onSheetLoaded, false);

    
    this._cssRules = deferred.promise;

    return this._cssRules;
  },

  






  form: function(detail) {
    if (detail === "actorid") {
      return this.actorID;
    }

    let docHref;
    if (this.ownerNode) {
      if (this.ownerNode instanceof Ci.nsIDOMHTMLDocument) {
        docHref = this.ownerNode.location.href;
      }
      else if (this.ownerNode.ownerDocument && this.ownerNode.ownerDocument.location) {
        docHref = this.ownerNode.ownerDocument.location.href;
      }
    }

    let form = {
      actor: this.actorID,  
      href: this.href,
      nodeHref: docHref,
      disabled: this.rawSheet.disabled,
      title: this.rawSheet.title,
      system: !CssLogic.isContentStylesheet(this.rawSheet),
      styleSheetIndex: this.styleSheetIndex
    }

    try {
      form.ruleCount = this.rawSheet.cssRules.length;
    }
    catch(e) {
      
      this.getCSSRules().then(() => {
        this._notifyPropertyChanged("ruleCount");
      });
    }
    return form;
  },

  





  toggleDisabled: method(function() {
    this.rawSheet.disabled = !this.rawSheet.disabled;
    this._notifyPropertyChanged("disabled");

    return this.rawSheet.disabled;
  }, {
    response: { disabled: RetVal("boolean")}
  }),

  






  _notifyPropertyChanged: function(property) {
    events.emit(this, "property-change", property, this.form()[property]);
  },

  


  getText: method(function() {
    return this._getText().then((text) => {
      return new LongStringActor(this.conn, text || "");
    });
  }, {
    response: {
      text: RetVal("longstring")
    }
  }),

  






  _getText: function() {
    if (this.text) {
      return promise.resolve(this.text);
    }

    if (!this.href) {
      
      let content = this.ownerNode.textContent;
      this.text = content;
      return promise.resolve(content);
    }

    let options = {
      loadFromCache: true,
      policy: Ci.nsIContentPolicy.TYPE_STYLESHEET,
      window: this.window,
      charset: this._getCSSCharset()
    };

    return fetch(this.href, options).then(({ content }) => {
      this.text = content;
      return content;
    });
  },

  



  getOriginalSources: method(function() {
    if (this._originalSources) {
      return promise.resolve(this._originalSources);
    }
    return this._fetchOriginalSources();
  }, {
    request: {},
    response: {
      originalSources: RetVal("nullable:array:originalsource")
    }
  }),

  






  _fetchOriginalSources: function() {
    this._clearOriginalSources();
    this._originalSources = [];

    return this.getSourceMap().then((sourceMap) => {
      if (!sourceMap) {
        return null;
      }
      for (let url of sourceMap.sources) {
        let actor = new OriginalSourceActor(url, sourceMap, this);

        this.manage(actor);
        this._originalSources.push(actor);
      }
      return this._originalSources;
    })
  },

  






  getSourceMap: function() {
    if (this._sourceMap) {
      return this._sourceMap;
    }
    return this._fetchSourceMap();
  },

  





  _fetchSourceMap: function() {
    let deferred = promise.defer();

    this._getText().then((content) => {
      let url = this._extractSourceMapUrl(content);
      if (!url) {
        
        deferred.resolve(null);
        return;
      };

      url = normalize(url, this.href);
      let options = {
        loadFromCache: false,
        policy: Ci.nsIContentPolicy.TYPE_STYLESHEET,
        window: this.window
      };
      let map = fetch(url, options)
        .then(({content}) => {
          let map = new SourceMapConsumer(content);
          this._setSourceMapRoot(map, url, this.href);
          this._sourceMap = promise.resolve(map);

          deferred.resolve(map);
          return map;
        }, deferred.reject);

      this._sourceMap = map;
    }, deferred.reject);

    return deferred.promise;
  },

  


  _clearOriginalSources: function() {
    for (actor in this._originalSources) {
      this.unmanage(actor);
    }
    this._originalSources = null;
  },

  


  _setSourceMapRoot: function(aSourceMap, aAbsSourceMapURL, aScriptURL) {
    const base = dirname(
      aAbsSourceMapURL.startsWith("data:")
        ? aScriptURL
        : aAbsSourceMapURL);
    aSourceMap.sourceRoot = aSourceMap.sourceRoot
      ? normalize(aSourceMap.sourceRoot, base)
      : base;
  },

  







  _extractSourceMapUrl: function(content) {
    var matches = /sourceMappingURL\=([^\s\*]*)/.exec(content);
    if (matches) {
      return matches[1];
    }
    return null;
  },

  




  getOriginalLocation: method(function(line, column) {
    return this.getSourceMap().then((sourceMap) => {
      if (sourceMap) {
        return sourceMap.originalPositionFor({ line: line, column: column });
      }
      return {
        fromSourceMap: false,
        source: this.href,
        line: line,
        column: column
      };
    });
  }, {
    request: {
      line: Arg(0, "number"),
      column: Arg(1, "number")
    },
    response: RetVal(types.addDictType("originallocationresponse", {
      source: "string",
      line: "number",
      column: "number"
    }))
  }),

  


  getMediaRules: method(function() {
    return this._getMediaRules();
  }, {
    request: {},
    response: {
      mediaRules: RetVal("nullable:array:mediarule")
    }
  }),

  





  _getMediaRules: function() {
    return this.getCSSRules().then((rules) => {
      let mediaRules = [];
      for (let i = 0; i < rules.length; i++) {
        let rule = rules[i];
        if (rule.type != Ci.nsIDOMCSSRule.MEDIA_RULE) {
          continue;
        }
        let actor = new MediaRuleActor(rule, this);
        this.manage(actor);

        mediaRules.push(actor);
      }
      return mediaRules;
    });
  },

  






  _getCSSCharset: function(channelCharset)
  {
    
    if (channelCharset && channelCharset.length > 0) {
      
      return channelCharset;
    }

    let sheet = this.rawSheet;
    if (sheet) {
      
      
      if (sheet.cssRules) {
        let rules = sheet.cssRules;
        if (rules.length
            && rules.item(0).type == Ci.nsIDOMCSSRule.CHARSET_RULE) {
          return rules.item(0).encoding;
        }
      }

      
      if (sheet.ownerNode && sheet.ownerNode.getAttribute) {
        let linkCharset = sheet.ownerNode.getAttribute("charset");
        if (linkCharset != null) {
          return linkCharset;
        }
      }

      
      let parentSheet = sheet.parentStyleSheet;
      if (parentSheet && parentSheet.cssRules &&
          parentSheet.cssRules[0].type == Ci.nsIDOMCSSRule.CHARSET_RULE) {
        return parentSheet.cssRules[0].encoding;
      }

      
      if (sheet.ownerNode && sheet.ownerNode.ownerDocument.characterSet) {
        return sheet.ownerNode.ownerDocument.characterSet;
      }
    }

    
    return "UTF-8";
  },

  






  update: method(function(text, transition) {
    DOMUtils.parseStyleSheet(this.rawSheet, text);

    this.text = text;

    this._notifyPropertyChanged("ruleCount");

    if (transition) {
      this._insertTransistionRule();
    }
    else {
      events.emit(this, "style-applied");
    }

    this._getMediaRules().then((rules) => {
      events.emit(this, "media-rules-changed", rules);
    });
  }, {
    request: {
      text: Arg(0, "string"),
      transition: Arg(1, "boolean")
    }
  }),

  



  _insertTransistionRule: function() {
    this.document.documentElement.classList.add(TRANSITION_CLASS);

    
    this.rawSheet.insertRule(TRANSITION_RULE, this.rawSheet.cssRules.length);

    
    
    this.window.clearTimeout(this._transitionTimeout);
    this._transitionTimeout = this.window.setTimeout(this._onTransitionEnd.bind(this),
                              TRANSITION_DURATION_MS + TRANSITION_BUFFER_MS);
  },

  



  _onTransitionEnd: function()
  {
    this.document.documentElement.classList.remove(TRANSITION_CLASS);

    let index = this.rawSheet.cssRules.length - 1;
    let rule = this.rawSheet.cssRules[index];
    if (rule.selectorText == TRANSITION_RULE_SELECTOR) {
      this.rawSheet.deleteRule(index);
    }

    events.emit(this, "style-applied");
  }
})








const getRuleLocation = exports.getRuleLocation = function(rule) {
  let reply = {
    line: DOMUtils.getRuleLine(rule),
    column: DOMUtils.getRuleColumn(rule)
  };

  let sheet = rule.parentStyleSheet;
  if (sheet.ownerNode && sheet.ownerNode.localName === "style") {
     
     
     
     let text = sheet.ownerNode.textContent;
     
     
     let start = text.substring(0, text.indexOf("{"));
     let relativeStartLine = start.split("\n").length;

     let absoluteStartLine;
     let i = 0;
     while (absoluteStartLine == null) {
       let irule = sheet.cssRules[i];
       if (irule instanceof Ci.nsIDOMCSSStyleRule) {
         absoluteStartLine = DOMUtils.getRuleLine(irule);
       }
       else if (irule == null) {
         break;
       }

       i++;
     }

     if (absoluteStartLine != null) {
       let offset = absoluteStartLine - relativeStartLine;
       reply.line -= offset;
     }
  }

  return reply;
};




var StyleSheetFront = protocol.FrontClass(StyleSheetActor, {
  initialize: function(conn, form) {
    protocol.Front.prototype.initialize.call(this, conn, form);

    this._onPropertyChange = this._onPropertyChange.bind(this);
    events.on(this, "property-change", this._onPropertyChange);
  },

  destroy: function() {
    events.off(this, "property-change", this._onPropertyChange);
    protocol.Front.prototype.destroy.call(this);
  },

  _onPropertyChange: function(property, value) {
    this._form[property] = value;
  },

  form: function(form, detail) {
    if (detail === "actorid") {
      this.actorID = form;
      return;
    }
    this.actorID = form.actor;
    this._form = form;
  },

  get href() {
    return this._form.href;
  },
  get nodeHref() {
    return this._form.nodeHref;
  },
  get disabled() {
    return !!this._form.disabled;
  },
  get title() {
    return this._form.title;
  },
  get isSystem() {
    return this._form.system;
  },
  get styleSheetIndex() {
    return this._form.styleSheetIndex;
  },
  get ruleCount() {
    return this._form.ruleCount;
  }
});





let OriginalSourceActor = protocol.ActorClass({
  typeName: "originalsource",

  initialize: function(aUrl, aSourceMap, aParentActor) {
    protocol.Actor.prototype.initialize.call(this, null);

    this.url = aUrl;
    this.sourceMap = aSourceMap;
    this.parentActor = aParentActor;
    this.conn = this.parentActor.conn;

    this.text = null;
  },

  form: function() {
    return {
      actor: this.actorID, 
      url: this.url,
      relatedStyleSheet: this.parentActor.form()
    };
  },

  _getText: function() {
    if (this.text) {
      return promise.resolve(this.text);
    }
    let content = this.sourceMap.sourceContentFor(this.url);
    if (content) {
      this.text = content;
      return promise.resolve(content);
    }
    let options = {
      policy: Ci.nsIContentPolicy.TYPE_STYLESHEET,
      window: this.window
    };
    return fetch(this.url, options).then(({content}) => {
      this.text = content;
      return content;
    });
  },

  


  getText: method(function() {
    return this._getText().then((text) => {
      return new LongStringActor(this.conn, text || "");
    });
  }, {
    response: {
      text: RetVal("longstring")
    }
  })
})




let OriginalSourceFront = protocol.FrontClass(OriginalSourceActor, {
  initialize: function(client, form) {
    protocol.Front.prototype.initialize.call(this, client, form);

    this.isOriginalSource = true;
  },

  form: function(form, detail) {
    if (detail === "actorid") {
      this.actorID = form;
      return;
    }
    this.actorID = form.actor;
    this._form = form;
  },

  get href() {
    return this._form.url;
  },
  get url() {
    return this._form.url;
  }
});


XPCOMUtils.defineLazyGetter(this, "DOMUtils", function () {
  return Cc["@mozilla.org/inspector/dom-utils;1"].getService(Ci.inIDOMUtils);
});

exports.StyleSheetsActor = StyleSheetsActor;
exports.StyleSheetsFront = StyleSheetsFront;

exports.StyleSheetActor = StyleSheetActor;
exports.StyleSheetFront = StyleSheetFront;





function normalize(...aURLs) {
  let base = Services.io.newURI(aURLs.pop(), null, null);
  let url;
  while ((url = aURLs.pop())) {
    base = Services.io.newURI(url, null, base);
  }
  return base.spec;
}

function dirname(aPath) {
  return Services.io.newURI(
    ".", null, Services.io.newURI(aPath, null, null)).spec;
}
