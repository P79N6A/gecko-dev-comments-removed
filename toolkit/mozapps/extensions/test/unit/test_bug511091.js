





































const ID = "bug511091@tests.mozilla.org";
const ADDON = "test_bug511091";

function run_test()
{
  
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1");

  
  startupEM();
  gEM.installItemFromFile(do_get_addon(ADDON), NS_INSTALL_LOCATION_APPPROFILE);
  var addon = gEM.getItemForID(ID);
  do_check_neq(addon, null);
  do_check_eq(getManifestProperty(ID, "iconURL"), "chrome://mozapps/skin/xpinstall/xpinstallItemGeneric.png");
  restartEM();

  var location = gEM.getInstallLocation(ID);
  var file = location.getItemFile(ID, "icon.png");
  var ioservice = Components.classes["@mozilla.org/network/io-service;1"]
                            .getService(Components.interfaces.nsIIOService);

  var addon = gEM.getItemForID(ID);
  do_check_neq(addon, null);
  var uri = ioservice.newURI(getManifestProperty(ID, "iconURL"), null, null);
  uri.QueryInterface(Components.interfaces.nsIFileURL);
  do_check_true(uri.file.equals(file));
  gEM.disableItem(ID);
  restartEM();

  addon = gEM.getItemForID(ID);
  do_check_neq(addon, null);
  uri = ioservice.newURI(getManifestProperty(ID, "iconURL"), null, null);
  uri.QueryInterface(Components.interfaces.nsIFileURL);
  do_check_true(uri.file.equals(file));

  shutdownEM();
}
