





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
const { defer } = require("../core/promise");
const { when: unload } = require("../system/unload");
const { validateOptions, getTypeOf } = require("../deprecated/api-utils");
const { window } = require("../addon/window");



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

  let element = makeFrame(window.document, {
    nodeName: "iframe",
    type: "content",
    allowJavascript: true,
    allowPlugins: true,
    allowAuth: true,
  });
  elements.set(frame, element);

  contentLoaded(element).then(function onFrameReady(element) {
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

unload(function() cache.splice(0).forEach(removeHiddenFrame));
