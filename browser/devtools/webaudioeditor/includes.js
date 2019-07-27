


"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource:///modules/devtools/ViewHelpers.jsm");
Cu.import("resource:///modules/devtools/gDevTools.jsm");

const devtools = Cu.import("resource://gre/modules/devtools/Loader.jsm", {}).devtools;
const { require } = devtools;

let { console } = Cu.import("resource://gre/modules/devtools/Console.jsm", {});
let { EventTarget } = require("sdk/event/target");

const { Task } = Cu.import("resource://gre/modules/Task.jsm", {});
const { Class } = require("sdk/core/heritage");
const EventEmitter = require("devtools/toolkit/event-emitter");
const STRINGS_URI = "chrome://browser/locale/devtools/webaudioeditor.properties";
const L10N = new ViewHelpers.L10N(STRINGS_URI);

devtools.lazyImporter(this, "LineGraphWidget",
  "resource:///modules/devtools/Graphs.jsm");



let AUDIO_NODE_DEFINITION;


const { defer, all } = Cu.import("resource://gre/modules/Promise.jsm", {}).Promise;


const EVENTS = {
  
  
  START_CONTEXT: "WebAudioEditor:StartContext",

  
  THEME_CHANGE: "WebAudioEditor:ThemeChange",

  
  UI_RESET: "WebAudioEditor:UIReset",

  
  
  UI_SET_PARAM: "WebAudioEditor:UISetParam",

  
  UI_SELECT_NODE: "WebAudioEditor:UISelectNode",

  
  UI_INSPECTOR_NODE_SET: "WebAudioEditor:UIInspectorNodeSet",

  
  UI_INSPECTOR_TOGGLED: "WebAudioEditor:UIInspectorToggled",

  
  UI_PROPERTIES_TAB_RENDERED: "WebAudioEditor:UIPropertiesTabRendered",

  
  UI_AUTOMATION_TAB_RENDERED: "WebAudioEditor:UIAutomationTabRendered",

  
  
  
  
  UI_GRAPH_RENDERED: "WebAudioEditor:UIGraphRendered",

  
  UI_INSPECTOR_RESIZE: "WebAudioEditor:UIInspectorResize"
};




let gToolbox, gTarget, gFront;




EventEmitter.decorate(this);




function $(selector, target = document) { return target.querySelector(selector); }
function $$(selector, target = document) { return target.querySelectorAll(selector); }







function findWhere (collection, attrs) {
  let keys = Object.keys(attrs);
  for (let model of collection) {
    if (keys.every(key => model[key] === attrs[key])) {
      return model;
    }
  }
  return void 0;
}

function mixin (source, ...args) {
  args.forEach(obj => Object.keys(obj).forEach(prop => source[prop] = obj[prop]));
  return source;
}
