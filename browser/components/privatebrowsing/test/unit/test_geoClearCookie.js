



































const accessToken = '{"location":{"latitude":51.5090332,"longitude":-0.1212726,"accuracy":150.0},"access_token":"2:jVhRZJ-j6PiRchH_:RGMrR0W1BiwdZs12"}'
function run_test() {
  if (!("@mozilla.org/privatebrowsing;1" in Components.classes)) {
    do_check_true(true);
    return;
  }
  var prefBranch = Components.classes["@mozilla.org/preferences-service;1"]
                             .getService(Components.interfaces.nsIPrefBranch2);
  var pb = Components.classes["@mozilla.org/privatebrowsing;1"]
                     .getService(Components.interfaces.nsIPrivateBrowsingService);
  prefBranch.setCharPref("geo.wifi.access_token.test", accessToken);
  var token = prefBranch.getCharPref("geo.wifi.access_token.test");
  do_check_eq(token, accessToken);
  pb.privateBrowsingEnabled = true;
  token = "";
  try {
    token = prefBranch.getCharPref("geo.wifi.access_token.test");
  }
  catch(e){}
  finally {
    do_check_eq(token, "");
  }
  token = "";
  prefBranch.setCharPref("geo.wifi.access_token.test", accessToken);
  pb.privateBrowsingEnabled = false;
  try {
    token = prefBranch.getCharPref("geo.wifi.access_token.test");
  }
  catch(e){}
  finally {
    do_check_eq(token, "");
  }
}
