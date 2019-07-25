








let pb = Cc["@mozilla.org/privatebrowsing;1"]
         .getService(Ci.nsIPrivateBrowsingService);

function runTests() {
  
  setLinks("0,1,2,3,4,5,6,7,8,9");
  ok(!pb.privateBrowsingEnabled, "private browsing is disabled");

  yield addNewTabPageTab();
  pinCell(cells[0]);
  checkGrid("0p,1,2,3,4,5,6,7,8");

  
  yield togglePrivateBrowsing();
  ok(pb.privateBrowsingEnabled, "private browsing is enabled");

  yield addNewTabPageTab();
  checkGrid("0p,1,2,3,4,5,6,7,8");

  
  yield blockCell(cells[1]);
  checkGrid("0p,2,3,4,5,6,7,8");

  yield unpinCell(cells[0]);
  checkGrid("0,2,3,4,5,6,7,8");

  
  yield togglePrivateBrowsing();
  ok(!pb.privateBrowsingEnabled, "private browsing is disabled");

  
  yield addNewTabPageTab();
  checkGrid("0p,1,2,3,4,5,6,7,8");
}

function togglePrivateBrowsing() {
  let topic = "private-browsing-transition-complete";

  Services.obs.addObserver(function observe() {
    Services.obs.removeObserver(observe, topic);
    executeSoon(TestRunner.next);
  }, topic, false);

  pb.privateBrowsingEnabled = !pb.privateBrowsingEnabled;
}
