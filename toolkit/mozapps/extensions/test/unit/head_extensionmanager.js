










































const PREFIX_NS_EM                    = "http://www.mozilla.org/2004/em-rdf#";
const PREFIX_NS_CHROME                = "http://www.mozilla.org/rdf/chrome#";
const PREFIX_ITEM_URI                 = "urn:mozilla:item:";

const NS_INSTALL_LOCATION_APPPROFILE = "app-profile";

const XULAPPINFO_CONTRACTID = "@mozilla.org/xre/app-info;1";
const XULAPPINFO_CID = Components.ID("{c763b610-9d49-455a-bbd2-ede71682a1ac}");

var gXULAppInfo = null;
var gEM = null;
var gRDF = Components.classes["@mozilla.org/rdf/rdf-service;1"]
                     .getService(Components.interfaces.nsIRDFService);




function EM_NS(property) {
  return PREFIX_NS_EM + property;
}

function CHROME_NS(property) {
  return PREFIX_NS_CHROME + property;
}

function EM_R(property) {
  return gRDF.GetResource(EM_NS(property));
}

function EM_L(literal) {
  return gRDF.GetLiteral(literal);
}

function EM_I(integer) {
  return gRDF.GetIntLiteral(integer);
}

function EM_D(integer) {
  return gRDF.GetDateLiteral(integer);
}








function stringData(literalOrResource) {
  if (literalOrResource instanceof Components.interfaces.nsIRDFLiteral)
    return literalOrResource.Value;
  if (literalOrResource instanceof Components.interfaces.nsIRDFResource)
    return literalOrResource.Value;
  return undefined;
}







function intData(literal) {
  if (literal instanceof Components.interfaces.nsIRDFInt)
    return literal.Value;
  return undefined;
}








function getResourceForID(id) {
  return gRDF.GetResource(PREFIX_ITEM_URI + id);
}










function getManifestProperty(id, property) {
  return stringData(gEM.datasource.GetTarget(getResourceForID(id), EM_R(property), true));
}







function do_get_addon(name)
{
  return do_get_file("addons/" + name + ".xpi");
}












function createAppInfo(id, name, version, platformVersion)
{
  gXULAppInfo = {
    vendor: "Mozilla",
    name: name,
    ID: id,
    version: version,
    appBuildID: "2007010101",
    platformVersion: platformVersion,
    platformBuildID: "2007010101",
    inSafeMode: false,
    logConsoleErrors: true,
    OS: "XPCShell",
    XPCOMABI: "noarch-spidermonkey",
    invalidateCachesOnRestart: function invalidateCachesOnRestart() {},

    QueryInterface: function QueryInterface(iid) {
      if (iid.equals(Components.interfaces.nsIXULAppInfo)
       || iid.equals(Components.interfaces.nsIXULRuntime)
       || iid.equals(Components.interfaces.nsISupports))
        return this;

      throw Components.results.NS_ERROR_NO_INTERFACE;
    }
  };

  var XULAppInfoFactory = {
    createInstance: function (outer, iid) {
      if (outer != null)
        throw Components.results.NS_ERROR_NO_AGGREGATION;
      return gXULAppInfo.QueryInterface(iid);
    }
  };
  var registrar = Components.manager.QueryInterface(Components.interfaces.nsIComponentRegistrar);
  registrar.registerFactory(XULAPPINFO_CID, "XULAppInfo",
                            XULAPPINFO_CONTRACTID, XULAppInfoFactory);
}

function startEM(upgraded) {
  var needsRestart = false;
  if (upgraded) {
    try {
      needsRestart = gEM.checkForMismatches();
    }
    catch (e) {
      do_throw("checkForMismatches threw an exception: " + e + "\n");
      needsRestart = false;
      upgraded = false;
    }
  }

  if (!upgraded || !needsRestart)
    needsRestart = gEM.start();

  if (needsRestart)
    restartEM();
}





function startupEM()
{
  gEM = Components.classes["@mozilla.org/extensions/manager;1"]
                  .getService(Components.interfaces.nsIExtensionManager);

  gEM.QueryInterface(Components.interfaces.nsIObserver);
  gEM.observe(null, "profile-after-change", "startup");

  
  
  startEM(true);
}





function shutdownEM()
{
  
  gEM = null;
}





function restartEM(newVersion)
{
  if (newVersion) {
    gXULAppInfo.version = newVersion;
    startEM(true);
  }
  else {
    startEM(false);
  }
}

var gDirSvc = Components.classes["@mozilla.org/file/directory_service;1"]
                        .getService(Components.interfaces.nsIProperties);


var gProfD = do_get_profile();

var gPrefs = Components.classes["@mozilla.org/preferences-service;1"]
                   .getService(Components.interfaces.nsIPrefBranch);

gPrefs.setBoolPref("extensions.logging.enabled", true);
