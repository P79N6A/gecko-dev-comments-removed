


"use strict";

module.metadata = {
  "stability": "experimental"
};

const { Ci } = require("chrome");
const { dispatcher } = require("../util/dispatcher");
const { add, remove, iterator } = require("../lang/weak-set");

let getTargetWindow = dispatcher("getTargetWindow");

getTargetWindow.define(function (target) {
  if (target instanceof Ci.nsIDOMWindow)
    return target;
  if (target instanceof Ci.nsIDOMDocument)
    return target.defaultView || null;

  return null;
});

exports.getTargetWindow = getTargetWindow;

let attachTo = dispatcher("attachTo");
exports.attachTo = attachTo;

let detachFrom = dispatcher("detatchFrom");
exports.detachFrom = detachFrom;

function attach(modification, target) {
  if (!modification)
    return;

  let window = getTargetWindow(target);

  attachTo(modification, window);

  
  
  add(modification, window.document);
}
exports.attach = attach;

function detach(modification, target) {
  if (!modification)
    return;

  if (target) {
    let window = getTargetWindow(target);
    detachFrom(modification, window);
    remove(modification, window.document);
  }
  else {
    let documents = iterator(modification);
    for (let document of documents) {
      detachFrom(modification, document.defaultView);
      remove(modification, document);
    }
  }
}
exports.detach = detach;
