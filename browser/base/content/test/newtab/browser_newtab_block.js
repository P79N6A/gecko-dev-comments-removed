







function runTests() {
  
  
  setLinks("0,1,2,3,4,5,6,7,8,9");
  setPinnedLinks("");

  yield addNewTabPageTab();
  checkGrid("0,1,2,3,4,5,6,7,8");

  yield blockCell(cells[4]);
  checkGrid("0,1,2,3,5,6,7,8,9");

  yield blockCell(cells[4]);
  checkGrid("0,1,2,3,6,7,8,9,");

  yield blockCell(cells[4]);
  checkGrid("0,1,2,3,7,8,9,,");

  
  reset();
  setLinks("0,1,2,3,4,5,6,7,8");
  setPinnedLinks(",1");

  yield addNewTabPageTab();
  checkGrid("0,1p,2,3,4,5,6,7,8");

  yield blockCell(cells[1]);
  checkGrid("0,2,3,4,5,6,7,8,");

  
  
  reset();
  setLinks("0,1,2,3,4,5,6,7,8,9");
  setPinnedLinks(",,,,,,,,8");

  yield addNewTabPageTab();
  checkGrid("0,1,2,3,4,5,6,7,8p");

  yield blockCell(cells[8]);
  checkGrid("0,1,2,3,4,5,6,7,9");

  
  
  reset();
  setLinks("0,1,2,3,4,5,6,7,8,9");
  setPinnedLinks(",,,,,,,,8");

  yield addNewTabPageTab();
  checkGrid("0,1,2,3,4,5,6,7,8p");

  yield blockCell(cells[0]);
  checkGrid("1,2,3,4,5,6,7,9,8p");
}
