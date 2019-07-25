








function runTests() {
  
  setLinks("0,1,2,3,4,5,6,7,8");
  setPinnedLinks("");

  yield addNewTabPageTab();
  checkGrid("0,1,2,3,4,5,6,7,8");

  yield simulateDrop(cells[1], cells[0]);
  checkGrid("1,0p,2,3,4,5,6,7,8");

  
  setLinks("0,1,2,3,4,5,6,7,8");
  setPinnedLinks("");

  yield addNewTabPageTab();
  checkGrid("0,1,2,3,4,5,6,7,8");

  yield simulateDrop(cells[0], cells[0]);
  checkGrid("0,1,2,3,4,5,6,7,8");

  
  setLinks("0,1,2,3,4,5,6,7,8");
  setPinnedLinks(",1,2");

  yield addNewTabPageTab();
  checkGrid("0,1p,2p,3,4,5,6,7,8");

  yield simulateDrop(cells[3], cells[0]);
  checkGrid("3,1p,2p,0p,4,5,6,7,8");

  
  
  setLinks("0,1,2,3,4,5,6,7,8");
  setPinnedLinks("0,1");

  yield addNewTabPageTab();
  checkGrid("0p,1p,2,3,4,5,6,7,8");

  yield simulateDrop(cells[0], cells[2]);
  checkGrid("2p,0p,1p,3,4,5,6,7,8");

  
  
  setLinks("0,1,2,3,4,5,6,7,8");
  setPinnedLinks(",,,,,,,7,8");

  yield addNewTabPageTab();
  checkGrid("0,1,2,3,4,5,6,7p,8p");

  yield simulateDrop(cells[8], cells[2]);
  checkGrid("0,1,3,4,5,6,7p,8p,2p");

  
  setLinks("0,1,2,3,4,5,6,7,8");
  setPinnedLinks("0,1,2,,,5");

  yield addNewTabPageTab();
  checkGrid("0p,1p,2p,3,4,5p,6,7,8");

  yield simulateDrop(cells[4], cells[0]);
  checkGrid("3,1p,2p,4,0p,5p,6,7,8");

  
  setLinks("0,1,2,3,4,5,6,7,8");
  setPinnedLinks(",,,,,,,7,8");

  yield addNewTabPageTab();
  checkGrid("0,1,2,3,4,5,6,7p,8p");

  yield simulateDrop(cells[0]);
  checkGrid("99p,0,1,2,3,4,5,7p,8p");

  
  
  setLinks("0,1,2,3,4,5,6,7,8");
  setPinnedLinks(",,,,,,,7,8");

  yield addNewTabPageTab();
  checkGrid("0,1,2,3,4,5,6,7p,8p");

  yield simulateDrop(cells[7]);
  checkGrid("0,1,2,3,4,5,7p,99p,8p");

  
  
  setLinks("0,1,2,3,4,5,6,7,8");
  setPinnedLinks(",,,,,,,,8");

  yield addNewTabPageTab();
  checkGrid("0,1,2,3,4,5,6,7,8p");

  yield simulateDrop(cells[7]);
  checkGrid("0,1,2,3,4,5,6,99p,8p");

  
  
  setLinks("0,1,2,3,4,5,6,7,8");
  setPinnedLinks("0,1,2,,,,,,");

  yield addNewTabPageTab();
  checkGrid("0p,1p,2p");

  yield simulateDrop(cells[1]);
  checkGrid("0p,99p,1p,2p,3,4,5,6,7");
}
