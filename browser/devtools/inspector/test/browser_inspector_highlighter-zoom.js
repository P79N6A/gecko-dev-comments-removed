



"use strict";




const TEST_URL = "data:text/html;charset=utf-8,<div>zoom me</div>";





const TEST_LEVELS = [{
  level: 2,
  expected: "position:absolute;transform-origin:top left;transform:scale(0.5);width:200%;height:200%;"
}, {
  level: 1,
  expected: "position:absolute;width:100%;height:100%;"
}, {
  level: .5,
  expected: "position:absolute;transform-origin:top left;transform:scale(2);width:50%;height:50%;"
}];

add_task(function*() {
  let {inspector, toolbox} = yield openInspectorForURL(TEST_URL);

  info("Highlighting the test node");

  yield hoverElement("div", inspector);
  let isVisible = yield isHighlighting(toolbox);
  ok(isVisible, "The highlighter is visible");

  for (let {level, expected} of TEST_LEVELS) {
    info("Zoom to level " + level + " and check that the highlighter is correct");

    let {actorID, connPrefix} = getHighlighterActorID(toolbox.highlighter);
    yield zoomPageTo(level, actorID, connPrefix);
    isVisible = yield isHighlighting(toolbox);
    ok(isVisible, "The highlighter is still visible at zoom level " + level);

    yield isNodeCorrectlyHighlighted("div", toolbox);

    info("Check that the highlighter root wrapper node was scaled down");

    let style = yield getRootNodeStyle(toolbox);
    is(style, expected, "The style attribute of the root element is correct");
  }
});

function* hoverElement(selector, inspector) {
  info("Hovering node " + selector + " in the markup view");
  let container = yield getContainerForSelector(selector, inspector);
  yield hoverContainer(container, inspector);
}

function* hoverContainer(container, inspector) {
  let onHighlight = inspector.toolbox.once("node-highlight");
  EventUtils.synthesizeMouse(container.tagLine, 2, 2, {type: "mousemove"},
      inspector.markup.doc.defaultView);
  yield onHighlight;
}

function* getRootNodeStyle(toolbox) {
  let value = yield getHighlighterNodeAttribute(toolbox.highlighter,
                                                "box-model-root", "style");
  return value;
}
