






































const UNPACKAGED_ADDON = do_get_file("data/test_bug564667");
const PACKAGED_ADDON = do_get_file("data/test_bug564667.xpi");

var gIOS = Cc["@mozilla.org/network/io-service;1"].
           getService(Ci.nsIIOService);

var gCR = Cc["@mozilla.org/chrome/chrome-registry;1"].
          getService(Ci.nsIChromeRegistry).
          QueryInterface(Ci.nsIXULOverlayProvider);




function test_mapping(chromeURL, target) {
  var uri = gIOS.newURI(chromeURL, null, null);

  try {
    var result = gCR.convertChromeURL(uri);
    do_check_eq(result.spec, target);
  }
  catch (ex) {
    do_throw(chromeURL + " not Registered");
  }
}




function test_removed_mapping(chromeURL, target) {
  var uri = gIOS.newURI(chromeURL, null, null);
  try {
    var result = gCR.convertChromeURL(uri);
    do_throw(chromeURL + " not removed");
  }
  catch (ex) {
    
  }
}







function test_no_overlays(chromeURL, target, type) {
  var type = type || "overlay";
  var uri = gIOS.newURI(chromeURL, null, null);
  var present = false, elem;

  var overlays = (type == "overlay") ?
      gCR.getXULOverlays(uri) : gCR.getStyleOverlays(uri);

  
  if (overlays.hasMoreElements()) {
    if (type == "styles")
      do_throw("Style Registered: " + chromeURL);
    else
      do_throw("Overlay Registered: " + chromeURL);
  }
}

function testManifest(manifestPath, baseURI) {

  
  Components.manager.addBootstrappedManifestLocation(manifestPath);

  
  test_mapping("chrome://test1/content", baseURI + "test/test1.xul");

  
  test_mapping("chrome://test1/locale", baseURI + "test/test1.dtd");

  
  test_mapping("chrome://test1/skin", baseURI + "test/test1.css");

  
  test_mapping("chrome://test2/content", baseURI + "test/test2.xul");
  test_mapping("chrome://test2/locale", baseURI + "test/test2.dtd");

  
  test_mapping("chrome://testOverride/content", 'file:///test1/override')

  
  test_no_overlays("chrome://test1/content/overlay.xul",
                   "chrome://test1/content/test1.xul");

  
  test_no_overlays("chrome://test1/content/style.xul",
                   "chrome://test1/content/test1.css", "styles");


  
  Components.manager.removeBootstrappedManifestLocation(manifestPath);

  
  test_removed_mapping("chrome://test1/content", baseURI + "test/test1.xul");

  
  test_removed_mapping("chrome://test1/locale", baseURI + "test/test1.dtd");

  
  test_removed_mapping("chrome://test1/skin", baseURI + "test/test1.css");

  
  test_removed_mapping("chrome://test2/content", baseURI + "test/test2.xul");
  test_removed_mapping("chrome://test2/locale", baseURI + "test/test2.dtd");
}

function run_test() {
  
  testManifest(UNPACKAGED_ADDON, gIOS.newFileURI(UNPACKAGED_ADDON).spec);

  
  testManifest(PACKAGED_ADDON, "jar:" + gIOS.newFileURI(PACKAGED_ADDON).spec + "!/");
}
