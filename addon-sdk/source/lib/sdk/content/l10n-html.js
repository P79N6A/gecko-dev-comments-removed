


"use strict";

module.metadata = {
  "stability": "unstable"
};

const { Ci } = require("chrome");
const core = require("../l10n/core");
const { loadSheet, removeSheet } = require("../stylesheet/utils");
const { process, frames } = require("../remote/child");

const assetsURI = require('../self').data.url();

const hideSheetUri = "data:text/css,:root {visibility: hidden !important;}";

function translateElementAttributes(element) {
  
  const attrList = ['title', 'accesskey', 'alt', 'label', 'placeholder'];
  const ariaAttrMap = {
          'ariaLabel': 'aria-label',
          'ariaValueText': 'aria-valuetext',
          'ariaMozHint': 'aria-moz-hint'
        };
  const attrSeparator = '.';
  
  
  for (let attribute of attrList) {
    const data = core.get(element.dataset.l10nId + attrSeparator + attribute);
    if (data)
      element.setAttribute(attribute, data);
  }
  
  
  for (let attrAlias in ariaAttrMap) {
    const data = core.get(element.dataset.l10nId + attrSeparator + attrAlias);
    if (data)
      element.setAttribute(ariaAttrMap[attrAlias], data);
  }
}



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

    translateElementAttributes(child);
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

function onContentWindow({ target: document }) {
  
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



const ON_CONTENT = "DOMDocElementInserted";
function enable() {
  frames.addEventListener(ON_CONTENT, onContentWindow, true);
}
process.port.on("sdk/l10n/html/enable", enable);

function disable() {
  frames.removeEventListener(ON_CONTENT, onContentWindow, true);
}
process.port.on("sdk/l10n/html/disable", disable);
