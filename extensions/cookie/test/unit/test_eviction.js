const Cc = Components.classes;
const Ci = Components.interfaces;

function run_test()
{
  var cs = Cc["@mozilla.org/cookieService;1"].getService(Ci.nsICookieService);
  var cm = Cc["@mozilla.org/cookiemanager;1"].getService(Ci.nsICookieManager2);
  var ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
  var prefs = Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch);

  
  prefs.setIntPref("network.cookie.purgeAge", 1);
  prefs.setIntPref("network.cookie.maxNumber", 1000);

  
  
  
  

  
  
  
  do_check_eq(testEviction(cm, 1101, 2, 50, 1051), 1051);

  
  
  do_check_eq(testEviction(cm, 1101, 2, 100, 1001), 1001);

  
  
  do_check_eq(testEviction(cm, 1101, 2, 500, 1001), 1001);

  
  do_check_eq(testEviction(cm, 2000, 0, 0, 2000), 2000);

  
  do_check_eq(testEviction(cm, 1100, 2, 200, 1100), 1100);

  cm.removeAll();

  
  prefs.setIntPref("network.cookie.purgeAge", 30 * 24 * 60 * 60);
  prefs.setIntPref("network.cookie.maxNumber", 2000);
}



function
testEviction(aCM, aNumberTotal, aSleepDuration, aNumberOld, aNumberToExpect)
{
  aCM.removeAll();
  var expiry = (Date.now() + 1e6) * 1000;

  var i;
  for (i = 0; i < aNumberTotal; ++i) {
    var host = "eviction." + i + ".tests";
    aCM.add(host, "", "test", "eviction", false, false, false, expiry);

    if ((i == aNumberOld - 1) && aSleepDuration) {
      
      
      sleep(aSleepDuration * 1000);
    }
  }

  var enumerator = aCM.enumerator;

  i = 0;
  while (enumerator.hasMoreElements()) {
    var cookie = enumerator.getNext().QueryInterface(Ci.nsICookie2);
    ++i;

    if (aNumberTotal != aNumberToExpect) {
      
      var hostNumber = new Number(cookie.rawHost.split(".")[1]);
      if (hostNumber < (aNumberOld - aNumberToExpect)) break;
    }
  }

  return i;
}


function sleep(delay)
{
  var start = Date.now();
  while (Date.now() < start + delay);
}

