


const PREF_NEWTAB_COLUMNS = "browser.newtabpage.columns";









function runTests() {
  registerCleanupFunction(_ => Services.prefs.clearUserPref(PREF_NEWTAB_COLUMNS));

  
  yield setLinks("0,1,2,3,4,5,6,7,8");
  setPinnedLinks(",,,,,,,7,8");

  yield addNewTabPageTab();
  checkGrid("0,1,2,3,4,5,6,7p,8p");

  yield simulateExternalDrop(0);
  checkGrid("99p,0,1,2,3,4,5,7p,8p");

  
  
  yield setLinks("0,1,2,3,4,5,6,7,8");
  setPinnedLinks(",,,,,,,7,8");

  yield addNewTabPageTab();
  checkGrid("0,1,2,3,4,5,6,7p,8p");

  
  Services.prefs.setIntPref(PREF_NEWTAB_COLUMNS, 3);
  yield simulateExternalDrop(7);
  checkGrid("0,1,2,3,4,5,7p,99p,8p");

  
  
  yield setLinks("0,1,2,3,4,5,6,7,8");
  setPinnedLinks(",,,,,,,,8");

  yield addNewTabPageTab();
  checkGrid("0,1,2,3,4,5,6,7,8p");

  yield simulateExternalDrop(7);
  checkGrid("0,1,2,3,4,5,6,99p,8p");

  
  
  yield setLinks("0,1,2,3,4,5,6,7,8");
  setPinnedLinks("0,1,2,,,,,,");

  yield addNewTabPageTab();
  checkGrid("0p,1p,2p");

  yield simulateExternalDrop(1);
  checkGrid("0p,99p,1p,2p,3,4,5,6,7");
}
