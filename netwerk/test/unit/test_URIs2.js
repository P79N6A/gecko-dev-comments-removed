
Components.utils.import("resource://gre/modules/NetUtil.jsm");

var gIoService = Components.classes["@mozilla.org/network/io-service;1"]
                           .getService(Components.interfaces.nsIIOService);
















var gTests = [
  { spec:    "view-source:about:blank",
    scheme:  "view-source",
    prePath: "view-source:",
    path:    "about:blank",
    ref:     "",
    nsIURL:  false, nsINestedURI: true, immutable: true },
  { spec:    "view-source:http://www.mozilla.org/",
    scheme:  "view-source",
    prePath: "view-source:",
    path:    "http://www.mozilla.org/",
    ref:     "",
    nsIURL:  false, nsINestedURI: true, immutable: true },
  { spec:    "x-external:",
    scheme:  "x-external",
    prePath: "x-external:",
    path:    "",
    ref:     "",
    nsIURL:  false, nsINestedURI: false },
  { spec:    "x-external:abc",
    scheme:  "x-external",
    prePath: "x-external:",
    path:    "abc",
    ref:     "",
    nsIURL:  false, nsINestedURI: false },
  { spec:    "http://www2.example.com/",
    relativeURI: "a/b/c/d",
    scheme:  "http",
    prePath: "http://www2.example.com",
    path:    "/a/b/c/d",
    ref:     "",
    nsIURL:  true, nsINestedURI: false },
  
  { spec:    "http://a/b/c/d;p?q",
    relativeURI: "g:h",
    scheme:  "g",
    prePath: "g:",
    path:    "h",
    ref:     "",
    nsIURL:  false, nsINestedURI: false },
  { spec:    "http://a/b/c/d;p?q",
    relativeURI: "g",
    scheme:  "http",
    prePath: "http://a",
    path:    "/b/c/g",
    ref:     "",
    nsIURL:  true, nsINestedURI: false },
  { spec:    "http://a/b/c/d;p?q",
    relativeURI: "./g",
    scheme:  "http",
    prePath: "http://a",
    path:    "/b/c/g",
    ref:     "",
    nsIURL:  true, nsINestedURI: false },
  { spec:    "http://a/b/c/d;p?q",
    relativeURI: "g/",
    scheme:  "http",
    prePath: "http://a",
    path:    "/b/c/g/",
    ref:     "",
    nsIURL:  true, nsINestedURI: false },
  { spec:    "http://a/b/c/d;p?q",
    relativeURI: "/g",
    scheme:  "http",
    prePath: "http://a",
    path:    "/g",
    ref:     "",
    nsIURL:  true, nsINestedURI: false },
  { spec:    "http://a/b/c/d;p?q",
    relativeURI: "?y",
    scheme:  "http",
    prePath: "http://a",
    path:    "/b/c/d;p?y",
    ref:     "",
    nsIURL:  true, nsINestedURI: false },
  { spec:    "http://a/b/c/d;p?q",
    relativeURI: "g?y",
    scheme:  "http",
    prePath: "http://a",
    path:    "/b/c/g?y",
    ref:     "",
    specIgnoringRef: "http://a/b/c/g?y",
    hasRef:  false,
    nsIURL:  true, nsINestedURI: false },
  { spec:    "http://a/b/c/d;p?q",
    relativeURI: "#s",
    scheme:  "http",
    prePath: "http://a",
    path:    "/b/c/d;p?q#s",
    ref:     "s",
    specIgnoringRef: "http://a/b/c/d;p?q",
    hasRef:  true,
    nsIURL:  true, nsINestedURI: false },
  { spec:    "http://a/b/c/d;p?q",
    relativeURI: "g#s",
    scheme:  "http",
    prePath: "http://a",
    path:    "/b/c/g#s",
    ref:     "s",
    nsIURL:  true, nsINestedURI: false },
  { spec:    "http://a/b/c/d;p?q",
    relativeURI: "g?y#s",
    scheme:  "http",
    prePath: "http://a",
    path:    "/b/c/g?y#s",
    ref:     "s",
    nsIURL:  true, nsINestedURI: false },
  









  { spec:    "http://a/b/c/d;p?q",
    relativeURI: "g;x",
    scheme:  "http",
    prePath: "http://a",
    path:    "/b/c/g;x",
    ref:     "",
    nsIURL:  true, nsINestedURI: false },
  { spec:    "http://a/b/c/d;p?q",
    relativeURI: "g;x?y#s",
    scheme:  "http",
    prePath: "http://a",
    path:    "/b/c/g;x?y#s",
    ref:     "s",
    nsIURL:  true, nsINestedURI: false },
  









  { spec:    "http://a/b/c/d;p?q",
    relativeURI: ".",
    scheme:  "http",
    prePath: "http://a",
    path:    "/b/c/",
    ref:     "",
    nsIURL:  true, nsINestedURI: false },
  { spec:    "http://a/b/c/d;p?q",
    relativeURI: "./",
    scheme:  "http",
    prePath: "http://a",
    path:    "/b/c/",
    ref:     "",
    nsIURL:  true, nsINestedURI: false },
  { spec:    "http://a/b/c/d;p?q",
    relativeURI: "..",
    scheme:  "http",
    prePath: "http://a",
    path:    "/b/",
    ref:     "",
    nsIURL:  true, nsINestedURI: false },
  { spec:    "http://a/b/c/d;p?q",
    relativeURI: "../",
    scheme:  "http",
    prePath: "http://a",
    path:    "/b/",
    ref:     "",
    nsIURL:  true, nsINestedURI: false },
  { spec:    "http://a/b/c/d;p?q",
    relativeURI: "../g",
    scheme:  "http",
    prePath: "http://a",
    path:    "/b/g",
    ref:     "",
    nsIURL:  true, nsINestedURI: false },
  { spec:    "http://a/b/c/d;p?q",
    relativeURI: "../..",
    scheme:  "http",
    prePath: "http://a",
    path:    "/",
    ref:     "",
    nsIURL:  true, nsINestedURI: false },
  { spec:    "http://a/b/c/d;p?q",
    relativeURI: "../../",
    scheme:  "http",
    prePath: "http://a",
    path:    "/",
    ref:     "",
    nsIURL:  true, nsINestedURI: false },
  { spec:    "http://a/b/c/d;p?q",
    relativeURI: "../../g",
    scheme:  "http",
    prePath: "http://a",
    path:    "/g",
    ref:     "",
    nsIURL:  true, nsINestedURI: false },

  
  { spec:    "http://a/b/c/d;p?q",
    relativeURI: "../../../g",
    scheme:  "http",
    prePath: "http://a",
    path:    "/g",
    ref:     "",
    nsIURL:  true, nsINestedURI: false },
  { spec:    "http://a/b/c/d;p?q",
    relativeURI: "../../../../g",
    scheme:  "http",
    prePath: "http://a",
    path:    "/g",
    ref:     "",
    nsIURL:  true, nsINestedURI: false },

  
  { spec:    "http://a/b/c/d;p?q",
    relativeURI: "/./g",
    scheme:  "http",
    prePath: "http://a",
    path:    "/g",
    ref:     "",
    nsIURL:  true, nsINestedURI: false },
  { spec:    "http://a/b/c/d;p?q",
    relativeURI: "/../g",
    scheme:  "http",
    prePath: "http://a",
    path:    "/g",
    ref:     "",
    nsIURL:  true, nsINestedURI: false },
  { spec:    "http://a/b/c/d;p?q",
    relativeURI: "g.",
    scheme:  "http",
    prePath: "http://a",
    path:    "/b/c/g.",
    ref:     "",
    nsIURL:  true, nsINestedURI: false },
  { spec:    "http://a/b/c/d;p?q",
    relativeURI: ".g",
    scheme:  "http",
    prePath: "http://a",
    path:    "/b/c/.g",
    ref:     "",
    nsIURL:  true, nsINestedURI: false },
  { spec:    "http://a/b/c/d;p?q",
    relativeURI: "g..",
    scheme:  "http",
    prePath: "http://a",
    path:    "/b/c/g..",
    ref:     "",
    nsIURL:  true, nsINestedURI: false },
  { spec:    "http://a/b/c/d;p?q",
    relativeURI: "..g",
    scheme:  "http",
    prePath: "http://a",
    path:    "/b/c/..g",
    ref:     "",
    nsIURL:  true, nsINestedURI: false },
  { spec:    "http://a/b/c/d;p?q",
    relativeURI: ".",
    scheme:  "http",
    prePath: "http://a",
    path:    "/b/c/",
    ref:     "",
    nsIURL:  true, nsINestedURI: false },
  { spec:    "http://a/b/c/d;p?q",
    relativeURI: "./../g",
    scheme:  "http",
    prePath: "http://a",
    path:    "/b/g",
    ref:     "",
    nsIURL:  true, nsINestedURI: false },
  { spec:    "http://a/b/c/d;p?q",
    relativeURI: "./g/.",
    scheme:  "http",
    prePath: "http://a",
    path:    "/b/c/g/",
    ref:     "",
    nsIURL:  true, nsINestedURI: false },
  { spec:    "http://a/b/c/d;p?q",
    relativeURI: "g/./h",
    scheme:  "http",
    prePath: "http://a",
    path:    "/b/c/g/h",
    ref:     "",
    nsIURL:  true, nsINestedURI: false },
  { spec:    "http://a/b/c/d;p?q",
    relativeURI: "g/../h",
    scheme:  "http",
    prePath: "http://a",
    path:    "/b/c/h",
    ref:     "",
    nsIURL:  true, nsINestedURI: false },
  { spec:    "http://a/b/c/d;p?q",
    relativeURI: "g;x=1/./y",
    scheme:  "http",
    prePath: "http://a",
    path:    "/b/c/g;x=1/y",
    ref:     "",
    nsIURL:  true, nsINestedURI: false },
  { spec:    "http://a/b/c/d;p?q",
    relativeURI: "g;x=1/../y",
    scheme:  "http",
    prePath: "http://a",
    path:    "/b/c/y",
    ref:     "",
    nsIURL:  true, nsINestedURI: false },
  
  { spec:    "http://www2.example.com/",
    relativeURI: "//www3.example2.com/bar",
    scheme:  "http",
    prePath: "http://www3.example2.com",
    path:    "/bar",
    ref:     "",
    nsIURL:  true, nsINestedURI: false },
  { spec:    "https://www2.example.com/",
    relativeURI: "//www3.example2.com/bar",
    scheme:  "https",
    prePath: "https://www3.example2.com",
    path:    "/bar",
    ref:     "",
    nsIURL:  true, nsINestedURI: false },
];

var gHashSuffixes = [
  "#",
  "#myRef",
  "#myRef?a=b",
  "#myRef#",
  "#myRef#x:yz"
];



function do_info(text, stack) {
  if (!stack)
    stack = Components.stack.caller;

  dump( "\n" +
       "TEST-INFO | " + stack.filename + " | [" + stack.name + " : " +
       stack.lineNumber + "] " + text + "\n");
}







function do_check_uri_eq(aURI1, aURI2, aCheckTrueFunc) {
  if (!aCheckTrueFunc) {
    aCheckTrueFunc = do_check_true;
  }

  do_info("(uri equals check: '" + aURI1.spec + "' == '" + aURI2.spec + "')");
  aCheckTrueFunc(aURI1.equals(aURI2));
  do_info("(uri equals check: '" + aURI2.spec + "' == '" + aURI1.spec + "')");
  aCheckTrueFunc(aURI2.equals(aURI1));

  
  
  
  if (aCheckTrueFunc == do_check_true) {
    do_check_uri_eqExceptRef(aURI1, aURI2, aCheckTrueFunc);
  }
}





function do_check_uri_eqExceptRef(aURI1, aURI2, aCheckTrueFunc) {
  if (!aCheckTrueFunc) {
    aCheckTrueFunc = do_check_true;
  }

  do_info("(uri equalsExceptRef check: '" +
          aURI1.spec + "' == '" + aURI2.spec + "')");
  aCheckTrueFunc(aURI1.equalsExceptRef(aURI2));
  do_info("(uri equalsExceptRef check: '" +
          aURI2.spec + "' == '" + aURI1.spec + "')");
  aCheckTrueFunc(aURI2.equalsExceptRef(aURI1));
}




function do_check_property(aTest, aURI, aPropertyName, aTestFunctor) {
  if (aTest[aPropertyName]) {
    var expectedVal = aTestFunctor ?
                      aTestFunctor(aTest[aPropertyName]) :
                      aTest[aPropertyName];

    do_info("testing " + aPropertyName + " of " +
            (aTestFunctor ? "modified '" : "'" ) + aTest.spec +
            "' is '" + expectedVal + "'");
    do_check_eq(aURI[aPropertyName], expectedVal);
  }
}


function do_test_uri_basic(aTest) {
  var URI;

  do_info("Basic tests for " + aTest.spec + " relative URI: " + aTest.relativeURI);

  try {
    URI = NetUtil.newURI(aTest.spec);
  } catch(e) {
    do_info("Caught error on parse of" + aTest.spec + " Error: " + e.result);
    if (aTest.fail) {
      do_check_eq(e.result, aTest.result);
      return;
    }
    do_throw(e.result);
  }

  if (aTest.relativeURI) {
    var relURI;

    try {
      relURI = gIoService.newURI(aTest.relativeURI, null, URI);
    } catch (e) {
      do_info("Caught error on Relative parse of " + aTest.spec + " + " + aTest.relativeURI +" Error: " + e.result);
      if (aTest.relativeFail) {
        do_check_eq(e.result, aTest.relativeFail);
        return;
      }
      do_throw(e.result);
    }
    do_info("relURI.path = " + relURI.path + ", was " + URI.path);
    URI = relURI;
    do_info("URI.path now = " + URI.path);
  }

  
  do_info("testing " + aTest.spec + " equals a clone of itself");
  do_check_uri_eq(URI, URI.clone());
  do_check_uri_eqExceptRef(URI, URI.cloneIgnoringRef());
  do_info("testing " + aTest.spec + " instanceof nsIURL");
  do_check_eq(URI instanceof Ci.nsIURL, aTest.nsIURL);
  do_info("testing " + aTest.spec + " instanceof nsINestedURI");
  do_check_eq(URI instanceof Ci.nsINestedURI,
              aTest.nsINestedURI);

  do_info("testing that " + aTest.spec + " throws or returns false " +
          "from equals(null)");
  
  
  var threw = false;
  var isEqualToNull;
  try {
    isEqualToNull = URI.equals(null);
  } catch(e) {
    threw = true;
  }
  do_check_true(threw || !isEqualToNull);


  
  do_check_property(aTest, URI, "scheme");
  do_check_property(aTest, URI, "prePath");
  do_check_property(aTest, URI, "path");
  do_check_property(aTest, URI, "ref");
  do_check_property(aTest, URI, "port");
  do_check_property(aTest, URI, "username");
  do_check_property(aTest, URI, "password");
  do_check_property(aTest, URI, "host");
  do_check_property(aTest, URI, "specIgnoringRef");
  if ("hasRef" in aTest) {
    do_info("testing hasref: " + aTest.hasRef + " vs " + URI.hasRef);
    do_check_eq(aTest.hasRef, URI.hasRef);
  }
}


function do_test_uri_with_hash_suffix(aTest, aSuffix) {
  do_info("making sure caller is using suffix that starts with '#'");
  do_check_eq(aSuffix[0], "#");

  var origURI = NetUtil.newURI(aTest.spec);
  var testURI;

  if (aTest.relativeURI) {
    try {
      origURI = gIoService.newURI(aTest.relativeURI, null, origURI);
    } catch (e) {
      do_info("Caught error on Relative parse of " + aTest.spec + " + " + aTest.relativeURI +" Error: " + e.result);
      return;
    }
    try {
      testURI = gIoService.newURI(aSuffix, null, origURI);
    } catch (e) {
      do_info("Caught error adding suffix to " + aTest.spec + " + " + aTest.relativeURI + ", suffix " + aSuffix + " Error: " + e.result);
      return;
    }
  } else {
    testURI = NetUtil.newURI(aTest.spec + aSuffix);
  }

  do_info("testing " + aTest.spec + " with '" + aSuffix + "' appended " +
           "equals a clone of itself");
  do_check_uri_eq(testURI, testURI.clone());

  do_info("testing " + aTest.spec +
          " doesn't equal self with '" + aSuffix + "' appended");

  do_check_false(origURI.equals(testURI));

  do_info("testing " + aTest.spec +
          " is equalExceptRef to self with '" + aSuffix + "' appended");
  do_check_uri_eqExceptRef(origURI, testURI);

  do_check_eq(testURI.hasRef, true);

  if (!origURI.ref) {
    
    do_info("testing cloneIgnoringRef on " + testURI.spec +
            " is equal to no-ref version but not equal to ref version");
    var cloneNoRef = testURI.cloneIgnoringRef();
    do_check_uri_eq(cloneNoRef, origURI);
    do_check_false(cloneNoRef.equals(testURI));
  }

  do_check_property(aTest, testURI, "scheme");
  do_check_property(aTest, testURI, "prePath");
  if (!origURI.ref) {
    
    do_check_property(aTest, testURI, "path",
                      function(aStr) { return aStr + aSuffix; });
    do_check_property(aTest, testURI, "ref",
                      function(aStr) { return aSuffix.substr(1); });
  }
}


function do_test_mutate_ref(aTest, aSuffix) {
  do_info("making sure caller is using suffix that starts with '#'");
  do_check_eq(aSuffix[0], "#");

  var refURIWithSuffix    = NetUtil.newURI(aTest.spec + aSuffix);
  var refURIWithoutSuffix = NetUtil.newURI(aTest.spec);

  var testURI             = NetUtil.newURI(aTest.spec);

  
  do_info("testing that setting .ref on " + aTest.spec +
          " to '" + aSuffix + "' does what we expect");
  testURI.ref = aSuffix;
  do_check_uri_eq(testURI, refURIWithSuffix);
  do_check_uri_eqExceptRef(testURI, refURIWithoutSuffix);

  
  var suffixLackingHash = aSuffix.substr(1);
  if (suffixLackingHash) { 
    do_info("testing that setting .ref on " + aTest.spec +
            " to '" + suffixLackingHash + "' does what we expect");
    testURI.ref = suffixLackingHash;
    do_check_uri_eq(testURI, refURIWithSuffix);
    do_check_uri_eqExceptRef(testURI, refURIWithoutSuffix);
  }

  
  do_info("testing that clearing .ref on " + testURI.spec +
          " does what we expect");
  testURI.ref = "";
  do_check_uri_eq(testURI, refURIWithoutSuffix);
  do_check_uri_eqExceptRef(testURI, refURIWithSuffix);

  if (!aTest.relativeURI) {
    

    
    var specWithSuffix = aTest.spec + aSuffix;
    do_info("testing that setting spec to " +
            specWithSuffix + " and then clearing ref does what we expect");
    testURI.spec = specWithSuffix;
    testURI.ref = "";
    do_check_uri_eq(testURI, refURIWithoutSuffix);
    do_check_uri_eqExceptRef(testURI, refURIWithSuffix);

    
    if (!(testURI instanceof Ci.nsIJARURI)) {
      
      
      testURI = NetUtil.newURI(aTest.spec);

      var pathWithSuffix = aTest.path + aSuffix;
      do_info("testing that setting path to " +
              pathWithSuffix + " and then clearing ref does what we expect");
      testURI.path = pathWithSuffix;
      testURI.ref = "";
      do_check_uri_eq(testURI, refURIWithoutSuffix);
      do_check_uri_eqExceptRef(testURI, refURIWithSuffix);

      
      testURI.path = pathWithSuffix;
      do_info("testing that clearing path from " + 
              pathWithSuffix + " also clears .ref");
      testURI.path = "";
      do_check_eq(testURI.ref, "");
    }
  }
}



function do_test_immutable(aTest) {
  do_check_true(aTest.immutable);

  var URI = NetUtil.newURI(aTest.spec);
  
  var propertiesToCheck = ["spec", "scheme", "userPass", "username", "password",
                           "hostPort", "host", "port", "path", "ref"];

  propertiesToCheck.forEach(function(aProperty) {
    var threw = false;
    try {
      URI[aProperty] = "anothervalue";
    } catch(e) {
      threw = true;
    }

    do_info("testing that setting '" + aProperty +
            "' on immutable URI '" + aTest.spec + "' will throw");
    do_check_true(threw);
  });
}




function run_test()
{
  
  
  let base = gIoService.newURI("http://example.org/xenia?", null, null);
  let resolved = gIoService.newURI("?x", null, base);
  let expected = gIoService.newURI("http://example.org/xenia?x",
                                  null, null);
  do_info("Bug 662981: ACSII - comparing " + resolved.spec + " and " + expected.spec);
  do_check_true(resolved.equals(expected));

  
  
  base = gIoService.newURI("http://example.org/xènia?", null, null);
  resolved = gIoService.newURI("?x", null, base);
  expected = gIoService.newURI("http://example.org/xènia?x",
                              null, null);
  do_info("Bug 662981: UTF8 - comparing " + resolved.spec + " and " + expected.spec);
  do_check_true(resolved.equals(expected));

  gTests.forEach(function(aTest) {
    
    do_test_uri_basic(aTest);

    if (!aTest.fail) {
      
      gHashSuffixes.forEach(function(aSuffix) {
          do_test_uri_with_hash_suffix(aTest, aSuffix);
          if (!aTest.immutable) {
            do_test_mutate_ref(aTest, aSuffix);
          }
        });

      
      
      if (aTest.immutable) {
        do_test_immutable(aTest);
      }
    }
  });
}
