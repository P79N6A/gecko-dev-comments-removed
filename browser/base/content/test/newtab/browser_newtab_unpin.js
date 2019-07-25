






function runTests() {
  
  
  setLinks("0,1,2,3,4,5,6,7,8");
  setPinnedLinks(",1");

  yield addNewTabPageTab();
  checkGrid("0,1p,2,3,4,5,6,7,8");

  yield unpinCell(cells[1]);
  checkGrid("0,1,2,3,4,5,6,7,8");

  
  
  
  setLinks("0,1,2,3,4,5,6,7,8");
  setPinnedLinks(",99");

  yield addNewTabPageTab();
  checkGrid("0,99p,1,2,3,4,5,6,7");

  yield unpinCell(cells[1]);
  checkGrid("0,1,2,3,4,5,6,7,8");

  
  
  setLinks("0,1,2,3,4,5,6,7");
  setPinnedLinks(",1,,,,,,,0");

  yield addNewTabPageTab();
  checkGrid("2,1p,3,4,5,6,7,,0p");

  yield unpinCell(cells[1]);
  checkGrid("1,2,3,4,5,6,7,,0p");

  yield unpinCell(cells[8]);
  checkGrid("0,1,2,3,4,5,6,7,");

  
  
  setLinks("0,1,2,3,4,5,6,7,8,9");
  setPinnedLinks("9");

  yield addNewTabPageTab();
  checkGrid("9p,0,1,2,3,4,5,6,7");

  yield unpinCell(cells[0]);
  checkGrid("0,1,2,3,4,5,6,7,8");
}
