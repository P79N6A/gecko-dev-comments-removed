


"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource:///modules/devtools/SideMenuWidget.jsm");
Cu.import("resource:///modules/devtools/ViewHelpers.jsm");
Cu.import("resource://gre/modules/devtools/Console.jsm");
Cu.import("resource:///modules/devtools/gDevTools.jsm");

const require = Cu.import("resource://gre/modules/devtools/Loader.jsm", {}).devtools.require;
const promise = Cu.import("resource://gre/modules/Promise.jsm", {}).Promise;
const EventEmitter = require("devtools/toolkit/event-emitter");
const { CallWatcherFront } = require("devtools/server/actors/call-watcher");
const { CanvasFront } = require("devtools/server/actors/canvas");
const Telemetry = require("devtools/shared/telemetry");
const telemetry = new Telemetry();

const CANVAS_ACTOR_RECORDING_ATTEMPT = gDevTools.testing ? 500 : 5000;

XPCOMUtils.defineLazyModuleGetter(this, "Task",
  "resource://gre/modules/Task.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PluralForm",
  "resource://gre/modules/PluralForm.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "FileUtils",
  "resource://gre/modules/FileUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
  "resource://gre/modules/NetUtil.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "DevToolsUtils",
  "resource://gre/modules/devtools/DevToolsUtils.jsm");


const EVENTS = {
  
  UI_RESET: "CanvasDebugger:UIReset",

  
  SNAPSHOTS_LIST_CLEARED: "CanvasDebugger:SnapshotsListCleared",

  
  
  SNAPSHOT_RECORDING_STARTED: "CanvasDebugger:SnapshotRecordingStarted",
  SNAPSHOT_RECORDING_FINISHED: "CanvasDebugger:SnapshotRecordingFinished",
  SNAPSHOT_RECORDING_COMPLETED: "CanvasDebugger:SnapshotRecordingCompleted",
  SNAPSHOT_RECORDING_CANCELLED: "CanvasDebugger:SnapshotRecordingCancelled",

  
  SNAPSHOT_RECORDING_SELECTED: "CanvasDebugger:SnapshotRecordingSelected",

  
  
  CALL_LIST_POPULATED: "CanvasDebugger:CallListPopulated",

  
  
  CALL_STACK_DISPLAYED: "CanvasDebugger:CallStackDisplayed",

  
  
  CALL_SCREENSHOT_DISPLAYED: "CanvasDebugger:ScreenshotDisplayed",

  
  
  THUMBNAILS_DISPLAYED: "CanvasDebugger:ThumbnailsDisplayed",

  
  SOURCE_SHOWN_IN_JS_DEBUGGER: "CanvasDebugger:SourceShownInJsDebugger",
  SOURCE_NOT_FOUND_IN_JS_DEBUGGER: "CanvasDebugger:SourceNotFoundInJsDebugger"
};

const HTML_NS = "http://www.w3.org/1999/xhtml";
const STRINGS_URI = "chrome://browser/locale/devtools/canvasdebugger.properties";
const SHARED_STRINGS_URI = "chrome://browser/locale/devtools/shared.properties";

const SNAPSHOT_START_RECORDING_DELAY = 10; 
const SNAPSHOT_DATA_EXPORT_MAX_BLOCK = 1000; 
const SNAPSHOT_DATA_DISPLAY_DELAY = 10; 
const SCREENSHOT_DISPLAY_DELAY = 100; 
const STACK_FUNC_INDENTATION = 14; 





const CALLS_LIST_SERIALIZER_IDENTIFIER = "Recorded Animation Frame Snapshot";
const CALLS_LIST_SERIALIZER_VERSION = 1;
const CALLS_LIST_SLOW_SAVE_DELAY = 100; 




let gToolbox, gTarget, gFront;




function startupCanvasDebugger() {
  return promise.all([
    EventsHandler.initialize(),
    SnapshotsListView.initialize(),
    CallsListView.initialize()
  ]);
}




function shutdownCanvasDebugger() {
  return promise.all([
    EventsHandler.destroy(),
    SnapshotsListView.destroy(),
    CallsListView.destroy()
  ]);
}




let EventsHandler = {
  


  initialize: function() {
    telemetry.toolOpened("canvasdebugger");
    this._onTabNavigated = this._onTabNavigated.bind(this);
    gTarget.on("will-navigate", this._onTabNavigated);
    gTarget.on("navigate", this._onTabNavigated);
  },

  


  destroy: function() {
    telemetry.toolClosed("canvasdebugger");
    gTarget.off("will-navigate", this._onTabNavigated);
    gTarget.off("navigate", this._onTabNavigated);
  },

  


  _onTabNavigated: function(event) {
    if (event != "will-navigate") {
      return;
    }
    
    gFront.setup({ reload: false });

    
    SnapshotsListView.empty();
    CallsListView.empty();

    $("#record-snapshot").removeAttribute("checked");
    $("#record-snapshot").removeAttribute("disabled");
    $("#record-snapshot").hidden = false;

    $("#reload-notice").hidden = true;
    $("#empty-notice").hidden = false;
    $("#waiting-notice").hidden = true;

    $("#debugging-pane-contents").hidden = true;
    $("#screenshot-container").hidden = true;
    $("#snapshot-filmstrip").hidden = true;

    window.emit(EVENTS.UI_RESET);
  }
};




let L10N = new ViewHelpers.L10N(STRINGS_URI);
let SHARED_L10N = new ViewHelpers.L10N(SHARED_STRINGS_URI);




EventEmitter.decorate(this);




function $(selector, target = document) target.querySelector(selector);
function $all(selector, target = document) target.querySelectorAll(selector);




function nsIURL(url, store = nsIURL.store) {
  if (store.has(url)) {
    return store.get(url);
  }
  let uri = Services.io.newURI(url, null, null).QueryInterface(Ci.nsIURL);
  store.set(url, uri);
  return uri;
}


nsIURL.store = new Map();




function getFileName(url) {
  try {
    let { fileName } = nsIURL(url);
    return fileName || "/";
  } catch (e) {
    
    return "";
  }
}















function getImageDataStorage(ctx, w, h) {
  let storage = getImageDataStorage.cache;
  if (storage && storage.width == w && storage.height == h) {
    return storage;
  }
  return getImageDataStorage.cache = ctx.createImageData(w, h);
}


getImageDataStorage.cache = null;





















function drawImage(canvas, width, height, pixels, options = {}) {
  let ctx = canvas.getContext("2d");

  
  
  if (pixels.length <= 1) {
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    return;
  }

  let imageData = getImageDataStorage(ctx, width, height);
  imageData.data.set(pixels);

  if (options.centered) {
    let left = (canvas.width - width) / 2;
    let top = (canvas.height - height) / 2;
    ctx.putImageData(imageData, left, top);
  } else {
    ctx.putImageData(imageData, 0, 0);
  }
}














function drawBackground(id, width, height, pixels) {
  let canvas = document.createElementNS(HTML_NS, "canvas");
  canvas.width = width;
  canvas.height = height;

  drawImage(canvas, width, height, pixels);
  document.mozSetImageElement(id, canvas);

  
  if (window._onMozSetImageElement) {
    window._onMozSetImageElement(pixels);
  }
}




function getNextDrawCall(calls, call) {
  for (let i = calls.indexOf(call) + 1, len = calls.length; i < len; i++) {
    let nextCall = calls[i];
    let name = nextCall.attachment.actor.name;
    if (CanvasFront.DRAW_CALLS.has(name)) {
      return nextCall;
    }
  }
  return null;
}





function getScreenshotFromCallLoadedFromDisk(calls, call) {
  for (let i = calls.indexOf(call); i >= 0; i--) {
    let prevCall = calls[i];
    let screenshot = prevCall.screenshot;
    if (screenshot) {
      return screenshot;
    }
  }
  return CanvasFront.INVALID_SNAPSHOT_IMAGE;
}




function getThumbnailForCall(thumbnails, index) {
  for (let i = thumbnails.length - 1; i >= 0; i--) {
    let thumbnail = thumbnails[i];
    if (thumbnail.index <= index) {
      return thumbnail;
    }
  }
  return CanvasFront.INVALID_SNAPSHOT_IMAGE;
}





function viewSourceInDebugger(url, line) {
  let showSource = ({ DebuggerView }) => {
    let item = DebuggerView.Sources.getItemForAttachment(a => a.source.url === url);
    if (item) {
      DebuggerView.setEditorLocation(item.attachment.source.actor, line, { noDebug: true }).then(() => {
        window.emit(EVENTS.SOURCE_SHOWN_IN_JS_DEBUGGER);
      }, () => {
        window.emit(EVENTS.SOURCE_NOT_FOUND_IN_JS_DEBUGGER);
      });
    }
  }

  
  
  
  let debuggerAlreadyOpen = gToolbox.getPanel("jsdebugger");
  gToolbox.selectTool("jsdebugger").then(({ panelWin: dbg }) => {
    if (debuggerAlreadyOpen) {
      showSource(dbg);
    } else {
      dbg.once(dbg.EVENTS.SOURCES_ADDED, () => showSource(dbg));
    }
  });
}
