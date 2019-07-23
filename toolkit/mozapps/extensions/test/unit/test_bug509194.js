





































const ID = "{972ce4c6-7e08-4474-a285-3208198ce6fd}";

function run_test()
{
  
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1");
  startupEM();

  var addon = gEM.getItemForID(ID);
  
  if (!addon)
    return;

  var iconURL = getManifestProperty(ID, "iconURL");
  var location = gEM.getInstallLocation(ID);
  var file = location.getItemFile(ID, "icon.png");
  var ioservice = Components.classes["@mozilla.org/network/io-service;1"]
                            .getService(Components.interfaces.nsIIOService);
  var uri = ioservice.newURI(iconURL, null, null);
  uri.QueryInterface(Components.interfaces.nsIFileURL);
  do_check_true(uri.file.equals(file));

  shutdownEM();
}
