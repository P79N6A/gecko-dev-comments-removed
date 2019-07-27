



"use strict";


const TEST_URL = TEST_URL_ROOT + "doc_markup_anonymous.html";

add_task(function*() {
  let {inspector} = yield addTab(TEST_URL).then(openInspector);

  let pseudo = yield getNodeFront("#pseudo", inspector);

  
  let children = yield inspector.walker.children(pseudo);
  is (children.nodes.length, 3, "Children returned from walker");

  info ("Checking the ::before pseudo element");
  let before = children.nodes[0];
  yield isEditingMenuDisabled(before, inspector);

  info ("Checking the normal child element");
  let span = children.nodes[1];
  yield isEditingMenuEnabled(span, inspector);

  info ("Checking the ::after pseudo element");
  let after = children.nodes[2];
  yield isEditingMenuDisabled(after, inspector);

  let native = yield getNodeFront("#native", inspector);

  
  let nativeChildren = yield inspector.walker.children(native);
  is (nativeChildren.nodes.length, 1, "Children returned from walker");

  info ("Checking the video element");
  let video = nativeChildren.nodes[0];
  ok (!video.isAnonymous, "<video> is not anonymous");

  let videoChildren = yield inspector.walker.children(video);
  is (videoChildren.nodes.length, 0,
    "No native children returned from walker for <video> by default");
});
