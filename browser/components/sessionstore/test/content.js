



let Cu = Components.utils;
let Ci = Components.interfaces;

Cu.import("resource:///modules/sessionstore/FrameTree.jsm", this);
let gFrameTree = new FrameTree(this);

gFrameTree.addObserver({
  onFrameTreeReset: function () {
    sendAsyncMessage("ss-test:onFrameTreeReset");
  },

  onFrameTreeCollected: function () {
    sendAsyncMessage("ss-test:onFrameTreeCollected");
  }
});






addEventListener("MozStorageChanged", function () {
  sendSyncMessage("ss-test:MozStorageChanged");
});

addMessageListener("ss-test:modifySessionStorage", function (msg) {
  for (let key of Object.keys(msg.data)) {
    content.sessionStorage[key] = msg.data[key];
  }
});

addMessageListener("ss-test:notifyObservers", function ({data: {topic, data}}) {
  Services.obs.notifyObservers(null, topic, data || "");
});

addMessageListener("ss-test:getStyleSheets", function (msg) {
  let sheets = content.document.styleSheets;
  let titles = Array.map(sheets, ss => [ss.title, ss.disabled]);
  sendSyncMessage("ss-test:getStyleSheets", titles);
});

addMessageListener("ss-test:enableStyleSheetsForSet", function (msg) {
  content.document.enableStyleSheetsForSet(msg.data);
  sendAsyncMessage("ss-test:enableStyleSheetsForSet");
});

addMessageListener("ss-test:enableSubDocumentStyleSheetsForSet", function (msg) {
  let iframe = content.document.getElementById(msg.data.id);
  iframe.contentDocument.enableStyleSheetsForSet(msg.data.set);
  sendAsyncMessage("ss-test:enableSubDocumentStyleSheetsForSet");
});

addMessageListener("ss-test:getAuthorStyleDisabled", function (msg) {
  let {authorStyleDisabled} =
    docShell.contentViewer.QueryInterface(Ci.nsIMarkupDocumentViewer);
  sendSyncMessage("ss-test:getAuthorStyleDisabled", authorStyleDisabled);
});

addMessageListener("ss-test:setAuthorStyleDisabled", function (msg) {
  let markupDocumentViewer =
    docShell.contentViewer.QueryInterface(Ci.nsIMarkupDocumentViewer);
  markupDocumentViewer.authorStyleDisabled = msg.data;
  sendSyncMessage("ss-test:setAuthorStyleDisabled");
});

addMessageListener("ss-test:setUsePrivateBrowsing", function (msg) {
  let loadContext =
    docShell.QueryInterface(Ci.nsILoadContext);
  loadContext.usePrivateBrowsing = msg.data;
  sendAsyncMessage("ss-test:setUsePrivateBrowsing");
});

addMessageListener("ss-test:getScrollPosition", function (msg) {
  let frame = content;
  if (msg.data.hasOwnProperty("frame")) {
    frame = content.frames[msg.data.frame];
  }
  let {scrollX: x, scrollY: y} = frame;
  sendAsyncMessage("ss-test:getScrollPosition", {x: x, y: y});
});

addMessageListener("ss-test:setScrollPosition", function (msg) {
  let frame = content;
  let {x, y} = msg.data;
  if (msg.data.hasOwnProperty("frame")) {
    frame = content.frames[msg.data.frame];
  }
  frame.scrollTo(x, y);

  frame.addEventListener("scroll", function onScroll(event) {
    if (frame.document == event.target) {
      frame.removeEventListener("scroll", onScroll);
      sendAsyncMessage("ss-test:setScrollPosition");
    }
  });
});

addMessageListener("ss-test:createDynamicFrames", function ({data}) {
  function createIFrame(rows) {
    let frames = content.document.getElementById(data.id);
    frames.setAttribute("rows", rows);

    let frame = content.document.createElement("frame");
    frame.setAttribute("src", data.url);
    frames.appendChild(frame);
  }

  addEventListener("DOMContentLoaded", function onContentLoaded(event) {
    if (content.document == event.target) {
      removeEventListener("DOMContentLoaded", onContentLoaded, true);
      
      createIFrame("33%, 33%, 33%");
    }
  }, true);

  addEventListener("load", function onLoad(event) {
    if (content.document == event.target) {
      removeEventListener("load", onLoad, true);

      
      
      createIFrame("25%, 25%, 25%, 25%");
    }
  }, true);

  sendAsyncMessage("ss-test:createDynamicFrames");
});

addMessageListener("ss-test:removeLastFrame", function ({data}) {
  let frames = content.document.getElementById(data.id);
  frames.lastElementChild.remove();
  sendAsyncMessage("ss-test:removeLastFrame");
});

addMessageListener("ss-test:mapFrameTree", function (msg) {
  let result = gFrameTree.map(frame => ({href: frame.location.href}));
  sendAsyncMessage("ss-test:mapFrameTree", result);
});
