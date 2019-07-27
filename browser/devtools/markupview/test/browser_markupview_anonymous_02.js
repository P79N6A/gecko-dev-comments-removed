



"use strict";


const TEST_URL = "chrome://browser/content/devtools/scratchpad.xul";

let test = asyncTest(function*() {
  let {inspector} = yield addTab(TEST_URL).then(openInspector);

  let toolbarbutton = yield getNodeFront("toolbarbutton", inspector);
  let children = yield inspector.walker.children(toolbarbutton);

  is(toolbarbutton.numChildren, 3, "Correct number of children");
  is (children.nodes.length, 3, "Children returned from walker");

  is(toolbarbutton.isAnonymous, false, "Toolbarbutton is not anonymous");
  yield isEditingMenuEnabled(toolbarbutton, inspector);

  for (let node of children.nodes) {
    ok (node.isAnonymous, "Child is anonymous");
    ok (node._form.isXBLAnonymous, "Child is XBL anonymous");
    ok (!node._form.isShadowAnonymous, "Child is not shadow anonymous");
    ok (!node._form.isNativeAnonymous, "Child is not native anonymous");
    yield isEditingMenuDisabled(node, inspector);
  }
});
