




"use strict";

const { Cu, Cc, Ci, components } = require("chrome");

const TAB_SIZE    = "devtools.editor.tabsize";
const EXPAND_TAB  = "devtools.editor.expandtab";
const L10N_BUNDLE = "chrome://browser/locale/devtools/sourceeditor.properties";
const XUL_NS      = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";



const MAX_VERTICAL_OFFSET = 3;

const promise = require("sdk/core/promise");
const events  = require("devtools/shared/event-emitter");

Cu.import("resource://gre/modules/Services.jsm");
const L10N = Services.strings.createBundle(L10N_BUNDLE);





const CM_STYLES   = [
  "chrome://browser/skin/devtools/common.css",
  "chrome://browser/content/devtools/codemirror/codemirror.css",
  "chrome://browser/content/devtools/codemirror/dialog.css",
  "chrome://browser/content/devtools/codemirror/mozilla.css"
];

const CM_SCRIPTS  = [
  "chrome://browser/content/devtools/theme-switching.js",
  "chrome://browser/content/devtools/codemirror/codemirror.js",
  "chrome://browser/content/devtools/codemirror/dialog.js",
  "chrome://browser/content/devtools/codemirror/searchcursor.js",
  "chrome://browser/content/devtools/codemirror/search.js",
  "chrome://browser/content/devtools/codemirror/matchbrackets.js",
  "chrome://browser/content/devtools/codemirror/closebrackets.js",
  "chrome://browser/content/devtools/codemirror/comment.js",
  "chrome://browser/content/devtools/codemirror/javascript.js",
  "chrome://browser/content/devtools/codemirror/xml.js",
  "chrome://browser/content/devtools/codemirror/css.js",
  "chrome://browser/content/devtools/codemirror/htmlmixed.js",
  "chrome://browser/content/devtools/codemirror/activeline.js"
];

const CM_IFRAME   =
  "data:text/html;charset=utf8,<!DOCTYPE html>" +
  "<html dir='ltr'>" +
  "  <head>" +
  "    <style>" +
  "      html, body { height: 100%; }" +
  "      body { margin: 0; overflow: hidden; }" +
  "      .CodeMirror { width: 100%; height: 100% !important; }" +
  "    </style>" +
[ "    <link rel='stylesheet' href='" + style + "'>" for (style of CM_STYLES) ].join("\n") +
  "  </head>" +
  "  <body class='theme-body devtools-monospace'></body>" +
  "</html>";

const CM_MAPPING = [
  "focus",
  "hasFocus",
  "getCursor",
  "somethingSelected",
  "setSelection",
  "getSelection",
  "replaceSelection",
  "undo",
  "redo",
  "clearHistory",
  "openDialog",
  "cursorCoords",
  "lineCount",
  "refresh"
];

const CM_JUMP_DIALOG = [
  L10N.GetStringFromName("gotoLineCmd.promptTitle")
    + " <input type=text style='width: 10em'/>"
];

const { cssProperties, cssValues, cssColors } = getCSSKeywords();

const editors = new WeakMap();

Editor.modes = {
  text: { name: "text" },
  js:   { name: "javascript" },
  html: { name: "htmlmixed" },
  css:  { name: "css" }
};

function ctrl(k) {
  return (Services.appinfo.OS == "Darwin" ? "Cmd-" : "Ctrl-") + k;
}






















function Editor(config) {
  const tabSize = Services.prefs.getIntPref(TAB_SIZE);
  const useTabs = !Services.prefs.getBoolPref(EXPAND_TAB);

  this.version = null;
  this.config = {
    value:           "",
    mode:            Editor.modes.text,
    indentUnit:      tabSize,
    tabSize:         tabSize,
    contextMenu:     null,
    matchBrackets:   true,
    extraKeys:       {},
    indentWithTabs:  useTabs,
    styleActiveLine: true,
    theme: "mozilla"
  };

  
  Object.keys(config).forEach((k) => this.config[k] = config[k]);

  
  this.config.extraKeys[ctrl("J")] = (cm) => this.jumpToLine();
  this.config.extraKeys[ctrl("/")] = "toggleComment";

  
  
  this.config.extraKeys[ctrl("[")] = false;
  this.config.extraKeys[ctrl("]")] = false;

  
  
  
  
  this.config.extraKeys.Tab = (cm) => {
    if (cm.somethingSelected())
      return void cm.indentSelection("add");

    if (this.config.indentWithTabs)
      return void cm.replaceSelection("\t", "end", "+input");

    var num = cm.getOption("indentUnit");
    if (cm.getCursor().ch !== 0) num -= 1;
    cm.replaceSelection(" ".repeat(num), "end", "+input");
  };

  events.decorate(this);
}

Editor.prototype = {
  container: null,
  version: null,
  config: null,

  






  appendTo: function (el) {
    let def = promise.defer();
    let cm  = editors.get(this);
    let env = el.ownerDocument.createElement("iframe");
    env.flex = 1;

    if (cm)
      throw new Error("You can append an editor only once.");

    let onLoad = () => {
      
      

      env.removeEventListener("load", onLoad, true);
      let win = env.contentWindow.wrappedJSObject;

      CM_SCRIPTS.forEach((url) =>
        Services.scriptloader.loadSubScript(url, win, "utf8"));

      
      
      let cssSpec = win.CodeMirror.resolveMode("text/css");
      cssSpec.propertyKeywords = cssProperties;
      cssSpec.colorKeywords = cssColors;
      cssSpec.valueKeywords = cssValues;
      win.CodeMirror.defineMIME("text/css", cssSpec);

      let scssSpec = win.CodeMirror.resolveMode("text/x-scss");
      scssSpec.propertyKeywords = cssProperties;
      scssSpec.colorKeywords = cssColors;
      scssSpec.valueKeywords = cssValues;
      win.CodeMirror.defineMIME("text/x-scss", scssSpec);

      
      
      

      cm = win.CodeMirror(win.document.body, this.config);
      cm.getWrapperElement().addEventListener("contextmenu", (ev) => {
        ev.preventDefault();
        this.showContextMenu(el.ownerDocument, ev.screenX, ev.screenY);
      }, false);

      cm.on("focus", () => this.emit("focus"));
      cm.on("change", () => this.emit("change"));
      cm.on("gutterClick", (cm, line) => this.emit("gutterClick", line));
      cm.on("cursorActivity", (cm) => this.emit("cursorActivity"));

      win.CodeMirror.defineExtension("l10n", (name) => {
        return L10N.GetStringFromName(name);
      });

      cm.getInputField().controllers.insertControllerAt(0, controller(this));

      this.container = env;
      editors.set(this, cm);
      def.resolve();
    };

    env.addEventListener("load", onLoad, true);
    env.setAttribute("src", CM_IFRAME);
    el.appendChild(env);

    this.once("destroy", () => el.removeChild(env));
    return def.promise;
  },

  


  canUndo: function () {
    let cm = editors.get(this);
    return cm.historySize().undo > 0;
  },

  


  canRedo: function () {
    let cm = editors.get(this);
    return cm.historySize().redo > 0;
  },

  







  getPosition: function (...args) {
    let cm = editors.get(this);
    let res = args.map((ind) => cm.posFromIndex(ind));
    return args.length === 1 ? res[0] : res;
  },

  




  getOffset: function (...args) {
    let cm = editors.get(this);
    let res = args.map((pos) => cm.indexFromPos(pos));
    return args.length > 1 ? res : res[0];
  },

  



  getText: function (line) {
    let cm = editors.get(this);
    return line == null ?
      cm.getValue() : (cm.lineInfo(line) ? cm.lineInfo(line).text : "");
  },

  



  setText: function (value) {
    let cm = editors.get(this);
    cm.setValue(value);
  },

  



  setMode: function (value) {
    let cm = editors.get(this);
    cm.setOption("mode", value);
  },

  



  getMode: function () {
    let cm = editors.get(this);
    return cm.getOption("mode");
  },

  


  isReadOnly: function () {
    let cm = editors.get(this);
    return cm.getOption("readOnly");
  },

  





  replaceText: function (value, from, to) {
    let cm = editors.get(this);

    if (!from)
      return void this.setText(value);

    if (!to) {
      let text = cm.getRange({ line: 0, ch: 0 }, from);
      return void this.setText(text + value);
    }

    cm.replaceRange(value, from, to);
  },

  



  insertText: function (value, at) {
    let cm = editors.get(this);
    cm.replaceRange(value, at, at);
  },

  


  dropSelection: function () {
    if (!this.somethingSelected())
      return;

    this.setCursor(this.getCursor());
  },

  



  setClean: function () {
    let cm = editors.get(this);
    this.version = cm.changeGeneration();
    return this.version;
  },

  



  isClean: function () {
    let cm = editors.get(this);
    return cm.isClean(this.version);
  },

  





  showContextMenu: function (container, x, y) {
    if (this.config.contextMenu == null)
      return;

    let popup = container.getElementById(this.config.contextMenu);
    popup.openPopupAtScreen(x, y, true);
  },

  



  jumpToLine: function () {
    this.openDialog(CM_JUMP_DIALOG, (line) =>
      this.setCursor({ line: line - 1, ch: 0 }));
  },

  



  getPositionFromCoords: function (left, top) {
    let cm = editors.get(this);
    return cm.coordsChar({ left: left, top: top });
  },

  



  extendSelection: function (pos) {
    let cm = editors.get(this);
    let cursor = cm.indexFromPos(cm.getCursor());
    let anchor = cm.posFromIndex(cursor + pos.start);
    let head   = cm.posFromIndex(cursor + pos.start + pos.length);
    cm.setSelection(anchor, head);
  },

  
















  extend: function (funcs) {
    Object.keys(funcs).forEach((name) => {
      let cm  = editors.get(this);
      let ctx = { ed: this, cm: cm };

      if (name === "initialize")
        return void funcs[name](ctx);

      this[name] = funcs[name].bind(null, ctx);
    });
  },

  


  getFirstVisibleLine: function () {
    let cm = editors.get(this);
    return cm.lineAtHeight(0, "local");
  },

  


  setFirstVisibleLine: function (line) {
    let cm = editors.get(this);
    let { top } = cm.charCoords({line: line, ch: 0}, "local");
    cm.scrollTo(0, top);
  },

  




  setCursor: function ({line, ch}, align) {
    let cm = editors.get(this);
    this.alignLine(line, align);
    cm.setCursor({line: line, ch: ch});
  },

  




  alignLine: function(line, align) {
    let cm = editors.get(this);
    let from = cm.lineAtHeight(0, "page");
    let to = cm.lineAtHeight(cm.getWrapperElement().clientHeight, "page");
    let linesVisible = to - from;
    let halfVisible = Math.round(linesVisible/2);

    
    if (line <= to && line >= from) {
      return;
    }

    
    
    
    let offset = Math.min(halfVisible, MAX_VERTICAL_OFFSET);

    let topLine = {
      "center": Math.max(line - halfVisible, 0),
      "bottom": Math.max(line - linesVisible + offset, 0),
      "top": Math.max(line - offset, 0)
    }[align || "top"] || offset;

    
    topLine = Math.min(topLine, this.lineCount());
    this.setFirstVisibleLine(topLine);
  },

  


  hasMarker: function (line, gutterName, markerClass) {
    let cm = editors.get(this);
    let info = cm.lineInfo(line);
    if (!info)
      return false;

    let gutterMarkers = info.gutterMarkers;
    if (!gutterMarkers)
      return false;

    let marker = gutterMarkers[gutterName];
    if (!marker)
      return false;

    return marker.classList.contains(markerClass);
  },

  



  addMarker: function (line, gutterName, markerClass) {
    let cm = editors.get(this);
    let info = cm.lineInfo(line);
    if (!info)
      return;

    let gutterMarkers = info.gutterMarkers;
    if (gutterMarkers) {
      let marker = gutterMarkers[gutterName];
      if (marker) {
        marker.classList.add(markerClass);
        return;
      }
    }

    let marker = cm.getWrapperElement().ownerDocument.createElement("div");
    marker.className = markerClass;
    cm.setGutterMarker(info.line, gutterName, marker);
  },

  



  removeMarker: function (line, gutterName, markerClass) {
    if (!this.hasMarker(line, gutterName, markerClass))
      return;

    let cm = editors.get(this);
    cm.lineInfo(line).gutterMarkers[gutterName].classList.remove(markerClass);
  },

  


  removeAllMarkers: function (gutterName) {
    let cm = editors.get(this);
    cm.clearGutter(gutterName);
  },

  








  setMarkerListeners: function(line, gutterName, markerClass, events, data) {
    if (!this.hasMarker(line, gutterName, markerClass))
      return;

    let cm = editors.get(this);
    let marker = cm.lineInfo(line).gutterMarkers[gutterName];

    for (let name in events) {
      let listener = events[name].bind(this, line, marker, data);
      marker.addEventListener(name, listener);
    }
  },

  


  hasLineClass: function (line, className) {
    let cm = editors.get(this);
    let info = cm.lineInfo(line);
    if (!info)
      return false;

    return info.wrapClass == className;
  },

  


  addLineClass: function (line, className) {
    let cm = editors.get(this);
    cm.addLineClass(line, "wrap", className);
  },

  


  removeLineClass: function (line, className) {
    let cm = editors.get(this);
    cm.removeLineClass(line, "wrap", className);
  },

  destroy: function () {
    this.container = null;
    this.config = null;
    this.version = null;
    this.emit("destroy");
  }
};




CM_MAPPING.forEach(function (name) {
  Editor.prototype[name] = function (...args) {
    let cm = editors.get(this);
    return cm[name].apply(cm, args);
  };
});






function getCSSKeywords() {
  function keySet(array) {
    var keys = {};
    for (var i = 0; i < array.length; ++i) {
      keys[array[i]] = true;
    }
    return keys;
  }

  let domUtils = Cc["@mozilla.org/inspector/dom-utils;1"]
                   .getService(Ci.inIDOMUtils);
  let cssProperties = domUtils.getCSSPropertyNames(domUtils.INCLUDE_ALIASES);
  let cssColors = {};
  let cssValues = {};
  cssProperties.forEach(property => {
    if (property.contains("color")) {
      domUtils.getCSSValuesForProperty(property).forEach(value => {
        cssColors[value] = true;
      });
    }
    else {
      domUtils.getCSSValuesForProperty(property).forEach(value => {
        cssValues[value] = true;
      });
    }
  });
  return {
    cssProperties: keySet(cssProperties),
    cssValues: cssValues,
    cssColors: cssColors
  };
}






function controller(ed) {
  return {
    supportsCommand: function (cmd) {
      switch (cmd) {
        case "cmd_find":
        case "cmd_findAgain":
        case "cmd_findPrevious":
        case "cmd_gotoLine":
        case "cmd_undo":
        case "cmd_redo":
        case "cmd_delete":
        case "cmd_selectAll":
          return true;
      }

      return false;
    },

    isCommandEnabled: function (cmd) {
      let cm = editors.get(ed);

      switch (cmd) {
        case "cmd_find":
        case "cmd_gotoLine":
        case "cmd_selectAll":
          return true;
        case "cmd_findAgain":
          return cm.state.search != null && cm.state.search.query != null;
        case "cmd_undo":
          return ed.canUndo();
        case "cmd_redo":
          return ed.canRedo();
        case "cmd_delete":
          return ed.somethingSelected();
      }

      return false;
    },

    doCommand: function (cmd) {
      let cm  = editors.get(ed);
      let map = {
        "cmd_selectAll": "selectAll",
        "cmd_find": "find",
        "cmd_undo": "undo",
        "cmd_redo": "redo",
        "cmd_delete": "delCharAfter",
        "cmd_findAgain": "findNext"
      };

      if (map[cmd])
        return void cm.execCommand(map[cmd]);

      if (cmd == "cmd_gotoLine")
        ed.jumpToLine(cm);
    },

    onEvent: function () {}
  };
}

module.exports = Editor;
