
Components.utils.import("resource://gre/modules/NetUtil.jsm");
const Ci = Components.interfaces;



var gTests = [
  { spec:    "about:foobar",
    scheme:  "about",
    prePath: "about:",
    path:    "foobar",
    ref:     "",
    nsIURL:  false, nsINestedURI: false, immutable: true },
  { spec:    "chrome://foobar/somedir/somefile.xml",
    scheme:  "chrome",
    prePath: "chrome://foobar",
    path:    "/somedir/somefile.xml",
    ref:     "",
    nsIURL:  true, nsINestedURI: false, immutable: true },
  { spec:    "data:text/html;charset=utf-8,<html></html>",
    scheme:  "data",
    prePath: "data:",
    path:    "text/html;charset=utf-8,<html></html>",
    ref:     "",
    nsIURL:  false, nsINestedURI: false },
  { spec:    "data:text/plain,hello world",
    scheme:  "data",
    prePath: "data:",
    path:    "text/plain,hello%20world",
    ref:     "",
    nsIURL:  false, nsINestedURI: false },
  { spec:    "file://",
    scheme:  "file",
    prePath: "file://",
    path:    "/",
    ref:     "",
    nsIURL:  true, nsINestedURI: false },
  { spec:    "file:///",
    scheme:  "file",
    prePath: "file://",
    path:    "/",
    ref:     "",
    nsIURL:  true, nsINestedURI: false },
  { spec:    "file:///myFile.html",
    scheme:  "file",
    prePath: "file://",
    path:    "/myFile.html",
    ref:     "",
    nsIURL:  true, nsINestedURI: false },
  { spec:    "http://",
    scheme:  "http",
    prePath: "http://",
    path:    "/",
    ref:     "",
    nsIURL:  true, nsINestedURI: false },
  { spec:    "http:///",
    scheme:  "http",
    prePath: "http://",
    path:    "/",
    ref:     "",
    nsIURL:  true, nsINestedURI: false },
  { spec:    "http://www.example.com/",
    scheme:  "http",
    prePath: "http://www.example.com",
    path:    "/",
    ref:     "",
    nsIURL:  true, nsINestedURI: false },
  { spec:    "jar:resource://!/",
    scheme:  "jar",
    prePath: "jar:",
    path:    "resource:///!/",
    ref:     "",
    nsIURL:  true, nsINestedURI: true },
  { spec:    "jar:resource://gre/chrome.toolkit.jar!/",
    scheme:  "jar",
    prePath: "jar:",
    path:    "resource://gre/chrome.toolkit.jar!/",
    ref:     "",
    nsIURL:  true, nsINestedURI: true },
  { spec:    "resource://gre/",
    scheme:  "resource",
    prePath: "resource://gre",
    path:    "/",
    ref:     "",
    nsIURL:  true, nsINestedURI: false },
  { spec:    "resource://gre/components/",
    scheme:  "resource",
    prePath: "resource://gre",
    path:    "/components/",
    ref:     "",
    nsIURL:  true, nsINestedURI: false },
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




function do_check_property(aTest, aURI, aPropertyName, aTestFunctor) {
  var expectedVal = aTestFunctor ?
    aTestFunctor(aTest[aPropertyName]) :
    aTest[aPropertyName];

  do_info("testing " + aPropertyName + " of " +
          (aTestFunctor ? "modified '" : "'" ) + aTest.spec +
          "' is '" + expectedVal + "'");
  do_check_eq(aURI[aPropertyName], expectedVal);
}


function do_test_uri_basic(aTest) {
  var URI = NetUtil.newURI(aTest.spec);

  
  do_info("testing " + aTest.spec + " equals a clone of itself");
  do_check_true(URI.equals(URI.clone()));
  do_info("testing " + aTest.spec + " instanceof nsIURL");
  do_check_eq(URI instanceof Ci.nsIURL, aTest.nsIURL);
  do_info("testing " + aTest.spec + " instanceof nsINestedURI");
  do_check_eq(URI instanceof Ci.nsINestedURI,
              aTest.nsINestedURI);

  
  do_check_property(aTest, URI, "scheme");
  do_check_property(aTest, URI, "prePath");
  do_check_property(aTest, URI, "path");

  
  
  if (aTest.nsIURL) {
    var URL = URI.QueryInterface(Ci.nsIURL);
    do_check_property(aTest, URL, "ref");
  }
}


function do_test_uri_with_hash_suffix(aTest, aSuffix) {
  do_info("making sure caller is using suffix that starts with '#'");
  do_check_eq(aSuffix[0], "#");

  var testURI = NetUtil.newURI(aTest.spec + aSuffix);

  do_info("testing " + aTest.spec +
          " doesn't equal self with '" + aSuffix + "' appended");

  var origURI = NetUtil.newURI(aTest.spec);
  if (aTest.spec == "file://") {
    do_info("TODO: bug 656853");
    todo_check_false(origURI.equals(testURI));
    return;  
  }

  do_check_false(origURI.equals(testURI));

  do_check_property(aTest, testURI, "scheme");
  do_check_property(aTest, testURI, "prePath");
  do_check_property(aTest, testURI, "path",
                    function(aStr) { return aStr + aSuffix; });

  
  
  if (aTest.nsIURL) {
    var URL = testURI.QueryInterface(Ci.nsIURL);
    do_check_property(aTest, URL, "ref",
                      function(aStr) { return aSuffix.substr(1); });
  }
}


function do_test_mutate_ref(aTest, aSuffix) {
  do_info("making sure caller is using suffix that starts with '#'");
  do_check_eq(aSuffix[0], "#");

  
  
  if (!aTest.nsIURL) {
    return;
  }

  var refURIWithSuffix    = NetUtil.newURI(aTest.spec + aSuffix).QueryInterface(Ci.nsIURL);
  var refURIWithoutSuffix = NetUtil.newURI(aTest.spec).QueryInterface(Ci.nsIURL);

  var testURI             = NetUtil.newURI(aTest.spec).QueryInterface(Ci.nsIURL);

  if (aTest.spec == "file://") {
    do_info("TODO: bug 656853");
    testURI.ref = aSuffix;
    todo_check_true(testURI.equals(refURIWithSuffix));
    return; 
  }

  
  do_info("testing that setting .ref on " + aTest.spec +
          " to '" + aSuffix + "' does what we expect");
  testURI.ref = aSuffix;
  do_check_true(testURI.equals(refURIWithSuffix));

  
  var suffixLackingHash = aSuffix.substr(1);
  if (suffixLackingHash) { 
    do_info("testing that setting .ref on " + aTest.spec +
            " to '" + suffixLackingHash + "' does what we expect");
    testURI.ref = suffixLackingHash;
    do_check_true(testURI.equals(refURIWithSuffix));
  }

  
  do_info("testing that clearing .ref on " + testURI.spec +
          " does what we expect");
  testURI.ref = "";
  do_check_true(testURI.equals(refURIWithoutSuffix));

  
  var specWithSuffix = aTest.spec + aSuffix;
  do_info("testing that setting spec to " +
          specWithSuffix + " and then clearing ref does what we expect");
  testURI.spec = specWithSuffix
  testURI.ref = "";
  if (aTest.spec == "http://" && aSuffix == "#") {
    do_info("TODO: bug 657033");
    todo_check_true(testURI.equals(refURIWithoutSuffix));
  } else {
    do_check_true(testURI.equals(refURIWithoutSuffix));
  }

  
  if (!(testURI instanceof Ci.nsIJARURI)) {
    
    
    testURI = NetUtil.newURI(aTest.spec).QueryInterface(Ci.nsIURL);

    var pathWithSuffix = aTest.path + aSuffix;
    do_info("testing that setting path to " +
            pathWithSuffix + " and then clearing ref does what we expect");
    testURI.path = pathWithSuffix;
    testURI.ref = "";
    do_check_true(testURI.equals(refURIWithoutSuffix));

    
    testURI.path = pathWithSuffix;
    do_info("testing that clearing path from " + 
            pathWithSuffix + " also clears .ref");
    testURI.path = "";
    do_check_eq(testURI.ref, "");
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
  gTests.forEach(function(aTest) {
    
    do_test_uri_basic(aTest);

    
    gHashSuffixes.forEach(function(aSuffix) {
      do_test_uri_with_hash_suffix(aTest, aSuffix);
      if (!aTest.immutable) {
        do_test_mutate_ref(aTest, aSuffix);
      }
    });

    
    
    if (aTest.immutable) {
      do_test_immutable(aTest);
    }
  });
}
