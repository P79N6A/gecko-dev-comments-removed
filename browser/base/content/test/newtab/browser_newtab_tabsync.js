








function runTests() {
  
  return;

  yield setLinks("0,1,2,3,4,5,6,7,8,9");
  setPinnedLinks(",1");

  yield addNewTabPageTab();
  checkGrid("0,1p,2,3,4,5,6,7,8");

  let resetButton = getContentDocument().getElementById("toolbar-button-reset");
  ok(!resetButton.hasAttribute("modified"), "page is not modified");

  let oldSites = getGrid().sites;
  let oldResetButton = resetButton;

  
  yield addNewTabPageTab();
  checkGrid("0,1p,2,3,4,5,6,7,8");

  resetButton = getContentDocument().getElementById("toolbar-button-reset");
  ok(!resetButton.hasAttribute("modified"), "page is not modified");

  
  yield unpinCell(1);
  checkGrid("0,1,2,3,4,5,6,7,8");
  checkGrid("0,1,2,3,4,5,6,7,8", oldSites);

  
  yield blockCell(1);
  checkGrid("0,2,3,4,5,6,7,8,9");
  checkGrid("0,2,3,4,5,6,7,8,9", oldSites);
  ok(resetButton.hasAttribute("modified"), "page is modified");
  ok(oldResetButton.hasAttribute("modified"), "page is modified");

  
  yield simulateExternalDrop(1);
  checkGrid("0,99p,2,3,4,5,6,7,8");
  checkGrid("0,99p,2,3,4,5,6,7,8", oldSites);

  
  yield simulateDrop(2, 1);
  checkGrid("0,2p,99p,3,4,5,6,7,8");
  checkGrid("0,2p,99p,3,4,5,6,7,8", oldSites);

  
  yield getContentWindow().gToolbar.reset(TestRunner.next);
  checkGrid("0,1,2,3,4,5,6,7,8");
  checkGrid("0,1,2,3,4,5,6,7,8", oldSites);
  ok(!resetButton.hasAttribute("modified"), "page is not modified");
  ok(!oldResetButton.hasAttribute("modified"), "page is not modified");
}
