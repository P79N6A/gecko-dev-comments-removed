






function runTests() {
  
  setLinks("0,1,2,3,4,5,6,7,8");
  setPinnedLinks("");

  yield addNewTabPageTab();
  let gridNode = getGrid().node;

  ok(!gridNode.hasAttribute("page-disabled"), "page is not disabled");

  NewTabUtils.allPages.enabled = false;
  ok(gridNode.hasAttribute("page-disabled"), "page is disabled");

  let oldGridNode = gridNode;

  
  
  yield addNewTabPageTab();
  ok(gridNode.hasAttribute("page-disabled"), "page is disabled");

  
  is(0, getContentDocument().querySelectorAll(".site").length, "no sites have been rendered");

  NewTabUtils.allPages.enabled = true;
  ok(!gridNode.hasAttribute("page-disabled"), "page is not disabled");
  ok(!oldGridNode.hasAttribute("page-disabled"), "old page is not disabled");
}
