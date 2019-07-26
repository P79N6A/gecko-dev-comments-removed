



"use strict";

const { Cc, Ci, Cu } = require("chrome");

const { XPCOMUtils } = require("resource://gre/modules/XPCOMUtils.jsm");
const Services = require("Services");

const promise = require("resource://gre/modules/Promise.jsm").Promise;
const { getRuleLocation } = require("devtools/server/actors/stylesheets");

const protocol = require("devtools/server/protocol");
const { method, custom, RetVal, Arg } = protocol;

loader.lazyGetter(this, "gDevTools", () => {
  return require("resource:///modules/devtools/gDevTools.jsm").gDevTools;
});
loader.lazyGetter(this, "DOMUtils", () => {
  return Cc["@mozilla.org/inspector/dom-utils;1"].getService(Ci.inIDOMUtils)
});
loader.lazyGetter(this, "prettifyCSS", () => {
  return require("resource:///modules/devtools/StyleSheetEditor.jsm").prettifyCSS;
});

const CSSRule = Ci.nsIDOMCSSRule;

const MAX_UNUSED_RULES = 10000;




const l10n = exports.l10n = {
  _URI: "chrome://global/locale/devtools/csscoverage.properties",
  lookup: function(msg) {
    if (this._stringBundle == null) {
      this._stringBundle = Services.strings.createBundle(this._URI);
    }
    return this._stringBundle.GetStringFromName(msg);
  }
};
































let UsageReportActor = protocol.ActorClass({
  typeName: "usageReport",

  initialize: function(conn, tabActor) {
    protocol.Actor.prototype.initialize.call(this, conn);

    this._tabActor = tabActor;
    this._running = false;

    this._onTabLoad = this._onTabLoad.bind(this);
    this._onChange = this._onChange.bind(this);
  },

  destroy: function() {
    this._tabActor = undefined;

    delete this._onTabLoad;
    delete this._onChange;

    protocol.Actor.prototype.destroy.call(this);
  },

  


  start: method(function() {
    if (this._running) {
      throw new Error(l10n.lookup("csscoverageRunningError"));
    }

    this._visitedPages = new Set();
    this._knownRules = new Map();
    this._running = true;
    this._tooManyUnused = false;

    this._tabActor.browser.addEventListener("load", this._onTabLoad, true);

    this._observeMutations(this._tabActor.window.document);

    this._populateKnownRules(this._tabActor.window.document);
    this._updateUsage(this._tabActor.window.document, false);
  }),

  


  stop: method(function() {
    if (!this._running) {
      throw new Error(l10n.lookup("csscoverageNotRunningError"));
    }

    this._tabActor.browser.removeEventListener("load", this._onTabLoad, true);
    this._running = false;
  }),

  


  toggle: method(function() {
    return this._running ?
        this.stop().then(() => false) :
        this.start().then(() => true);
  }, {
    response: RetVal("boolean"),
  }),

  



  oneshot: method(function() {
    if (this._running) {
      throw new Error(l10n.lookup("csscoverageRunningError"));
    }

    this._visitedPages = new Set();
    this._knownRules = new Map();

    this._populateKnownRules(this._tabActor.window.document);
    this._updateUsage(this._tabActor.window.document, false);
  }),

  


  _onTabLoad: function(ev) {
    let document = ev.target;
    this._populateKnownRules(document);
    this._updateUsage(document, true);

    this._observeMutations(document);
  },

  


  _observeMutations: function(document) {
    let MutationObserver = document.defaultView.MutationObserver;
    let observer = new MutationObserver(mutations => {
      
      
      this._onChange(document);
    });

    observer.observe(document, {
      attributes: true,
      childList: true,
      characterData: false
    });
  },

  



  _onChange: function(document) {
    
    if (!this._visitedPages.has(getURL(document))) {
      return;
    }
    this._updateUsage(document, false);
  },

  



  _populateKnownRules: function(document) {
    let url = getURL(document);
    this._visitedPages.add(url);
    
    
    for (let rule of getAllSelectorRules(document)) {
      let ruleId = ruleToId(rule);
      let ruleData = this._knownRules.get(ruleId);
      if (ruleData == null) {
        ruleData = {
           selectorText: rule.selectorText,
           cssText: rule.cssText,
           test: getTestSelector(rule.selectorText),
           isUsed: false,
           presentOn: new Set(),
           preLoadOn: new Set(),
           isError: false
        };
        this._knownRules.set(ruleId, ruleData);
      }

      ruleData.presentOn.add(url);
    }
  },

  


  _updateUsage: function(document, isLoad) {
    let qsaCount = 0;

    
    let url = getURL(document);

    for (let [ , ruleData ] of this._knownRules) {
      
      if (ruleData.isError) {
        continue;
      }

      
      
      if (!isLoad && ruleData.isUsed) {
        continue;
      }

      
      if (!ruleData.presentOn.has(url)) {
        continue;
      }

      qsaCount++;
      if (qsaCount > MAX_UNUSED_RULES) {
        console.error("Too many unused rules on " + url + " ");
        this._tooManyUnused = true;
        continue;
      }

      try {
        let match = document.querySelector(ruleData.test);
        if (match != null) {
          ruleData.isUsed = true;
          if (isLoad) {
            ruleData.preLoadOn.add(url);
          }
        }
      }
      catch (ex) {
        ruleData.isError = true;
      }
    }
  },

  












  createEditorReport: method(function(url) {
    if (this._knownRules == null) {
      return { reports: [] };
    }

    let reports = [];
    for (let [ruleId, ruleData] of this._knownRules) {
      let { url: ruleUrl, line, column } = deconstructRuleId(ruleId);
      if (ruleUrl !== url || ruleData.isUsed) {
        continue;
      }

      let ruleReport = {
        selectorText: ruleData.selectorText,
        start: { line: line, column: column }
      };

      if (ruleData.end) {
        ruleReport.end = ruleData.end;
      }

      reports.push(ruleReport);
    }

    return { reports: reports };
  }, {
    request: { url: Arg(0, "string") },
    response: { reports: RetVal("array:json") }
  }),

  
























  createPageReport: method(function() {
    if (this._running) {
      throw new Error(l10n.lookup("csscoverageRunningError"));
    }

    if (this._visitedPages == null) {
      throw new Error(l10n.lookup("csscoverageNotRunError"));
    }

    
    const ruleToRuleReport = function(ruleId, ruleData) {
      let { url, line, column } = deconstructRuleId(ruleId);
      return {
        url: url,
        shortHref: url.split("/").slice(-1),
        start: { line: line, column: column },
        selectorText: ruleData.selectorText,
        formattedCssText: prettifyCSS(ruleData.cssText)
      };
    }

    let pages = [];
    let unusedRules = [];

    
    for (let [ruleId, ruleData] of this._knownRules) {
      if (!ruleData.isUsed) {
        let ruleReport = ruleToRuleReport(ruleId, ruleData);
        unusedRules.push(ruleReport);
      }
    }

    
    for (let url of this._visitedPages) {
      let page = {
        url: url,
        shortHref: url.split("/").slice(-1),
        preloadRules: []
      };

      for (let [ruleId, ruleData] of this._knownRules) {
        if (ruleData.preLoadOn.has(url)) {
          let ruleReport = ruleToRuleReport(ruleId, ruleData);
          page.preloadRules.push(ruleReport);
        }
      }

      if (page.preloadRules.length > 0) {
        pages.push(page);
      }
    }

    return {
      pages: pages,
      unusedRules: unusedRules
    };
  }, {
    response: RetVal("json")
  }),

  


  _testOnly_isRunning: method(function() {
    return this._running;
  }, {
    response: { value: RetVal("boolean")}
  }),

});

exports.UsageReportActor = UsageReportActor;





function* getAllSelectorRules(document) {
  for (let rule of getAllRules(document)) {
    if (rule.type === CSSRule.STYLE_RULE && rule.selectorText !== "") {
      yield rule;
    }
  }
}





function* getAllRules(document) {
  
  let sheets = getAllSheets(document);
  for (let i = 0; i < sheets.length; i++) {
    for (let j = 0; j < sheets[i].cssRules.length; j++) {
      yield sheets[i].cssRules[j];
    }
  }
}






function getAllSheets(document) {
  
  let sheets = Array.slice(document.styleSheets);
  
  for (let i = 0; i < sheets.length; i++) {
    let subSheets = getImportedSheets(sheets[i]);
    sheets = sheets.concat(...subSheets);
  }
  return sheets;
}






function getImportedSheets(stylesheet) {
  let sheets = [];
  for (let i = 0; i < stylesheet.cssRules.length; i++) {
    let rule = stylesheet.cssRules[i];
    
    if (rule.type === CSSRule.IMPORT_RULE && rule.styleSheet != null) {
      sheets.push(rule.styleSheet);
      let subSheets = getImportedSheets(rule.styleSheet);
      sheets = sheets.concat(...subSheets);
    }
  }
  return sheets;
}






function ruleToId(rule) {
  let loc = getRuleLocation(rule);
  return sheetToUrl(rule.parentStyleSheet) + "|" + loc.line + "|" + loc.column;
}





const deconstructRuleId = exports.deconstructRuleId = function(ruleId) {
  let split = ruleId.split("|");
  if (split.length > 3) {
    let replace = split.slice(0, split.length - 3 + 1).join("|");
    split.splice(0, split.length - 3 + 1, replace);
  }
  let [ url, line, column ] = split;
  return {
    url: url,
    line: parseInt(line, 10),
    column: parseInt(column, 10)
  };
};







const getURL = exports.getURL = function(document) {
  let url = new document.defaultView.URL(document.documentURI);
  return '' + url.origin + url.pathname;
};

















const SEL_EXTERNAL = [
  "active", "active-drop", "current", "dir", "focus", "future", "hover",
  "invalid-drop",  "lang", "past", "placeholder-shown", "target", "valid-drop",
  "visited"
];









const SEL_FORM = [
  "checked", "default", "disabled", "enabled", "fullscreen", "in-range",
  "indeterminate", "invalid", "optional", "out-of-range", "required", "valid"
];








const SEL_ELEMENT = [
  "after", "before", "first-letter", "first-line", "selection"
];








const SEL_STRUCTURAL = [
  "empty", "first-child", "first-of-type", "last-child", "last-of-type",
  "nth-column", "nth-last-column", "nth-child", "nth-last-child",
  "nth-last-of-type", "nth-of-type", "only-child", "only-of-type", "root"
];







const SEL_SEMI = [ "any-link", "link", "read-only", "read-write", "scope" ];









const SEL_COMBINING = [ "not", "has", "matches" ];







const SEL_MEDIA = [ "blank", "first", "left", "right" ];






function getTestSelector(selector) {
  let replaceSelector = pseudo => {
    selector = selector.replace(" :" + selector, " *")
                       .replace("(:" + selector, "(*")
                       .replace(":" + selector, "");
  };

  SEL_EXTERNAL.forEach(replaceSelector);
  SEL_FORM.forEach(replaceSelector);
  SEL_ELEMENT.forEach(replaceSelector);

  
  SEL_ELEMENT.forEach(pseudo => {
    selector = selector.replace("::" + selector, "");
  });

  return selector;
}







exports.SEL_ALL = [
  SEL_EXTERNAL, SEL_FORM, SEL_ELEMENT, SEL_STRUCTURAL, SEL_SEMI,
  SEL_COMBINING, SEL_MEDIA
].reduce(function(prev, curr) { return prev.concat(curr); }, []);




const sheetToUrl = exports.sheetToUrl = function(stylesheet) {
  if (stylesheet.href) {
    return stylesheet.href;
  }
  if (stylesheet.ownerNode && stylesheet.ownerNode.baseURI) {
    return stylesheet.ownerNode.baseURI;
  }

  throw new Error("Unknown sheet source");
}




const UsageReportFront = protocol.FrontClass(UsageReportActor, {
  initialize: function(client, form) {
    protocol.Front.prototype.initialize.call(this, client, form);
    this.actorID = form.usageReportActor;
    this.manage(this);
  },

  start: custom(function(chromeWindow, target) {
    if (chromeWindow != null) {
      let gnb = chromeWindow.document.getElementById("global-notificationbox");
      let notification = gnb.getNotificationWithValue("csscoverage-running");

      if (!notification) {
        let notifyStop = ev => {
          if (ev == "removed") {
            this.stop();
            gDevTools.showToolbox(target, "styleeditor");
          }
        };

        gnb.appendNotification(l10n.lookup("csscoverageRunningReply"),
                               "csscoverage-running",
                               "", 
                               gnb.PRIORITY_INFO_HIGH,
                               null, 
                               notifyStop);
      }
    }

    return this._start();
  }, {
    impl: "_start"
  })
});

exports.UsageReportFront = UsageReportFront;




exports.register = function(handle) {
  handle.addGlobalActor(UsageReportActor, "usageReportActor");
  handle.addTabActor(UsageReportActor, "usageReportActor");
};

exports.unregister = function(handle) {
  handle.removeGlobalActor(UsageReportActor, "usageReportActor");
  handle.removeTabActor(UsageReportActor, "usageReportActor");
};

const knownFronts = new WeakMap();




const getUsage = exports.getUsage = function(target) {
  return target.makeRemote().then(() => {
    let front = knownFronts.get(target.client)
    if (front == null) {
      front = new UsageReportFront(target.client, target.form);
      knownFronts.set(target.client, front);
    }
    return front;
  });
};
