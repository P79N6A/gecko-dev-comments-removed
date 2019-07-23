




































const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;

function loadUtilsScript() {
  var loader = Cc["@mozilla.org/moz/jssubscript-loader;1"].
               getService(Ci.mozIJSSubScriptLoader);
  loader.loadSubScript("chrome://global/content/contentAreaUtils.js");
}

function test_urlSecurityCheck() {
  var nullPrincipal = Cc["@mozilla.org/nullprincipal;1"].
                      createInstance(Ci.nsIPrincipal);

  const HTTP_URI = "http://www.mozilla.org/";
  const CHROME_URI = "chrome://browser/content/browser.xul";
  const DISALLOW_INHERIT_PRINCIPAL =
    Ci.nsIScriptSecurityManager.DISALLOW_INHERIT_PRINCIPAL;

  try {
    urlSecurityCheck(makeURI(HTTP_URI), nullPrincipal,
                     DISALLOW_INHERIT_PRINCIPAL);
  }
  catch(ex) {
    do_throw("urlSecurityCheck should not throw when linking to a http uri with a null principal");
  }

  
  try {
    urlSecurityCheck(HTTP_URI, nullPrincipal,
                     DISALLOW_INHERIT_PRINCIPAL);
  }
  catch(ex) {
    do_throw("urlSecurityCheck failed to handle the http URI as a string (uri spec)");
  }

  try {
    urlSecurityCheck(CHROME_URI, nullPrincipal,
                     DISALLOW_INHERIT_PRINCIPAL);
    do_throw("urlSecurityCheck should throw when linking to a chrome uri with a null principal");
  }
  catch(ex) { }
}

function run_test()
{
  loadUtilsScript();
  test_urlSecurityCheck();
}
