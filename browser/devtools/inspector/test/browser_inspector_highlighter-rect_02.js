



"use strict";




const TEST_URL = TEST_URL_ROOT + "doc_inspector_highlighter_rect.html";

add_task(function*() {
  let {inspector, testActor} = yield openInspectorForURL(TEST_URL);
  let front = inspector.inspector;
  let highlighter = yield front.getHighlighterByType("RectHighlighter");

  info("Showing the rect highlighter in the context of the iframe");

  
  let childBody = yield getNodeFrontInFrame("body", "iframe", inspector);
  yield highlighter.show(childBody, {
    rect: {x: 50, y: 50, width: 100, height: 100}
  });

  let style = yield testActor.getHighlighterNodeAttribute("highlighted-rect", "style", highlighter);

  
  
  
  
  is(style, "left:170px;top:170px;width:100px;height:100px;",
    "The highlighter is correctly positioned");

  yield highlighter.hide();
  yield highlighter.finalize();
});
