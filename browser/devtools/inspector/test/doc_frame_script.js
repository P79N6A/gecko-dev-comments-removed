



"use strict";









let {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;
let {LayoutHelpers} = Cu.import("resource://gre/modules/devtools/LayoutHelpers.jsm", {});
let DOMUtils = Cc["@mozilla.org/inspector/dom-utils;1"].getService(Ci.inIDOMUtils);
let loader = Cc["@mozilla.org/moz/jssubscript-loader;1"]
            .getService(Ci.mozIJSSubScriptLoader);
let EventUtils = {};
loader.loadSubScript("chrome://marionette/content/EventUtils.js", EventUtils);







function getHighlighterActor(actorID, connPrefix) {
  let {DebuggerServer} = Cu.import("resource://gre/modules/devtools/dbg-server.jsm");
  if (!DebuggerServer.initialized) {
    return;
  }

  let conn = DebuggerServer._connections[connPrefix];
  if (!conn) {
    return;
  }

  return conn.getActor(actorID);
}










function getHighlighterCanvasFrameHelper(actorID, connPrefix) {
  let actor = getHighlighterActor(actorID, connPrefix);
  if (actor && actor._highlighter) {
    return actor._highlighter.markup;
  }
}











addMessageListener("Test:GetHighlighterAttribute", function(msg) {
  let {nodeID, name, actorID, connPrefix} = msg.data;

  let value;
  let helper = getHighlighterCanvasFrameHelper(actorID, connPrefix);
  if (helper) {
    value = helper.getAttributeForElement(nodeID, name);
  }

  sendAsyncMessage("Test:GetHighlighterAttribute", value);
});










addMessageListener("Test:GetHighlighterTextContent", function(msg) {
  let {nodeID, actorID, connPrefix} = msg.data;

  let value;
  let helper = getHighlighterCanvasFrameHelper(actorID, connPrefix);
  if (helper) {
    value = helper.getTextContentForElement(nodeID);
  }

  sendAsyncMessage("Test:GetHighlighterTextContent", value);
});









addMessageListener("Test:GetSelectorHighlighterBoxNb", function(msg) {
  let {actorID, connPrefix} = msg.data;
  let {_highlighter: h} = getHighlighterActor(actorID, connPrefix);
  if (!h || !h._highlighters) {
    sendAsyncMessage("Test:GetSelectorHighlighterBoxNb", null);
  } else {
    sendAsyncMessage("Test:GetSelectorHighlighterBoxNb", h._highlighters.length);
  }
});











addMessageListener("Test:ChangeHighlightedNodeWaitForUpdate", function(msg) {
  
  let {name, value, actorID, connPrefix} = msg.data;
  let {_highlighter: h} = getHighlighterActor(actorID, connPrefix);

  h.once("updated", () => {
    sendAsyncMessage("Test:ChangeHighlightedNodeWaitForUpdate");
  });

  h.currentNode.setAttribute(name, value);
});








addMessageListener("Test:WaitForHighlighterEvent", function(msg) {
  let {event, actorID, connPrefix} = msg.data;
  let {_highlighter: h} = getHighlighterActor(actorID, connPrefix);

  h.once(event, () => {
    sendAsyncMessage("Test:WaitForHighlighterEvent");
  });
});










addMessageListener("Test:ChangeZoomLevel", function(msg) {
  let {level, actorID, connPrefix} = msg.data;
  dumpn("Zooming page to " + level);

  if (actorID) {
    let {_highlighter: h} = getHighlighterActor(actorID, connPrefix);
    h.once("updated", () => {
      sendAsyncMessage("Test:ChangeZoomLevel");
    });
  }

  let docShell = content.QueryInterface(Ci.nsIInterfaceRequestor)
                        .getInterface(Ci.nsIWebNavigation)
                        .QueryInterface(Ci.nsIDocShell);
  docShell.contentViewer.fullZoom = level;

  if (!actorID) {
    sendAsyncMessage("Test:ChangeZoomLevel");
  }
});








addMessageListener("Test:ElementFromPoint", function(msg) {
  let {x, y} = msg.data;
  dumpn("Getting the element at " + x + "/" + y);

  let helper = new LayoutHelpers(content);
  let element = helper.getElementFromPoint(content.document, x, y);
  sendAsyncMessage("Test:ElementFromPoint", null, {element});
});







addMessageListener("Test:GetAllAdjustedQuads", function(msg) {
  let {node} = msg.objects;
  let regions = {};

  let helper = new LayoutHelpers(content);
  for (let boxType of ["content", "padding", "border", "margin"]) {
    regions[boxType] = helper.getAdjustedQuads(node, boxType);
  }

  sendAsyncMessage("Test:GetAllAdjustedQuads", regions);
});














addMessageListener("Test:SynthesizeMouse", function(msg) {
  let {node} = msg.objects;
  let {x, y, center, type} = msg.data;

  if (center) {
    EventUtils.synthesizeMouseAtCenter(node, {type}, node.ownerDocument.defaultView);
  } else {
    EventUtils.synthesizeMouse(node, x, y, {type}, node.ownerDocument.defaultView);
  }
});









addMessageListener("Test:HasPseudoClassLock", function(msg) {
  let {node} = msg.objects;
  let {pseudo} = msg.data
  sendAsyncMessage("Test:HasPseudoClassLock", DOMUtils.hasPseudoClassLock(node, pseudo));
});

let dumpn = msg => dump(msg + "\n");
