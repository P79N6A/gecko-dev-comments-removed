





































"use strict";

const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource:///modules/source-editor-ui.jsm");

const PREF_EDITOR_COMPONENT = "devtools.editor.component";
const SOURCEEDITOR_L10N = "chrome://browser/locale/devtools/sourceeditor.properties";

var component = Services.prefs.getCharPref(PREF_EDITOR_COMPONENT);
var obj = {};
try {
  if (component == "ui") {
    throw new Error("The ui editor component is not available.");
  }
  Cu.import("resource:///modules/source-editor-" + component + ".jsm", obj);
} catch (ex) {
  Cu.reportError(ex);
  Cu.reportError("SourceEditor component failed to load: " + component);

  
  Services.prefs.clearUserPref(PREF_EDITOR_COMPONENT);

  
  component = Services.prefs.getCharPref(PREF_EDITOR_COMPONENT);
  Cu.import("resource:///modules/source-editor-" + component + ".jsm", obj);
}


var SourceEditor = obj.SourceEditor;
var EXPORTED_SYMBOLS = ["SourceEditor"];



XPCOMUtils.defineLazyGetter(SourceEditorUI, "strings", function() {
  return Services.strings.createBundle(SOURCEEDITOR_L10N);
});




SourceEditor.PREFS = {
  TAB_SIZE: "devtools.editor.tabsize",
  EXPAND_TAB: "devtools.editor.expandtab",
  COMPONENT: PREF_EDITOR_COMPONENT,
};




SourceEditor.MODES = {
  JAVASCRIPT: "js",
  CSS: "css",
  TEXT: "text",
  HTML: "html",
  XML: "xml",
};




SourceEditor.THEMES = {
  MOZILLA: "mozilla",
};




SourceEditor.DEFAULTS = {
  MODE: SourceEditor.MODES.TEXT,
  THEME: SourceEditor.THEMES.MOZILLA,
  UNDO_LIMIT: 200,
  TAB_SIZE: 4, 
  EXPAND_TAB: true, 
};




SourceEditor.EVENTS = {
  











  CONTEXT_MENU: "ContextMenu",

  







  TEXT_CHANGED: "TextChanged",

  






  SELECTION: "Selection",
};







function extend(aDestination, aSource)
{
  for (let name in aSource) {
    if (!aDestination.hasOwnProperty(name)) {
      aDestination[name] = aSource[name];
    }
  }
}




extend(SourceEditor.prototype, {
  _lastFind: null,

  
















  find: function SE_find(aString, aOptions)
  {
    if (typeof(aString) != "string") {
      return -1;
    }

    aOptions = aOptions || {};

    let str = aOptions.ignoreCase ? aString.toLowerCase() : aString;

    let text = this.getText();
    if (aOptions.ignoreCase) {
      text = text.toLowerCase();
    }

    let index = aOptions.backwards ?
                text.lastIndexOf(str, aOptions.start) :
                text.indexOf(str, aOptions.start);

    let lastFoundIndex = index;
    if (index == -1 && this.lastFind && this.lastFind.index > -1 &&
        this.lastFind.str === aString &&
        this.lastFind.ignoreCase === !!aOptions.ignoreCase) {
      lastFoundIndex = this.lastFind.index;
    }

    this._lastFind = {
      str: aString,
      index: index,
      lastFound: lastFoundIndex,
      ignoreCase: !!aOptions.ignoreCase,
    };

    return index;
  },

  








  findNext: function SE_findNext(aWrap)
  {
    if (!this.lastFind && this.lastFind.lastFound == -1) {
      return -1;
    }

    let options = {
      start: this.lastFind.lastFound + this.lastFind.str.length,
      ignoreCase: this.lastFind.ignoreCase,
    };

    let index = this.find(this.lastFind.str, options);
    if (index == -1 && aWrap) {
      options.start = 0;
      index = this.find(this.lastFind.str, options);
    }

    return index;
  },

  








  findPrevious: function SE_findPrevious(aWrap)
  {
    if (!this.lastFind && this.lastFind.lastFound == -1) {
      return -1;
    }

    let options = {
      start: this.lastFind.lastFound - this.lastFind.str.length,
      ignoreCase: this.lastFind.ignoreCase,
      backwards: true,
    };

    let index;
    if (options.start > 0) {
      index = this.find(this.lastFind.str, options);
    } else {
      index = this._lastFind.index = -1;
    }

    if (index == -1 && aWrap) {
      options.start = this.getCharCount() - 1;
      index = this.find(this.lastFind.str, options);
    }

    return index;
  },
});













Object.defineProperty(SourceEditor.prototype, "lastFind", {
  get: function() { return this._lastFind; },
  enumerable: true,
  configurable: true,
});

