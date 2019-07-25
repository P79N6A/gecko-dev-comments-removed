






function runTests() {
  
  setLinks("0,1,2,3,4,5,6,7,8");
  setPinnedLinks("");

  yield addNewTabPageTab();
  let gridNode = cw.gGrid.node;

  ok(!gridNode.hasAttribute("page-disabled"), "page is not disabled");

  cw.gToolbar.hide();
  ok(gridNode.hasAttribute("page-disabled"), "page is disabled");

  let oldGridNode = cw.gGrid.node;

  
  
  yield addNewTabPageTab();
  ok(gridNode.hasAttribute("page-disabled"), "page is disabled");

  
  is(0, cw.document.querySelectorAll(".site").length, "no sites have been rendered");

  cw.gToolbar.show();
  ok(!gridNode.hasAttribute("page-disabled"), "page is not disabled");
  ok(!oldGridNode.hasAttribute("page-disabled"), "old page is not disabled");
}
