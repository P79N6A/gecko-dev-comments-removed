


"use strict";

const { Class } = require("../core/heritage");
const self = require("../self");
const { WorkerChild } = require("./worker-child");
const { getInnerId } = require("../window/utils");
const { Ci } = require("chrome");
const { Services } = require("resource://gre/modules/Services.jsm");



function getElementWithSelection(window) {
  let element = Services.focus.getFocusedElementForWindow(window, false, {});
  if (!element)
    return null;

  try {
    
    
    

    let { value, selectionStart, selectionEnd } = element;

    let hasSelection = typeof value === "string" &&
                      !isNaN(selectionStart) &&
                      !isNaN(selectionEnd) &&
                      selectionStart !== selectionEnd;

    return hasSelection ? element : null;
  }
  catch (err) {
    console.exception(err);
    return null;
  }
}

function safeGetRange(selection, rangeNumber) {
  try {
    let { rangeCount } = selection;
    let range = null;

    for (let rangeNumber = 0; rangeNumber < rangeCount; rangeNumber++ ) {
      range = selection.getRangeAt(rangeNumber);

      if (range && range.toString())
        break;

      range = null;
    }

    return range;
  }
  catch (e) {
    return null;
  }
}

function getSelection(window) {
  let selection = window.getSelection();
  let range = safeGetRange(selection);
  if (range)
    return range.toString();

  let node = getElementWithSelection(window);
  if (!node)
    return null;

  return node.value.substring(node.selectionStart, node.selectionEnd);
}




const NON_PAGE_CONTEXT_ELTS = [
  Ci.nsIDOMHTMLAnchorElement,
  Ci.nsIDOMHTMLAppletElement,
  Ci.nsIDOMHTMLAreaElement,
  Ci.nsIDOMHTMLButtonElement,
  Ci.nsIDOMHTMLCanvasElement,
  Ci.nsIDOMHTMLEmbedElement,
  Ci.nsIDOMHTMLImageElement,
  Ci.nsIDOMHTMLInputElement,
  Ci.nsIDOMHTMLMapElement,
  Ci.nsIDOMHTMLMediaElement,
  Ci.nsIDOMHTMLMenuElement,
  Ci.nsIDOMHTMLObjectElement,
  Ci.nsIDOMHTMLOptionElement,
  Ci.nsIDOMHTMLSelectElement,
  Ci.nsIDOMHTMLTextAreaElement,
];



let editableInputs = {
  email: true,
  number: true,
  password: true,
  search: true,
  tel: true,
  text: true,
  textarea: true,
  url: true
};

let CONTEXTS = {};

let Context = Class({
  initialize: function(id) {
    this.id = id;
  },

  adjustPopupNode: function adjustPopupNode(popupNode) {
    return popupNode;
  },

  
  
  getState: function(popupNode) {
    return false;
  }
});



CONTEXTS.PageContext = Class({
  extends: Context,

  getState: function(popupNode) {
    
    if (!popupNode.ownerDocument.defaultView.getSelection().isCollapsed)
      return false;

    
    
    while (!(popupNode instanceof Ci.nsIDOMDocument)) {
      if (NON_PAGE_CONTEXT_ELTS.some(function(type) popupNode instanceof type))
        return false;

      popupNode = popupNode.parentNode;
    }

    return true;
  }
});


CONTEXTS.SelectionContext = Class({
  extends: Context,

  getState: function(popupNode) {
    if (!popupNode.ownerDocument.defaultView.getSelection().isCollapsed)
      return true;

    try {
      
      
      let { selectionStart, selectionEnd } = popupNode;
      return !isNaN(selectionStart) && !isNaN(selectionEnd) &&
             selectionStart !== selectionEnd;
    }
    catch (e) {
      return false;
    }
  }
});



CONTEXTS.SelectorContext = Class({
  extends: Context,

  initialize: function initialize(id, selector) {
    Context.prototype.initialize.call(this, id);
    this.selector = selector;
  },

  adjustPopupNode: function adjustPopupNode(popupNode) {
    let selector = this.selector;

    while (!(popupNode instanceof Ci.nsIDOMDocument)) {
      if (popupNode.mozMatchesSelector(selector))
        return popupNode;

      popupNode = popupNode.parentNode;
    }

    return null;
  },

  getState: function(popupNode) {
    return !!this.adjustPopupNode(popupNode);
  }
});


CONTEXTS.URLContext = Class({
  extends: Context,

  getState: function(popupNode) {
    return popupNode.ownerDocument.URL;
  }
});


CONTEXTS.PredicateContext = Class({
  extends: Context,

  getState: function(node) {
    let window = node.ownerDocument.defaultView;
    let data = {};

    data.documentType = node.ownerDocument.contentType;

    data.documentURL = node.ownerDocument.location.href;
    data.targetName = node.nodeName.toLowerCase();
    data.targetID = node.id || null ;

    if ((data.targetName === 'input' && editableInputs[node.type]) ||
        data.targetName === 'textarea') {
      data.isEditable = !node.readOnly && !node.disabled;
    }
    else {
      data.isEditable = node.isContentEditable;
    }

    data.selectionText = getSelection(window, "TEXT");

    data.srcURL = node.src || null;
    data.value = node.value || null;

    while (!data.linkURL && node) {
      data.linkURL = node.href || null;
      node = node.parentNode;
    }

    return data;
  },
});

function instantiateContext({ id, type, args }) {
  if (!(type in CONTEXTS)) {
    console.error("Attempt to use unknown context " + type);
    return;
  }
  return new CONTEXTS[type](id, ...args);
}

let ContextWorker = Class({
  implements: [ WorkerChild ],

  
  
  
  
  getMatchedContext: function getCurrentContexts(popupNode) {
    let results = this.sandbox.emitSync("context", popupNode);
    if (!results.length)
      return true;
    return results.reduce((val, result) => val || result);
  },

  
  
  
  fireClick: function fireClick(popupNode, clickedItemData) {
    this.sandbox.emitSync("click", popupNode, clickedItemData);
  }
});




function getItemWorkerForWindow(item, window) {
  if (!item.contentScript && !item.contentScriptFile)
    return null;

  let id = getInnerId(window);
  let worker = item.workerMap.get(id);

  if (worker)
    return worker;

  worker = ContextWorker({
    id: item.id,
    window: id,
    manager: item.manager,
    contentScript: item.contentScript,
    contentScriptFile: item.contentScriptFile,
    onDetach: function() {
      item.workerMap.delete(id);
    }
  });

  item.workerMap.set(id, worker);

  return worker;
}




let RemoteItem = Class({
  initialize: function(options, manager) {
    this.id = options.id;
    this.contexts = [instantiateContext(c) for (c of options.contexts)];
    this.contentScript = options.contentScript;
    this.contentScriptFile = options.contentScriptFile;

    this.manager = manager;

    this.workerMap = new Map();
  },

  destroy: function() {
    for (let worker of this.workerMap.values()) {
      worker.destroy();
    }
  },

  activate: function(popupNode, data) {
    let worker = getItemWorkerForWindow(this, popupNode.ownerDocument.defaultView);
    if (!worker)
      return;

    for (let context of this.contexts)
      popupNode = context.adjustPopupNode(popupNode);

    worker.fireClick(popupNode, data);
  },

  
  getContextState: function(popupNode, addonInfo) {
    if (!(self.id in addonInfo))
      addonInfo[self.id] = {};

    let worker = getItemWorkerForWindow(this, popupNode.ownerDocument.defaultView);
    let contextStates = {};
    for (let context of this.contexts)
      contextStates[context.id] = context.getState(popupNode);

    addonInfo[self.id][this.id] = {
      
      
      pageContext: (new CONTEXTS.PageContext()).getState(popupNode),
      contextStates,
      hasWorker: !!worker,
      workerContext: worker ? worker.getMatchedContext(popupNode) : true
    }
  }
});
exports.RemoteItem = RemoteItem;
