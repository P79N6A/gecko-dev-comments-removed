





"use strict";

module.metadata = {
  "stability": "experimental"
};

const { Cc, Ci } = require("chrome");
const errors = require("../deprecated/errors");
const { Class } = require("../core/heritage");
const { List, addListItem, removeListItem } = require("../util/list");
const { EventTarget } = require("../event/target");
const { emit } = require("../event/core");
const { create: makeFrame } = require("./utils");
const { defer, resolve } = require("../core/promise");
const { when: unload } = require("../system/unload");
const { validateOptions, getTypeOf } = require("../deprecated/api-utils");


let appShellService = Cc["@mozilla.org/appshell/appShellService;1"].
                        getService(Ci.nsIAppShellService);

let hiddenWindow = appShellService.hiddenDOMWindow;

if (!hiddenWindow) {
  throw new Error([
    "The hidden-frame module needs an app that supports a hidden window. ",
    "We would like it to support other applications, however. Please see ",
    "https://bugzilla.mozilla.org/show_bug.cgi?id=546740 for more information."
  ].join(""));
}



let cache = [];
let elements = new WeakMap();

function contentLoaded(target) {
  var deferred = defer();
  target.addEventListener("DOMContentLoaded", function DOMContentLoaded(event) {
    
    
    if (event.target === target || event.target === target.contentDocument) {
      target.removeEventListener("DOMContentLoaded", DOMContentLoaded, false);
      deferred.resolve(target);
    }
  }, false);
  return deferred.promise;
}

function makeHostFrame() {
  
  
  
  if (hiddenWindow.location.protocol == "chrome:" &&
      (hiddenWindow.document.contentType == "application/vnd.mozilla.xul+xml" ||
      hiddenWindow.document.contentType == "application/xhtml+xml")) {

    
    return resolve({ contentDocument: hiddenWindow.document });
  }
  else {
    return contentLoaded(makeFrame(hiddenWindow.document, {
      
      
      
      uri: "chrome://global/content/mozilla.xhtml",
      namespaceURI: hiddenWindow.document.documentElement.namespaceURI,
      nodeName: "iframe",
      allowJavascript: true,
      allowPlugins: true,
      allowAuth: true
    }));
  }
}
var hostFrame = makeHostFrame();

function FrameOptions(options) {
  options = options || {}
  return validateOptions(options, FrameOptions.validator);
}
FrameOptions.validator = {
  onReady: {
    is: ["undefined", "function", "array"],
    ok: function(v) {
      if (getTypeOf(v) === "array") {
        
        return v.every(function (item) typeof(item) === "function")
      }
      return true;
    }
  },
  onUnload: {
    is: ["undefined", "function"]
  }
};

var HiddenFrame = Class({
  extends: EventTarget,
  initialize: function initialize(options) {
    options = FrameOptions(options);
    EventTarget.prototype.initialize.call(this, options);
  },
  get element() {
    return elements.get(this);
  },
  toString: function toString() {
    return "[object Frame]"
  }
});
exports.HiddenFrame = HiddenFrame

function isFrameCached(frame) {
  
  return cache.some(function(value) {
    return value === frame
  })
}

function addHidenFrame(frame) {
  if (!(frame instanceof HiddenFrame))
    throw Error("The object to be added must be a HiddenFrame.");

  
  if (isFrameCached(frame)) return frame;
  else cache.push(frame);

  hostFrame.then(function({ contentDocument }) {
    let element = makeFrame(contentDocument, {
      nodeName: "iframe",
      type: "content",
      allowJavascript: true,
      allowPlugins: true,
      allowAuth: true,
    });
    elements.set(frame, element);
    return contentLoaded(element);
  }).then(function onFrameReady(element) {
    emit(frame, "ready");
  }, console.exception);

  return frame;
}
exports.add = addHidenFrame

function removeHiddenFrame(frame) {
  if (!(frame instanceof HiddenFrame))
    throw Error("The object to be removed must be a HiddenFrame.");

  if (!isFrameCached(frame)) return;

  
  cache.splice(cache.indexOf(frame), 1);
  emit(frame, "unload")
  let element = frame.element
  if (element) element.parentNode.removeChild(element)
}
exports.remove = removeHiddenFrame;

unload(function () {
  cache.splice(0).forEach(removeHiddenFrame);

  hostFrame.then(function(host) {
    if (hast.parentNode) frame.parentNode.removeChild(frame);
  });
});
