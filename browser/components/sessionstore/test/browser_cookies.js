


"use strict";

const PATH = "/browser/browser/components/sessionstore/test/";




add_task(function* test_setup() {
  Services.cookies.removeAll();
});




add_task(function* test_run() {
  
  
  
  yield testCookieCollection({
    host: "http://www.example.com",
    cookieHost: "www.example.com",
    cookieURIs: ["http://www.example.com" + PATH],
    noCookieURIs: ["http://example.com/" + PATH]
  });

  
  
  
  yield testCookieCollection({
    host: "http://example.com",
    cookieHost: "example.com",
    cookieURIs: ["http://example.com" + PATH],
    noCookieURIs: ["http://www.example.com/" + PATH]
  });

  
  
  
  yield testCookieCollection({
    host: "http://example.com",
    domain: "example.com",
    cookieHost: ".example.com",
    cookieURIs: ["http://example.com" + PATH, "http://www.example.com/" + PATH],
    noCookieURIs: ["about:robots"]
  });

  
  
  
  yield testCookieCollection({
    host: "http://example.com",
    domain: ".example.com",
    cookieHost: ".example.com",
    cookieURIs: ["http://example.com" + PATH, "http://www.example.com/" + PATH],
    noCookieURIs: ["about:robots"]
  });

  
  
  
  yield testCookieCollection({
    host: "http://www.example.com",
    domain: "www.example.com",
    cookieHost: ".www.example.com",
    cookieURIs: ["http://www.example.com/" + PATH],
    noCookieURIs: ["http://example.com"]
  });

  
  
  
  yield testCookieCollection({
    host: "http://www.example.com",
    domain: ".www.example.com",
    cookieHost: ".www.example.com",
    cookieURIs: ["http://www.example.com/" + PATH],
    noCookieURIs: ["http://example.com"]
  });
});






let testCookieCollection = Task.async(function (params) {
  let tab = gBrowser.addTab("about:blank");
  let browser = tab.linkedBrowser;

  let urlParams = new URLSearchParams();
  let value = Math.random();
  urlParams.append("value", value);

  if (params.domain) {
    urlParams.append("domain", params.domain);
  }

  
  let uri = `${params.host}${PATH}browser_cookies.sjs?${urlParams}`;

  
  
  
  yield Promise.all([
    waitForNewCookie(),
    replaceCurrentURI(browser, uri)
  ]);

  
  for (let uri of params.cookieURIs || []) {
    yield replaceCurrentURI(browser, uri);

    
    let cookie = getCookie();
    is(cookie.host, params.cookieHost, "cookie host is correct");
    is(cookie.path, PATH, "cookie path is correct");
    is(cookie.name, "foobar", "cookie name is correct");
    is(cookie.value, value, "cookie value is correct");
  }

  
  for (let uri of params.noCookieURIs || []) {
    yield replaceCurrentURI(browser, uri);

    
    ok(!getCookie(), "no cookie collected");
  }

  
  gBrowser.removeTab(tab);
  Services.cookies.removeAll();
});






let replaceCurrentURI = Task.async(function* (browser, uri) {
  
  let flags = Ci.nsIWebNavigation.LOAD_FLAGS_REPLACE_HISTORY;
  browser.loadURIWithFlags(uri, flags);
  yield promiseBrowserLoaded(browser);

  
  TabState.flush(browser);
});




function waitForNewCookie() {
  return new Promise(resolve => {
    Services.obs.addObserver(function observer(subj, topic, data) {
      let cookie = subj.QueryInterface(Ci.nsICookie2);
      if (data == "added" && cookie.host.endsWith("example.com")) {
        Services.obs.removeObserver(observer, topic);
        resolve();
      }
    }, "cookie-changed", false);
  });
}





function getCookie() {
  let state = JSON.parse(ss.getWindowState(window));
  let cookies = state.windows[0].cookies || [];
  return cookies[0] || null;
}
