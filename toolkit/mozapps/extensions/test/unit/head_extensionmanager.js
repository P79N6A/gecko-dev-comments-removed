










































const PREFIX_NS_EM                    = "http://www.mozilla.org/2004/em-rdf#";
const PREFIX_NS_CHROME                = "http://www.mozilla.org/rdf/chrome#";
const PREFIX_ITEM_URI                 = "urn:mozilla:item:";

const NS_APP_USER_PROFILE_50_DIR      = "ProfD";
const NS_APP_PROFILE_DIR_STARTUP      = "ProfDS";
const NS_OS_TEMP_DIR                  = "TmpD";

const NS_INSTALL_LOCATION_APPPROFILE = "app-profile";

const XULAPPINFO_CONTRACTID = "@mozilla.org/xre/app-info;1";
const XULAPPINFO_CID = Components.ID("{c763b610-9d49-455a-bbd2-ede71682a1ac}");

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
  var lf = gTestRoot.clone();
  lf.append("unit");
  lf.append("addons");
  lf.append(name + ".xpi");
  if (!lf.exists())
    do_throw("Addon "+name+" does not exist.");

  return lf;
}












function createAppInfo(id, name, version, platformVersion)
{
  var XULAppInfo = {
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
      return XULAppInfo.QueryInterface(iid);
    }
  };
  var registrar = Components.manager.QueryInterface(Components.interfaces.nsIComponentRegistrar);
  registrar.registerFactory(XULAPPINFO_CID, "XULAppInfo",
                            XULAPPINFO_CONTRACTID, XULAppInfoFactory);
}





function startupEM()
{
  
  var updateSvc = Components.classes["@mozilla.org/updates/update-service;1"]
                            .getService(Components.interfaces.nsISupports);
  
  gEM = Components.classes["@mozilla.org/extensions/manager;1"]
                  .getService(Components.interfaces.nsIExtensionManager);
  
  gEM.QueryInterface(Components.interfaces.nsIObserver);
  gEM.observe(null, "app-startup", null);
  gEM.observe(null, "profile-after-change", "startup");
  
  var needsRestart = false;
  try {
    needsRestart = gEM.checkForMismatches();
  }
  catch (e) {
    needsRestart = gEM.start(null);
  }
  
  if (needsRestart)
    gEM.start(null);
}





function shutdownEM()
{
  
  gDirSvc.unregisterProvider(dirProvider);
  gEM = null;
}





function restartEM()
{
  
  var extensions = gProfD.clone();
  extensions.append("extensions");
  if (extensions.exists()) {
    var ioServ = Components.classes["@mozilla.org/network/io-service;1"]
                           .getService(Components.interfaces.nsIIOService);
    var addons = extensions.directoryEntries;
    while (addons.hasMoreElements()) {
      var addon = addons.getNext().QueryInterface(Components.interfaces.nsIFile);
      if (addon.isDirectory() && addon.leafName != "staged-xpis") {
        addon.append("install.rdf");
        if (addon.exists()) {
          var ds = gRDF.GetDataSource(ioServ.newFileURI(addon).spec);
          gRDF.UnregisterDataSource(ds);
        }
      }
    }
  }
  
  var needsRestart = gEM.start(null);
  if (needsRestart)
    gEM.start(null);
}

var gDirSvc = Components.classes["@mozilla.org/file/directory_service;1"]
                        .getService(Components.interfaces.nsIProperties);
var gTestRoot = gDirSvc.get("CurProcD", Components.interfaces.nsILocalFile);
gTestRoot = gTestRoot.parent.parent;
gTestRoot.append("_tests");
gTestRoot.append("xpcshell-simple");
gTestRoot.append("test_extensionmanager");


var gProfD = gTestRoot.clone();
gProfD.append("profile");
if (gProfD.exists())
  gProfD.remove(true);
gProfD.create(Components.interfaces.nsIFile.DIRECTORY_TYPE, 0755);

var dirProvider = {
  getFile: function(prop, persistent) {
    persistent.value = true;
    if (prop == NS_APP_USER_PROFILE_50_DIR ||
        prop == NS_APP_PROFILE_DIR_STARTUP)
      return gProfD.clone();
    return null;
  },
  QueryInterface: function(iid) {
    if (iid.equals(Components.interfaces.nsIDirectoryServiceProvider) ||
        iid.equals(Components.interfaces.nsISupports)) {
      return this;
    }
    throw Components.results.NS_ERROR_NO_INTERFACE;
  }
};
gDirSvc.QueryInterface(Components.interfaces.nsIDirectoryService)
       .registerProvider(dirProvider);

var gPrefs = Components.classes["@mozilla.org/preferences;1"]
                   .getService(Components.interfaces.nsIPrefBranch);

gPrefs.setBoolPref("extensions.logging.enabled", true);
