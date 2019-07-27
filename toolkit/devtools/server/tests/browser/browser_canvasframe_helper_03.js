



"use strict";





require("devtools/server/actors/inspector");
const {
  CanvasFrameAnonymousContentHelper,
  HighlighterEnvironment
} = require("devtools/server/actors/highlighter");
const TEST_URL = "data:text/html;charset=utf-8,CanvasFrameAnonymousContentHelper test";

add_task(function*() {
  let doc = yield addTab(TEST_URL);

  let nodeBuilder = () => {
    let root = doc.createElement("div");
    let child = doc.createElement("div");
    child.style = "pointer-events:auto;width:200px;height:200px;background:red;";
    child.id = "child-element";
    child.className = "child-element";
    root.appendChild(child);
    return root;
  };

  info("Building the helper");
  let env = new HighlighterEnvironment();
  env.initFromWindow(doc.defaultView);
  let helper = new CanvasFrameAnonymousContentHelper(env, nodeBuilder);

  let el = helper.getElement("child-element");

  info("Adding an event listener on the inserted element");
  let mouseDownHandled = 0;
  function onMouseDown(e, id) {
    is(id, "child-element", "The mousedown event was triggered on the element");
    ok(!e.originalTarget, "The originalTarget property isn't available");
    mouseDownHandled++;
  }
  el.addEventListener("mousedown", onMouseDown);

  info("Synthesizing an event on the inserted element");
  let onDocMouseDown = once(doc, "mousedown");
  synthesizeMouseDown(100, 100, doc.defaultView);
  yield onDocMouseDown;

  is(mouseDownHandled, 1, "The mousedown event was handled once on the element");

  info("Synthesizing an event somewhere else");
  onDocMouseDown = once(doc, "mousedown");
  synthesizeMouseDown(400, 400, doc.defaultView);
  yield onDocMouseDown;

  is(mouseDownHandled, 1, "The mousedown event was not handled on the element");

  info("Removing the event listener");
  el.removeEventListener("mousedown", onMouseDown);

  info("Synthesizing another event after the listener has been removed");
  
  onDocMouseDown = once(doc, "mousedown");
  synthesizeMouseDown(100, 100, doc.defaultView);
  yield onDocMouseDown;

  is(mouseDownHandled, 1,
    "The mousedown event hasn't been handled after the listener was removed");

  info("Adding again the event listener");
  el.addEventListener("mousedown", onMouseDown);

  info("Destroying the helper");
  env.destroy();
  helper.destroy();

  info("Synthesizing another event after the helper has been destroyed");
  
  onDocMouseDown = once(doc, "mousedown");
  synthesizeMouseDown(100, 100, doc.defaultView);
  yield onDocMouseDown;

  is(mouseDownHandled, 1,
    "The mousedown event hasn't been handled after the helper was destroyed");

  gBrowser.removeCurrentTab();
});

function synthesizeMouseDown(x, y, win) {
  
  
  
  let forceReflow = win.document.documentElement.offsetWidth;
  EventUtils.synthesizeMouseAtPoint(x, y, {type: "mousedown"}, win);
}
