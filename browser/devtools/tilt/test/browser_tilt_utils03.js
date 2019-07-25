

"use strict";

function test() {
  let dom = TiltUtils.DOM;

  is(dom.parentNode, null,
    "The parent node should not be initially set.");

  dom.parentNode = {};
  ok(dom.parentNode,
    "The parent node should now be set.");

  TiltUtils.clearCache();
  is(dom.parentNode, null,
    "The parent node should not be set after clearing the cache.");
}
