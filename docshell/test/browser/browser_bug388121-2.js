function test() {
  waitForExplicitFinish(); 

  var w;
  const secMan = Cc["@mozilla.org/scriptsecuritymanager;1"].getService(Ci.nsIScriptSecurityManager);
  var iteration = 1;
  const uris = ["", "about:blank"];
  var uri;
  var origDoc;

  function testLoad() {
    if (w.document == origDoc) {
      
      setTimeout(testLoad, 10);
      return;
    }
    var prin = w.document.nodePrincipal;
    isnot(prin, null, "Loaded principal must not be null when adding " + uri);
    isnot(prin, undefined, "Loaded principal must not be undefined when loading " + uri);
    is(secMan.isSystemPrincipal(prin), false,
       "Loaded principal must not be system when loading " + uri);
    w.close();

    if (iteration == uris.length) {
      finish();
    } else {
      ++iteration;
      doTest();
    }
  }

  function doTest() {
    uri = uris[iteration - 1];
    w = window.open(uri, "_blank", "width=10,height=10");
    var prin = w.document.nodePrincipal;
    if (!uri) {
      uri = undefined;
    }
    isnot(prin, null, "Forced principal must not be null when loading " + uri);
    isnot(prin, undefined,
          "Forced principal must not be undefined when loading " + uri);
    is(secMan.isSystemPrincipal(prin), false,
       "Forced principal must not be system when loading " + uri);
    if (uri == undefined) {
      
      w.close();
      ++iteration;
      doTest();
    } else {
      origDoc = w.document;
      
      
      setTimeout(testLoad, 10);
    }
  }

  doTest();
}
