




"use strict";

const { Cu, Cc, Ci, components } = require("chrome");

const TAB_SIZE    = "devtools.editor.tabsize";
const EXPAND_TAB  = "devtools.editor.expandtab";
const KEYMAP      = "devtools.editor.keymap";
const AUTO_CLOSE  = "devtools.editor.autoclosebrackets";
const L10N_BUNDLE = "chrome://browser/locale/devtools/sourceeditor.properties";
const XUL_NS      = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";



const MAX_VERTICAL_OFFSET = 3;

const {Promise: promise} = Cu.import("resource://gre/modules/Promise.jsm", {});
const events  = require("devtools/shared/event-emitter");

Cu.import("resource://gre/modules/Services.jsm");
const L10N = Services.strings.createBundle(L10N_BUNDLE);





const CM_STYLES   = [
  "chrome://browser/skin/devtools/common.css",
  "chrome://browser/content/devtools/codemirror/codemirror.css",
  "chrome://browser/content/devtools/codemirror/dialog.css",
  "chrome://browser/content/devtools/codemirror/mozilla.css",
  "chrome://browser/content/devtools/codemirror/foldgutter.css"
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
  "chrome://browser/content/devtools/codemirror/clike.js",
  "chrome://browser/content/devtools/codemirror/activeline.js",
  "chrome://browser/content/devtools/codemirror/trailingspace.js",
  "chrome://browser/content/devtools/codemirror/emacs.js",
  "chrome://browser/content/devtools/codemirror/vim.js",
  "chrome://browser/content/devtools/codemirror/foldcode.js",
  "chrome://browser/content/devtools/codemirror/brace-fold.js",
  "chrome://browser/content/devtools/codemirror/comment-fold.js",
  "chrome://browser/content/devtools/codemirror/xml-fold.js",
  "chrome://browser/content/devtools/codemirror/foldgutter.js"
];

const CM_IFRAME   =
  "data:text/html;charset=utf8,<!DOCTYPE html>" +
  "<html dir='ltr'>" +
  "  <head>" +
  "    <style>" +
  "      html, body { height: 100%; }" +
  "      body { margin: 0; overflow: hidden; }" +
  "      .CodeMirror { width: 100%; height: 100% !important; line-height: normal!important}" +
  "    </style>" +
[ "    <link rel='stylesheet' href='" + style + "'>" for (style of CM_STYLES) ].join("\n") +
  "  </head>" +
  "  <body class='theme-body devtools-monospace'></body>" +
  "</html>";

const CM_MAPPING = [
  "focus",
  "hasFocus",
  "lineCount",
  "somethingSelected",
  "getCursor",
  "setSelection",
  "getSelection",
  "replaceSelection",
  "extendSelection",
  "undo",
  "redo",
  "clearHistory",
  "openDialog",
  "refresh",
  "getScrollInfo",
  "getOption",
  "setOption"
];

const { cssProperties, cssValues, cssColors } = getCSSKeywords();

const editors = new WeakMap();

Editor.modes = {
  text: { name: "text" },
  html: { name: "htmlmixed" },
  css:  { name: "css" },
  js:   { name: "javascript" },
  vs:   { name: "x-shader/x-vertex" },
  fs:   { name: "x-shader/x-fragment" }
};






















function Editor(config) {
  const tabSize = Services.prefs.getIntPref(TAB_SIZE);
  const useTabs = !Services.prefs.getBoolPref(EXPAND_TAB);
  const keyMap = Services.prefs.getCharPref(KEYMAP);
  const useAutoClose = Services.prefs.getBoolPref(AUTO_CLOSE);

  this.version = null;
  this.config = {
    value:             "",
    mode:              Editor.modes.text,
    indentUnit:        tabSize,
    tabSize:           tabSize,
    contextMenu:       null,
    matchBrackets:     true,
    extraKeys:         {},
    indentWithTabs:    useTabs,
    styleActiveLine:   true,
    autoCloseBrackets: "()[]{}''\"\"",
    autoCloseEnabled:  useAutoClose,
    theme:             "mozilla"
  };

  
  this.config.extraKeys[Editor.keyFor("jumpToLine")] = () => this.jumpToLine();
  this.config.extraKeys[Editor.keyFor("moveLineUp")] = () => this.moveLineUp();
  this.config.extraKeys[Editor.keyFor("moveLineDown")] = () => this.moveLineDown();
  this.config.extraKeys[Editor.keyFor("toggleComment")] = "toggleComment";

  
  this.config.extraKeys[Editor.keyFor("indentLess")] = false;
  this.config.extraKeys[Editor.keyFor("indentMore")] = false;

  
  if (keyMap === "emacs" || keyMap === "vim")
    this.config.keyMap = keyMap;

  
  Object.keys(config).forEach((k) => {
    if (k != "extraKeys") {
      this.config[k] = config[k];
      return;
    }

    if (!config.extraKeys)
      return;

    Object.keys(config.extraKeys).forEach((key) => {
      this.config.extraKeys[key] = config.extraKeys[key];
    });
  });

  
  if (this.config.enableCodeFolding) {
    this.config.foldGutter = true;

    if (!this.config.gutters) {
      this.config.gutters = this.config.lineNumbers ? ["CodeMirror-linenumbers"] : [];
      this.config.gutters.push("CodeMirror-foldgutter");
    }
  }

  
  if (!this.config.autoCloseEnabled)
    this.config.autoCloseBrackets = false;

  
  
  
  
  this.config.extraKeys.Tab = (cm) => {
    if (cm.somethingSelected()) {
      cm.indentSelection("add");
      return;
    }

    if (this.config.indentWithTabs) {
      cm.replaceSelection("\t", "end", "+input");
      return;
    }

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

  







  appendTo: function (el, env) {
    let def = promise.defer();
    let cm  = editors.get(this);

    if (!env)
      env = el.ownerDocument.createElementNS(XUL_NS, "iframe");

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

      win.CodeMirror.commands.save = () => this.emit("save");

      
      
      

      cm = win.CodeMirror(win.document.body, this.config);
      cm.getWrapperElement().addEventListener("contextmenu", (ev) => {
        ev.preventDefault();
        if (!this.config.contextMenu) return;
        let popup = el.ownerDocument.getElementById(this.config.contextMenu);
        popup.openPopupAtScreen(ev.screenX, ev.screenY, true);
      }, false);

      cm.on("focus", () => this.emit("focus"));
      cm.on("scroll", () => this.emit("scroll"));
      cm.on("change", () => {
        this.emit("change");
        if (!this._lastDirty) {
          this._lastDirty = true;
          this.emit("dirty-change");
        }
      });
      cm.on("cursorActivity", (cm) => this.emit("cursorActivity"));

      cm.on("gutterClick", (cm, line, gutter, ev) => {
        let head = { line: line, ch: 0 };
        let tail = { line: line, ch: this.getText(line).length };

        
        if (ev.shiftKey) {
          cm.setSelection(head, tail);
          return;
        }

        this.emit("gutterClick", line);
      });

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

  



  getMode: function () {
    return this.getOption("mode");
  },

  



  setMode: function (value) {
    this.setOption("mode", value);
  },

  



  getText: function (line) {
    let cm = editors.get(this);

    if (line == null)
      return cm.getValue();

    let info = cm.lineInfo(line);
    return info ? cm.lineInfo(line).text : "";
  },

  



  setText: function (value) {
    let cm = editors.get(this);
    cm.setValue(value);
  },

  





  replaceText: function (value, from, to) {
    let cm = editors.get(this);

    if (!from) {
      this.setText(value);
      return;
    }

    if (!to) {
      let text = cm.getRange({ line: 0, ch: 0 }, from);
      this.setText(text + value);
      return;
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

    if (!info || !info.wrapClass)
      return false;

    return info.wrapClass.split(" ").indexOf(className) != -1;
  },

  


  addLineClass: function (line, className) {
    let cm = editors.get(this);
    cm.addLineClass(line, "wrap", className);
  },

  


  removeLineClass: function (line, className) {
    let cm = editors.get(this);
    cm.removeLineClass(line, "wrap", className);
  },

  




  markText: function(from, to, className = "marked-text") {
    let cm = editors.get(this);
    let text = cm.getRange(from, to);
    let span = cm.getWrapperElement().ownerDocument.createElement("span");
    span.className = className;
    span.textContent = text;

    let mark = cm.markText(from, to, { replacedWith: span });
    return {
      anchor: span,
      clear: () => mark.clear()
    };
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

  



  getPositionFromCoords: function ({left, top}) {
    let cm = editors.get(this);
    return cm.coordsChar({ left: left, top: top });
  },

  



  getCoordsFromPosition: function ({line, ch}) {
    let cm = editors.get(this);
    return cm.charCoords({ line: ~~line, ch: ~~ch });
  },

  


  canUndo: function () {
    let cm = editors.get(this);
    return cm.historySize().undo > 0;
  },

  


  canRedo: function () {
    let cm = editors.get(this);
    return cm.historySize().redo > 0;
  },

  



  setClean: function () {
    let cm = editors.get(this);
    this.version = cm.changeGeneration();
    this._lastDirty = false;
    this.emit("dirty-change");
    return this.version;
  },

  



  isClean: function () {
    let cm = editors.get(this);
    return cm.isClean(this.version);
  },

  



  jumpToLine: function () {
    let doc = editors.get(this).getWrapperElement().ownerDocument;
    let div = doc.createElement("div");
    let inp = doc.createElement("input");
    let txt = doc.createTextNode(L10N.GetStringFromName("gotoLineCmd.promptTitle"));

    inp.type = "text";
    inp.style.width = "10em";
    inp.style.MozMarginStart = "1em";

    div.appendChild(txt);
    div.appendChild(inp);

    this.openDialog(div, (line) => this.setCursor({ line: line - 1, ch: 0 }));
  },

  


  moveLineUp: function () {
    let cm = editors.get(this);
    let start = cm.getCursor("start");
    let end = cm.getCursor("end");

    if (start.line === 0)
      return;

    
    
    let value;
    if (start.line !== end.line) {
      value = cm.getRange({ line: start.line, ch: 0 },
        { line: end.line, ch: cm.getLine(end.line).length }) + "\n";
    } else {
      value = cm.getLine(start.line) + "\n";
    }
    value += cm.getLine(start.line - 1);

    
    
    cm.replaceRange(value, { line: start.line - 1, ch: 0 },
      { line: end.line, ch: cm.getLine(end.line).length });
    cm.setSelection({ line: start.line - 1, ch: start.ch },
      { line: end.line - 1, ch: end.ch });
  },

  


  moveLineDown: function () {
    let cm = editors.get(this);
    let start = cm.getCursor("start");
    let end = cm.getCursor("end");

    if (end.line + 1 === cm.lineCount())
      return;

    
    
    let value = cm.getLine(end.line + 1) + "\n";
    if (start.line !== end.line) {
      value += cm.getRange({ line: start.line, ch: 0 },
        { line: end.line, ch: cm.getLine(end.line).length });
    } else {
      value += cm.getLine(start.line);
    }

    
    
    cm.replaceRange(value, { line: start.line, ch: 0 },
      { line: end.line + 1, ch: cm.getLine(end.line + 1).length});
    cm.setSelection({ line: start.line + 1, ch: start.ch },
      { line: end.line + 1, ch: end.ch });
  },

  


  getFontSize: function () {
    let cm  = editors.get(this);
    let el  = cm.getWrapperElement();
    let win = el.ownerDocument.defaultView;

    return parseInt(win.getComputedStyle(el).getPropertyValue("font-size"), 10);
  },

  


  setFontSize: function (size) {
    let cm = editors.get(this);
    cm.getWrapperElement().style.fontSize = parseInt(size, 10) + "px";
  },

  
















  extend: function (funcs) {
    Object.keys(funcs).forEach((name) => {
      let cm  = editors.get(this);
      let ctx = { ed: this, cm: cm, Editor: Editor};

      if (name === "initialize") {
        funcs[name](ctx);
        return;
      }

      this[name] = funcs[name].bind(null, ctx);
    });
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











Editor.accel = function (key, modifiers={}) {
  return (modifiers.shift ? "Shift-" : "") +
         (Services.appinfo.OS == "Darwin" ? "Cmd-" : "Ctrl-") +
         (modifiers.alt ? "Alt-" : "") + key;
};







Editor.keyFor = function (cmd) {
  return Editor.accel(L10N.GetStringFromName(cmd + ".commandkey"));
};






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

      if (map[cmd]) {
        cm.execCommand(map[cmd]);
        return;
      }

      if (cmd == "cmd_gotoLine")
        ed.jumpToLine();
    },

    onEvent: function () {}
  };
}

module.exports = Editor;
