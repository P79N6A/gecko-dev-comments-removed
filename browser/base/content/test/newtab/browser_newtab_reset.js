





function runTests() {
  
  return;

  
  yield setLinks("0,1,2,3,4,5,6,7,8");
  setPinnedLinks("");

  yield addNewTabPageTab();
  let resetButton = getContentDocument().getElementById("toolbar-button-reset");

  checkGrid("0,1,2,3,4,5,6,7,8");
  ok(!resetButton.hasAttribute("modified"), "page is not modified");

  yield blockCell(4);
  checkGrid("0,1,2,3,5,6,7,8,");
  ok(resetButton.hasAttribute("modified"), "page is modified");

  yield getContentWindow().gToolbar.reset(TestRunner.next);
  checkGrid("0,1,2,3,4,5,6,7,8");
  ok(!resetButton.hasAttribute("modified"), "page is not modified");
}
