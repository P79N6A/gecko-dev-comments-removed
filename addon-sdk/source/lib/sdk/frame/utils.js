



'use strict';

module.metadata = {
  "stability": "experimental"
};

const { Ci } = require("chrome");
const XUL = 'http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul';

function eventTarget(frame) {
  return getDocShell(frame).chromeEventHandler;
}
exports.eventTarget = eventTarget;

function getDocShell(frame) {
  let { frameLoader } = frame.QueryInterface(Ci.nsIFrameLoaderOwner);
  return frameLoader && frameLoader.docShell;
}
exports.getDocShell = getDocShell;



















function create(document, options) {
  options = options || {};
  let remote = options.remote || false;
  let nodeName = options.nodeName || 'browser';
  let namespaceURI = options.namespaceURI || XUL;

  let frame = document.createElementNS(namespaceURI, nodeName);
  
  
  frame.setAttribute('type', options.type || 'content');
  frame.setAttribute('src', options.uri || 'about:blank');

  
  
  if (remote) {
    
    
    
    frame.setAttribute('style', '-moz-binding: none;');
    frame.setAttribute('remote', 'true');
  }

  document.documentElement.appendChild(frame);

  
  if (!remote) {
    let docShell = getDocShell(frame);
    docShell.allowAuth = options.allowAuth || false;
    docShell.allowJavascript = options.allowJavascript || false;
    docShell.allowPlugins = options.allowPlugins || false;
  }

  return frame;
}
exports.create = create;
