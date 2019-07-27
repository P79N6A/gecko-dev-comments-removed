


"use strict";




const TEST_URI = "data:text/html;charset=utf-8," +
  "<object><p>browser_inspector_inspect-object-element.js</p></object>";

add_task(function* () {
  let { inspector } = yield openInspectorForURL(TEST_URI);

  yield selectNode("object", inspector);

  ok(true, "Selected <object> without throwing");
});
