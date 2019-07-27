



"use strict";






require("devtools/server/actors/inspector");
const {
  CanvasFrameAnonymousContentHelper,
  HighlighterEnvironment
} = require("devtools/server/actors/highlighter");
const events = require("sdk/event/core");
const TEST_URL_1 = "data:text/html;charset=utf-8,CanvasFrameAnonymousContentHelper test 1";
const TEST_URL_2 = "data:text/html;charset=utf-8,CanvasFrameAnonymousContentHelper test 2";

add_task(function*() {
  let doc = yield addTab(TEST_URL_2);

  let nodeBuilder = () => {
    let root = doc.createElement("div");
    let child = doc.createElement("div");
    child.style = "pointer-events:auto;width:200px;height:200px;background:red;";
    child.id = "child-element";
    child.className = "child-element";
    child.textContent= "test content";
    root.appendChild(child);
    return root;
  };

  info("Building the helper");
  let env = new HighlighterEnvironment();
  env.initFromWindow(doc.defaultView);
  let helper = new CanvasFrameAnonymousContentHelper(env, nodeBuilder);

  info("Get an element from the helper");
  let el = helper.getElement("child-element");

  info("Try to access the element");
  is(el.getAttribute("class"), "child-element",
    "The attribute is correct before navigation");
  is(el.getTextContent(), "test content",
    "The text content is correct before navigation");

  info("Add an event listener on the element");
  let mouseDownHandled = 0;
  function onMouseDown(e, id) {
    is(id, "child-element", "The mousedown event was triggered on the element");
    mouseDownHandled ++;
  }
  el.addEventListener("mousedown", onMouseDown);

  info("Synthesizing an event on the element");
  let onDocMouseDown = once(doc, "mousedown");
  synthesizeMouseDown(100, 100, doc.defaultView);
  yield onDocMouseDown;
  is(mouseDownHandled, 1, "The mousedown event was handled once before navigation");

  info("Navigating to a new page");
  let loaded = once(gBrowser.selectedBrowser, "load", true);
  content.location = TEST_URL_2;
  yield loaded;
  doc = gBrowser.selectedBrowser.contentWindow.document;

  info("Try to access the element again");
  is(el.getAttribute("class"), "child-element",
    "The attribute is correct after navigation");
  is(el.getTextContent(), "test content",
    "The text content is correct after navigation");

  info("Synthesizing an event on the element again");
  onDocMouseDown = once(doc, "mousedown");
  synthesizeMouseDown(100, 100, doc.defaultView);
  yield onDocMouseDown;
  is(mouseDownHandled, 1, "The mousedown event was not handled after navigation");

  info("Destroying the helper");
  env.destroy();
  helper.destroy();

  gBrowser.removeCurrentTab();
});

function synthesizeMouseDown(x, y, win) {
  
  
  
  let forceReflow = win.document.documentElement.offsetWidth;
  EventUtils.synthesizeMouseAtPoint(x, y, {type: "mousedown"}, win);
}
