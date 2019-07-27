



"use strict";



const TEST_URL = TEST_URL_ROOT + "doc_markup_anonymous.html";

add_task(function*() {
  Services.prefs.setBoolPref("devtools.inspector.showAllAnonymousContent", true);

  let {inspector} = yield addTab(TEST_URL).then(openInspector);

  let native = yield getNodeFront("#native", inspector);

  
  let nativeChildren = yield inspector.walker.children(native);
  is (nativeChildren.nodes.length, 1, "Children returned from walker");

  info ("Checking the video element");
  let video = nativeChildren.nodes[0];
  ok (!video.isAnonymous, "<video> is not anonymous");

  let videoChildren = yield inspector.walker.children(video);
  is (videoChildren.nodes.length, 3, "<video> has native anonymous children");

  for (let node of videoChildren.nodes) {
    ok (node.isAnonymous, "Child is anonymous");
    ok (!node._form.isXBLAnonymous, "Child is not XBL anonymous");
    ok (!node._form.isShadowAnonymous, "Child is not shadow anonymous");
    ok (node._form.isNativeAnonymous, "Child is native anonymous");
    yield isEditingMenuDisabled(node, inspector);
  }
});
