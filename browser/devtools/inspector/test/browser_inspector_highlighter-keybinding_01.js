



"use strict";



const TEST_URL = TEST_URL_ROOT + "doc_inspector_highlighter_dom.html";

add_task(function*() {
  let {inspector, toolbox, testActor} = yield openInspectorForURL(TEST_URL);

  info("Starting element picker");
  yield toolbox.highlighterUtils.startPicker();

  info("Selecting the simple-div1 DIV");
  yield moveMouseOver("#simple-div1");

  ok((yield testActor.assertHighlightedNode("#simple-div1")), "The highlighter shows #simple-div1. OK.");

  
  info("Testing first-child selection.");

  yield doKeyHover({key: "VK_RIGHT", options: {}});
  ok((yield testActor.assertHighlightedNode("#useless-para")), "The highlighter shows #useless-para. OK.");

  info("Selecting the useful-para paragraph DIV");
  yield moveMouseOver("#useful-para");
  ok((yield testActor.assertHighlightedNode("#useful-para")), "The highlighter shows #useful-para. OK.");

  yield doKeyHover({key: "VK_RIGHT", options: {}});
  ok((yield testActor.assertHighlightedNode("#bold")), "The highlighter shows #bold. OK.");

  info("Going back up to the simple-div1 DIV");
  yield doKeyHover({key: "VK_LEFT", options: {}});
  yield doKeyHover({key: "VK_LEFT", options: {}});
  ok((yield testActor.assertHighlightedNode("#simple-div1")), "The highlighter shows #simple-div1. OK.");

  info("First child selection test Passed.");

  info("Stopping the picker");
  yield toolbox.highlighterUtils.stopPicker();

  function doKeyHover(args) {
    info("Key pressed. Waiting for element to be highlighted/hovered");
    testActor.synthesizeKey(args);
    return inspector.toolbox.once("picker-node-hovered");
  }

  function moveMouseOver(selector) {
    info("Waiting for element " + selector + " to be highlighted");
    testActor.synthesizeMouse({
      options: {type: "mousemove"},
      center: true,
      selector: selector
    });
    return inspector.toolbox.once("picker-node-hovered");
  }

});
