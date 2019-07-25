



function run_test()
{
  
  Services.prefs.setIntPref("network.cookie.maxPerHost", 50);

  var cm = Cc["@mozilla.org/cookiemanager;1"].getService(Ci.nsICookieManager2);

  cm.removeAll();

  
  
  
  

  
  setCookies(cm, "foo.bar", 100);
  do_check_eq(countCookies(cm, "foo.bar", "foo.bar"), 50);

  
  
  setCookies(cm, "foo.baz", 10);
  setCookies(cm, ".foo.baz", 10);
  setCookies(cm, "bar.foo.baz", 10);
  setCookies(cm, "baz.bar.foo.baz", 10);
  setCookies(cm, "unrelated.domain", 50);
  do_check_eq(countCookies(cm, "foo.baz", "baz.bar.foo.baz"), 40);
  setCookies(cm, "foo.baz", 20);
  do_check_eq(countCookies(cm, "foo.baz", "baz.bar.foo.baz"), 50);

  
  
  setCookies(cm, "horse.radish", 10);

  
  
  sleep(2 * 1000);
  setCookies(cm, "tasty.horse.radish", 50);
  do_check_eq(countCookies(cm, "horse.radish", "horse.radish"), 50);

  let enumerator = cm.enumerator;
  while (enumerator.hasMoreElements()) {
    let cookie = enumerator.getNext().QueryInterface(Ci.nsICookie2);

    if (cookie.host == "horse.radish")
      do_throw("cookies not evicted by lastAccessed order");
  }

  cm.removeAll();
}


function
setCookies(aCM, aHost, aNumber)
{
  let expiry = (Date.now() + 1e6) * 1000;

  for (let i = 0; i < aNumber; ++i)
    aCM.add(aHost, "", "test" + i, "eviction", false, false, false, expiry);
}







function
countCookies(aCM, aBaseDomain, aHost)
{
  let enumerator = aCM.enumerator;

  
  
  let cookies = [];
  while (enumerator.hasMoreElements()) {
    let cookie = enumerator.getNext().QueryInterface(Ci.nsICookie2);

    if (cookie.host.length >= aBaseDomain.length &&
        cookie.host.slice(cookie.host.length - aBaseDomain.length) == aBaseDomain)
      cookies.push(cookie);
  }

  
  let result = cookies.length;
  do_check_eq(aCM.countCookiesFromHost(aBaseDomain), cookies.length);
  do_check_eq(aCM.countCookiesFromHost(aHost), cookies.length);

  enumerator = aCM.getCookiesFromHost(aHost);
  while (enumerator.hasMoreElements()) {
    let cookie = enumerator.getNext().QueryInterface(Ci.nsICookie2);

    if (cookie.host.length >= aBaseDomain.length &&
        cookie.host.slice(cookie.host.length - aBaseDomain.length) == aBaseDomain) {
      let found = false;
      for (let i = 0; i < cookies.length; ++i) {
        if (cookies[i].host == cookie.host && cookies[i].name == cookie.name) {
          found = true;
          cookies.splice(i, 1);
          break;
        }
      }

      if (!found)
        do_throw("cookie " + cookie.name + " not found in master enumerator");

    } else {
      do_throw("cookie host " + cookie.host + " not within domain " + aBaseDomain);
    }
  }

  do_check_eq(cookies.length, 0);

  return result;
}


function sleep(delay)
{
  var start = Date.now();
  while (Date.now() < start + delay);
}

