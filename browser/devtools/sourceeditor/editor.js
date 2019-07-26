




"use strict";

const { Cu, Cc, Ci, components } = require("chrome");

const TAB_SIZE    = "devtools.editor.tabsize";
const EXPAND_TAB  = "devtools.editor.expandtab";
const L10N_BUNDLE = "chrome://browser/locale/devtools/sourceeditor.properties";
const XUL_NS      = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";

const promise = require("sdk/core/promise");
const events  = require("devtools/shared/event-emitter");

Cu.import("resource://gre/modules/Services.jsm");
const L10N = Services.strings.createBundle(L10N_BUNDLE);





const CM_STYLES   = [
  "chrome://browser/content/devtools/codemirror/codemirror.css",
  "chrome://browser/content/devtools/codemirror/dialog.css"
];

const CM_SCRIPTS  = [
  "chrome://browser/content/devtools/codemirror/codemirror.js",
  "chrome://browser/content/devtools/codemirror/dialog.js",
  "chrome://browser/content/devtools/codemirror/searchcursor.js",
  "chrome://browser/content/devtools/codemirror/search.js",
  "chrome://browser/content/devtools/codemirror/matchbrackets.js",
  "chrome://browser/content/devtools/codemirror/comment.js"
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
  "  <body></body>" +
  "</html>";

const CM_MAPPING = [
  "focus",
  "hasFocus",
  "setCursor",
  "getCursor",
  "somethingSelected",
  "setSelection",
  "getSelection",
  "replaceSelection",
  "undo",
  "redo",
  "clearHistory",
  "posFromIndex",
  "openDialog"
];

const CM_JUMP_DIALOG = [
  L10N.GetStringFromName("gotoLineCmd.promptTitle")
    + " <input type=text style='width: 10em'/>"
];

const editors = new WeakMap();

Editor.modes = {
  text: { name: "text" },
  js:   { name: "javascript", url: "chrome://browser/content/devtools/codemirror/javascript.js" }
};

function ctrl(k) {
  return (Services.appinfo.OS == "Darwin" ? "Cmd-" : "Ctrl-") + k;
}






















function Editor(config) {
  const tabSize = Services.prefs.getIntPref(TAB_SIZE);
  const useTabs = !Services.prefs.getBoolPref(EXPAND_TAB);

  this.version = null;
  this.config = {
    value:          "",
    mode:           Editor.modes.text,
    indentUnit:     tabSize,
    tabSize:        tabSize,
    contextMenu:    null,
    matchBrackets:  true,
    extraKeys:      {},
    indentWithTabs: useTabs,
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
  version: null,
  config:  null,

  






  appendTo: function (el) {
    let def = promise.defer();
    let cm  = editors.get(this);
    let doc = el.ownerDocument;
    let env = doc.createElementNS(XUL_NS, "iframe");
    env.flex = 1;

    if (cm)
      throw new Error("You can append an editor only once.");

    let onLoad = () => {
      
      
      env.removeEventListener("load", onLoad, true);
      let win = env.contentWindow.wrappedJSObject;

      CM_SCRIPTS.forEach((url) =>
        Services.scriptloader.loadSubScript(url, win, "utf8"));

      
      
      if (this.config.mode.name !== "text")
        Services.scriptloader.loadSubScript(this.config.mode.url, win, "utf8");

      
      
      

      cm = win.CodeMirror(win.document.body, this.config);
      cm.getWrapperElement().addEventListener("contextmenu", (ev) => {
        ev.preventDefault();
        this.showContextMenu(doc, ev.screenX, ev.screenY);
      }, false);

      cm.on("change", () => this.emit("change"));
      doc.defaultView.controllers.insertControllerAt(0, controller(this, doc.defaultView));

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

  


  getText: function () {
    let cm = editors.get(this);
    return cm.getValue();
  },

  



  setText: function (value) {
    let cm = editors.get(this);
    cm.setValue(value);
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

  


  dropSelection: function () {
    if (!this.somethingSelected())
      return;

    this.setCursor(this.getCursor());
  },

  



  markClean: function () {
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

  destroy: function () {
    this.config  = null;
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






function controller(ed, view) {
  return {
    supportsCommand: function (cmd) {
      let cm = editors.get(ed);

      switch (cmd) {
        case "cmd_find":
        case "cmd_findAgain":
        case "cmd_findPrevious":
        case "cmd_gotoLine":
        case "cmd_undo":
        case "cmd_redo":
        case "cmd_cut":
        case "cmd_paste":
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
        case "cmd_cut":
          return cm.getOption("readOnly") !== true && ed.somethingSelected();
        case "cmd_delete":
          return ed.somethingSelected();
        case "cmd_paste":
          return cm.getOption("readOnly") !== true;
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

      if (cmd === "cmd_cut")
        return void view.goDoCommand("cmd_cut");

      if (cmd === "cmdste")
        return void view.goDoCommand("cmd_paste");

      if (cmd == "cmd_gotoLine")
        ed.jumpToLine(cm);
    },

    onEvent: function () {}
  };
}

module.exports = Editor;