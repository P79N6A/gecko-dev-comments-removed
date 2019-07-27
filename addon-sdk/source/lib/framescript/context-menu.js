


"use strict";

const { query, constant, cache } = require("sdk/lang/functional");
const { pairs, each, map, object } = require("sdk/util/sequence");
const { nodeToMessageManager } = require("./util");




const Try = (fn, fallback=null) => (...args) => {
  try {
    return fn(...args);
  } catch(error) {
    console.error(error);
    return fallback;
  }
};



const JSONReturn = f => (...args) => JSON.parse(JSON.stringify(f(...args)));

const Null = constant(null);


const readers = Object.create(null);



const read = node =>
  object(...map(([id, read]) => [id, read(node, id)], pairs(readers)));



const parsers = Object.create(null)



const parse = descriptor => {
  const parser = parsers[descriptor.category];
  if (!parser) {
    console.error("Unknown reader descriptor was received", descriptor, `"${descriptor.category}"`);
    return Null
  }
  return Try(parser(descriptor));
}


const HTML_NS = "http://www.w3.org/1999/xhtml";
const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
const SVG_NS = "http://www.w3.org/2000/svg";





const isVideoLoadingAudio = node =>
  node.readyState >= node.HAVE_METADATA &&
    (node.videoWidth == 0 || node.videoHeight == 0)

const isVideo = node =>
  node instanceof node.ownerDocument.defaultView.HTMLVideoElement &&
  !isVideoLoadingAudio(node);

const isAudio = node => {
  const {HTMLVideoElement, HTMLAudioElement} = node.ownerDocument.defaultView;
  return node instanceof HTMLAudioElement ? true :
         node instanceof HTMLVideoElement ? isVideoLoadingAudio(node) :
         false;
};

const isImage = ({namespaceURI, localName}) =>
  namespaceURI === HTML_NS && localName === "img" ? true :
  namespaceURI === XUL_NS && localName === "image" ? true :
  namespaceURI === SVG_NS && localName === "image" ? true :
  false;

parsers["reader/MediaType()"] = constant(node =>
  isImage(node) ? "image" :
  isAudio(node) ? "audio" :
  isVideo(node) ? "video" :
  null);


const readLink = node =>
  node.namespaceURI === HTML_NS && node.localName === "a" ? node.href :
  readLink(node.parentNode);

parsers["reader/LinkURL()"] = constant(node =>
  node.matches("a, a *") ? readLink(node) : null);



parsers["reader/SelectorMatch()"] = ({selector}) =>
  node => node.matches(selector);




const getInputSelection = node => {
  try {
    if ("selectionStart" in node && "selectionEnd" in node) {
      const {selectionStart, selectionEnd} = node;
      return {selectionStart, selectionEnd}
    }
  }
  catch(_) {}

  return null;
}




parsers["reader/Selection()"] = constant(node => {
  const selection = node.ownerDocument.getSelection();
  if (!selection.isCollapsed) {
    return selection.toString();
  }
  
  
  
  
  else {
    const selection = getInputSelection(node);
    const isSelected = selection &&
                       Number.isInteger(selection.selectionStart) &&
                       Number.isInteger(selection.selectionEnd) &&
                       selection.selectionStart !== selection.selectionEnd;
    return  isSelected ? node.value.substring(selection.selectionStart,
                                              selection.selectionEnd) :
            null;
  }
});



parsers["reader/Query()"] = ({path}) => JSONReturn(query(path));

parsers["reader/Attribute()"] = ({name}) => node => node.getAttribute(name);






parsers["reader/Extractor()"] = ({source}) =>
  JSONReturn(new Function("return (" + source + ")")());





const nonPageElements = ["a", "applet", "area", "button", "canvas", "object",
                         "embed", "img", "input", "map", "video", "audio", "menu",
                         "option", "select", "textarea", "[contenteditable=true]"];
const nonPageSelector = nonPageElements.
                          concat(nonPageElements.map(tag => `${tag} *`)).
                          join(", ");




parsers["reader/isPage()"] = constant(node =>
  node.ownerDocument.defaultView.getSelection().isCollapsed &&
  !node.matches(nonPageSelector));


parsers["reader/isFrame()"] = constant(node =>
  !!node.ownerDocument.defaultView.frameElement);

parsers["reader/isEditable()"] = constant(node => {
  const selection = getInputSelection(node);
  return selection ? !node.readOnly && !node.disabled : node.isContentEditable;
});




const onReadersUpdate = message => {
  each(([id, descriptor]) => {
    if (descriptor) {
      readers[id] = parse(descriptor);
    }
    else {
      delete readers[id];
    }
  }, pairs(message.data));
};
exports.onReadersUpdate = onReadersUpdate;


const onContextMenu = event => {
  if (!event.defaultPrevented) {
    const manager = nodeToMessageManager(event.target);
    manager.sendSyncMessage("sdk/context-menu/read", read(event.target), readers);
  }
};
exports.onContextMenu = onContextMenu;


const onContentFrame = (frame) => {
  
  frame.addEventListener("contextmenu", onContextMenu);
  
  frame.addMessageListener("sdk/context-menu/readers", onReadersUpdate);

  
  
  frame.sendAsyncMessage("sdk/context-menu/readers?");
};
exports.onContentFrame = onContentFrame;
