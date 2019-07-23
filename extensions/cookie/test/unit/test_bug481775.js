const Cc = Components.classes;
const Ci = Components.interfaces;

function run_test() {
  var cm = Cc["@mozilla.org/cookiemanager;1"].getService(Ci.nsICookieManager2);
  var prefs = Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch);
  var pb = null;
  try {
    pb = Cc["@mozilla.org/privatebrowsing;1"].getService(Ci.nsIPrivateBrowsingService);
  } catch (e) {}

  
  prefs.setIntPref("network.cookie.lifetimePolicy", 0);
  cm.removeAll();

  
  addCookies(0, 5000);

  
  var count = getCookieCount();
  do_check_neq(count, 0);

  
  if (pb) {
    
    pb.privateBrowsingEnabled = true;

    
    do_check_eq(getCookieCount(), 0);

    
    addCookies(5000, 5000);

    
    do_check_eq(getCookieCount(), count);

    
    cm.removeAll();
    do_check_eq(getCookieCount(), 0);

    
    pb.privateBrowsingEnabled = false;
  }

  
  do_check_eq(getCookieCount(), count);

  
  addCookies(10000, 1000);

  
  var count = getCookieCount();
  do_check_eq(getCookieCount(), count);

  
  cm.removeAll();
  do_check_eq(getCookieCount(), 0);
}

function getCookieCount() {
  var count = 0;
  var cm = Cc["@mozilla.org/cookiemanager;1"].getService(Ci.nsICookieManager2);
  var enumerator = cm.enumerator;
  while (enumerator.hasMoreElements()) {
    if (!(enumerator.getNext() instanceof Ci.nsICookie2))
      throw new Error("not a cookie");
    ++count;
  }
  return count;
}

function addCookies(start, count) {
  var cm = Cc["@mozilla.org/cookiemanager;1"].getService(Ci.nsICookieManager2);
  var expiry = (Date.now() + 1000) * 1000;
  for (var i = start; i < start + count; ++i)
    cm.add(i + ".bar", "", "foo", "bar", false, false, true, expiry);
}

