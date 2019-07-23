






































let manifests = [
  do_get_file("data/test_data_protocol_registration.manifest"),
];
registerManifests(manifests);

let XULAppInfoFactory = {
  
  CID: XULAPPINFO_CID,
  scheme: "XULAppInfo",
  contractID: XULAPPINFO_CONTRACTID,
  createInstance: function (outer, iid) {
    if (outer != null)
      throw Cr.NS_ERROR_NO_AGGREGATION;
    return XULAppInfo.QueryInterface(iid);
  }
};

function run_test()
{
  
  let factories = [XULAppInfoFactory];

  
  for (let i = 0; i < factories.length; i++) {
    let factory = factories[i];
    Components.manager.QueryInterface(Ci.nsIComponentRegistrar)
              .registerFactory(factory.CID, "test-" + factory.scheme,
                               factory.contractID, factory);
  }

  
  let cr = Cc["@mozilla.org/chrome/chrome-registry;1"].
           getService(Ci.nsIChromeRegistry);
  cr.checkForNewChrome();

  
  let expectedURI = "data:application/vnd.mozilla.xul+xml,";
  let sourceURI = "chrome://good-package/content/test.xul";
  try {
    let ios = Cc["@mozilla.org/network/io-service;1"].
              getService(Ci.nsIIOService);
    sourceURI = ios.newURI(sourceURI, null, null);
    
    let uri = cr.convertChromeURL(sourceURI).spec;

    do_check_eq(expectedURI, uri);
  }
  catch (e) {
    dump(e + "\n");
    do_throw("Should have registered our URI!");
  }

  
  for (let i = 0; i < factories.length; i++) {
    let factory = factories[i];
    Components.manager.QueryInterface(Ci.nsIComponentRegistrar)
              .unregisterFactory(factory.CID, factory);
  }
}
