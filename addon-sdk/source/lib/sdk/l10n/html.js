


"use strict";

module.metadata = {
  "stability": "unstable"
};

const { Ci } = require("chrome");
const events = require("../system/events");
const core = require("./core");
const { loadSheet, removeSheet } = require("../stylesheet/utils");

const assetsURI = require('../self').data.url();

const hideSheetUri = "data:text/css,:root {visibility: hidden !important;}";



function translateElement(element) {
  element = element || document;

  
  var children = element.querySelectorAll('*[data-l10n-id]');
  var elementCount = children.length;
  for (var i = 0; i < elementCount; i++) {
    var child = children[i];

    
    var key = child.dataset.l10nId;
    var data = core.get(key);
    if (data)
      child.textContent = data;
  }
}
exports.translateElement = translateElement;

function onDocumentReady2Translate(event) {
  let document = event.target;
  document.removeEventListener("DOMContentLoaded", onDocumentReady2Translate,
                               false);

  translateElement(document);

  try {
    
    if (document.defaultView)
      removeSheet(document.defaultView, hideSheetUri, 'user');
  }
  catch(e) {
    console.exception(e);
  }
}

function onContentWindow(event) {
  let document = event.subject;

  
  if (!(document instanceof Ci.nsIDOMHTMLDocument))
    return;

  
  
  if (!document.location)
    return;

  
  if (document.location.href.indexOf(assetsURI) !== 0)
    return;

  try {
    
    
    loadSheet(document.defaultView, hideSheetUri, 'user');
  }
  catch(e) {
    console.exception(e);
  }
  
  document.addEventListener("DOMContentLoaded", onDocumentReady2Translate,
                            false);
}



const ON_CONTENT = "document-element-inserted";
let enabled = false;
function enable() {
  if (!enabled) {
    events.on(ON_CONTENT, onContentWindow);
    enabled = true;
  }
}
exports.enable = enable;

function disable() {
  if (enabled) {
    events.off(ON_CONTENT, onContentWindow);
    enabled = false;
  }
}
exports.disable = disable;

require("sdk/system/unload").when(disable);
