








function runTests() {
  requestLongerTimeout(2);
  yield addNewTabPageTab();

  
  yield setLinks("0,1,2,3,4,5,6,7,8");
  setPinnedLinks("");

  yield addNewTabPageTab();
  checkGrid("0,1,2,3,4,5,6,7,8");

  yield simulateDrop(0, 1);
  checkGrid("1,0p,2,3,4,5,6,7,8");

  
  yield setLinks("0,1,2,3,4,5,6,7,8");
  setPinnedLinks("");

  yield addNewTabPageTab();
  checkGrid("0,1,2,3,4,5,6,7,8");

  yield simulateDrop(0, 0);
  checkGrid("0,1,2,3,4,5,6,7,8");

  
  yield setLinks("0,1,2,3,4,5,6,7,8");
  setPinnedLinks(",1,2");

  yield addNewTabPageTab();
  checkGrid("0,1p,2p,3,4,5,6,7,8");

  yield simulateDrop(0, 3);
  checkGrid("3,1p,2p,0p,4,5,6,7,8");

  
  
  yield setLinks("0,1,2,3,4,5,6,7,8");
  setPinnedLinks("0,1");

  yield addNewTabPageTab();
  checkGrid("0p,1p,2,3,4,5,6,7,8");

  yield simulateDrop(2, 0);
  checkGrid("2p,0p,1p,3,4,5,6,7,8");

  
  
  yield setLinks("0,1,2,3,4,5,6,7,8");
  setPinnedLinks(",,,,,,,7,8");

  yield addNewTabPageTab();
  checkGrid("0,1,2,3,4,5,6,7p,8p");

  yield simulateDrop(2, 5);
  checkGrid("0,1,3,4,5,2p,6,7p,8p");

  
  yield setLinks("0,1,2,3,4,5,6,7,8");
  setPinnedLinks("0,1,2,,,5");

  yield addNewTabPageTab();
  checkGrid("0p,1p,2p,3,4,5p,6,7,8");

  yield simulateDrop(0, 4);
  checkGrid("3,1p,2p,4,0p,5p,6,7,8");
}
