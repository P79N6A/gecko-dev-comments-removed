








function runTests() {
  setLinks("0,1,2,3,4,5,6,7,8,9");
  setPinnedLinks(",1");

  yield addNewTabPageTab();
  checkGrid("0,1p,2,3,4,5,6,7,8");

  let resetButton = cw.document.getElementById("toolbar-button-reset");
  ok(!resetButton.hasAttribute("modified"), "page is not modified");

  let oldCw = cw;
  let oldResetButton = resetButton;

  
  yield addNewTabPageTab();
  checkGrid("0,1p,2,3,4,5,6,7,8");

  resetButton = cw.document.getElementById("toolbar-button-reset");
  ok(!resetButton.hasAttribute("modified"), "page is not modified");

  
  yield unpinCell(cells[1]);
  checkGrid("0,1,2,3,4,5,6,7,8");
  checkGrid("0,1,2,3,4,5,6,7,8", oldCw.gGrid.sites);

  
  yield blockCell(cells[1]);
  checkGrid("0,2,3,4,5,6,7,8,9");
  checkGrid("0,2,3,4,5,6,7,8,9", oldCw.gGrid.sites);
  ok(resetButton.hasAttribute("modified"), "page is modified");
  ok(oldResetButton.hasAttribute("modified"), "page is modified");

  
  yield simulateDrop(cells[1]);
  checkGrid("0,99p,2,3,4,5,6,7,8");
  checkGrid("0,99p,2,3,4,5,6,7,8", oldCw.gGrid.sites);

  
  yield simulateDrop(cells[1], cells[2]);
  checkGrid("0,2p,99p,3,4,5,6,7,8");
  checkGrid("0,2p,99p,3,4,5,6,7,8", oldCw.gGrid.sites);

  
  yield cw.gToolbar.reset(TestRunner.next);
  checkGrid("0,1,2,3,4,5,6,7,8");
  checkGrid("0,1,2,3,4,5,6,7,8", oldCw.gGrid.sites);
  ok(!resetButton.hasAttribute("modified"), "page is not modified");
  ok(!oldResetButton.hasAttribute("modified"), "page is not modified");
}
