


"use strict";




const TEST_URI = "data:text/html;charset=utf-8," +
  "<object><p>browser_inspector_inspect-object-element.js</p></object>";

add_task(function* () {
  let { inspector } = yield openInspectorForURL(TEST_URI);
  let objectNode = getNode("object");
  ok(objectNode, "We have the object node");

  yield selectNode("object", inspector);
});
