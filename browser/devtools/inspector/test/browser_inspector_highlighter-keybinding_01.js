



"use strict";



const TEST_URL = TEST_URL_ROOT + "doc_inspector_highlighter_dom.html";

add_task(function*() {
  let {inspector, toolbox} = yield openInspectorForURL(TEST_URL);

  info("Starting element picker");
  yield toolbox.highlighterUtils.startPicker();

  info("Selecting the simple-div1 DIV");
  yield moveMouseOver("#simple-div1");

  let highlightedNode = yield getHighlitNode(toolbox);
  is(highlightedNode.id, "simple-div1", "The highlighter shows #simple-div1. OK.");

  
  info("Testing first-child selection.");

  yield doKeyHover({key: "VK_RIGHT", options: {}});
  highlightedNode = yield getHighlitNode(toolbox);
  is(highlightedNode.id, "useless-para", "The highlighter shows #useless-para. OK.");

  info("Selecting the useful-para paragraph DIV");
  yield moveMouseOver("#useful-para");
  highlightedNode = yield getHighlitNode(toolbox);
  is(highlightedNode.id, "useful-para", "The highlighter shows #useful-para. OK.");

  yield doKeyHover({key: "VK_RIGHT", options: {}});
  highlightedNode = yield getHighlitNode(toolbox);
  is(highlightedNode.id, "bold", "The highlighter shows #bold. OK.");

  info("Going back up to the simple-div1 DIV");
  yield doKeyHover({key: "VK_LEFT", options: {}});
  yield doKeyHover({key: "VK_LEFT", options: {}});
  highlightedNode = yield getHighlitNode(toolbox);
  is(highlightedNode.id, "simple-div1", "The highlighter shows #simple-div1. OK.");

  info("First child selection test Passed.");

  info("Stopping the picker");
  yield toolbox.highlighterUtils.stopPicker();

  function doKeyHover(msg) {
    info("Key pressed. Waiting for element to be highlighted/hovered");
    executeInContent("Test:SynthesizeKey", msg);
    return inspector.toolbox.once("picker-node-hovered");
  }

  function moveMouseOver(selector) {
    info("Waiting for element " + selector + " to be highlighted");
    executeInContent("Test:SynthesizeMouse", {
      options: {type: "mousemove"},
      center: true,
      selector: selector
    }, null, false);
    return inspector.toolbox.once("picker-node-hovered");
  }

});
