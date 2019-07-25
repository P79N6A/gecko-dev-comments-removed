






































if (typeof registerManifests === "undefined") {
  load("../unit/head_crtestutils.js");
}

let manifestFile = do_get_file("../unit/data/test_resolve_uris.manifest");

let manifests = [ manifestFile ];
registerManifests(manifests);

let ios = Cc["@mozilla.org/network/io-service;1"].
          getService(Ci.nsIIOService);

function do_run_test()
{
  let cr = Cc["@mozilla.org/chrome/chrome-registry;1"].
           getService(Ci.nsIChromeRegistry);

  
  
  var appInfo = Cc["@mozilla.org/xre/app-info;1"];
  if (!appInfo ||
      (appInfo.getService(Ci.nsIXULRuntime).processType ==
       Ci.nsIXULRuntime.PROCESS_TYPE_DEFAULT)) {
    cr.checkForNewChrome();
  }

  
  let registrationTypes = [
      "content",
      "locale",
      "skin",
      "override",
      "resource",
  ];

  for (let j = 0; j < registrationTypes.length; j++) {
    let type = registrationTypes[j];
    dump("Testing type '" + type + "'\n");
    let expectedURI = "resource://foo/foo-" + type + "/";
    let sourceURI = "chrome://foo/" + type + "/";
    switch (type) {
      case "content":
        expectedURI += "foo.xul";
        break;
      case "locale":
        expectedURI += "foo.dtd";
        break;
      case "skin":
        expectedURI += "foo.css";
        break;
      case "override":
        sourceURI = "chrome://good-package/content/override-me.xul";
        expectedURI += "override-me.xul";
        break;
      case "resource":
        expectedURI = ios.newFileURI(manifestFile.parent).spec;
        sourceURI = "resource://foo/";
        break;
    };
    try {
      sourceURI = ios.newURI(sourceURI, null, null);
      let uri;
      if (type == "resource") {
        
        let rph = ios.getProtocolHandler("resource").
            QueryInterface(Ci.nsIResProtocolHandler);
        uri = rph.resolveURI(sourceURI);
      }
      else {
        uri = cr.convertChromeURL(sourceURI).spec;
      }
      
      do_check_eq(expectedURI, uri);
    }
    catch (e) {
      dump(e + "\n");
      do_throw("Should have registered a handler for type '" +
               type + "'\n");
    }
  }
}

if (typeof run_test === "undefined") {
  run_test = function() {
    do_run_test();
  };
}
