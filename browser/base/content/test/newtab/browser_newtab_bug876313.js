






function runTests() {
  
  yield setLinks("0,1,2,3,4,5,6,7,8");
  setPinnedLinks(",99");

  yield addNewTabPageTab();
  checkGrid("0,99p,1,2,3,4,5,6,7");

  
  yield unpinCell(1);
  checkGrid("0,1,2,3,4,5,6,7,8");

  
  NewTabUtils.pinnedLinks.resetCache();
  NewTabUtils.allPages.update();
  checkGrid("0,1,2,3,4,5,6,7,8");
}
