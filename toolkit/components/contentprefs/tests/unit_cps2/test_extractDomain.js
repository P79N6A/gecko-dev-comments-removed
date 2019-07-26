



function run_test() {
  let tests = {
    "http://example.com": "example.com",
    "http://example.com/": "example.com",
    "http://example.com/foo/bar/baz": "example.com",
    "http://subdomain.example.com/foo/bar/baz": "subdomain.example.com",
    "http://qix.quux.example.com/foo/bar/baz": "qix.quux.example.com",
    "not a url": "not a url",
  };
  let cps = Cc["@mozilla.org/content-pref/service;1"].
            getService(Ci.nsIContentPrefService2);
  for (let url in tests)
    do_check_eq(cps.extractDomain(url), tests[url]);
}
