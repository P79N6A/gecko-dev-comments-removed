



"use strict";









let {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;
let {LayoutHelpers} = Cu.import("resource://gre/modules/devtools/LayoutHelpers.jsm", {});
let DOMUtils = Cc["@mozilla.org/inspector/dom-utils;1"].getService(Ci.inIDOMUtils);
let loader = Cc["@mozilla.org/moz/jssubscript-loader;1"]
            .getService(Ci.mozIJSSubScriptLoader);
let EventUtils = {};
loader.loadSubScript("chrome://marionette/content/EventUtils.js", EventUtils);






addEventListener("DOMWindowCreated", () => {
  content.addEventListener("test-page-processing-done", () => {
    sendAsyncMessage("Test:TestPageProcessingDone");
  }, false);
});







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
  let {selector} = msg.data;
  let node = superQuerySelector(selector);

  let regions = {};

  let helper = new LayoutHelpers(content);
  for (let boxType of ["content", "padding", "border", "margin"]) {
    regions[boxType] = helper.getAdjustedQuads(node, boxType);
  }

  sendAsyncMessage("Test:GetAllAdjustedQuads", regions);
});
















addMessageListener("Test:SynthesizeMouse", function(msg) {
  let {x, y, center, options, selector} = msg.data;
  let {node} = msg.objects;

  if (!node && selector) {
    node = superQuerySelector(selector);
  }

  if (center) {
    EventUtils.synthesizeMouseAtCenter(node, options, node.ownerDocument.defaultView);
  } else {
    EventUtils.synthesizeMouse(node, x, y, options, node.ownerDocument.defaultView);
  }

  
  
  
  sendAsyncMessage("Test:SynthesizeMouse");
});









addMessageListener("Test:SynthesizeKey", function(msg) {
  let {key, options} = msg.data;

  EventUtils.synthesizeKey(key, options, content);
});









addMessageListener("Test:HasPseudoClassLock", function(msg) {
  let {node} = msg.objects;
  let {pseudo} = msg.data
  sendAsyncMessage("Test:HasPseudoClassLock", DOMUtils.hasPseudoClassLock(node, pseudo));
});













addMessageListener("Test:ScrollWindow", function(msg) {
  let {x, y, relative} = msg.data;

  if (isNaN(x) || isNaN(y)) {
    sendAsyncMessage("Test:ScrollWindow", {});
    return;
  }

  content.addEventListener("scroll", function onScroll(event) {
    this.removeEventListener("scroll", onScroll);

    let data = {x: content.scrollX, y: content.scrollY};
    sendAsyncMessage("Test:ScrollWindow", data);
  });

  content[relative ? "scrollBy" : "scrollTo"](x, y);
});











function superQuerySelector(superSelector, root=content.document) {
  let frameIndex = superSelector.indexOf("||");
  if (frameIndex === -1) {
    return root.querySelector(superSelector);
  } else {
    let rootSelector = superSelector.substring(0, frameIndex).trim();
    let childSelector = superSelector.substring(frameIndex+2).trim();
    root = root.querySelector(rootSelector);
    if (!root || !root.contentWindow) {
      return null;
    }

    return superQuerySelector(childSelector, root.contentWindow.document);
  }
}

let dumpn = msg => dump(msg + "\n");
