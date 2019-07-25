





































"use strict";

const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");

const PREF_EDITOR_COMPONENT = "devtools.editor.component";

var component = Services.prefs.getCharPref(PREF_EDITOR_COMPONENT);
var obj = {};
try {
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

