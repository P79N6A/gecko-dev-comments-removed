





































const ADDON = "test_bug425657";
const ID = "bug425657@tests.mozilla.org";

function run_test()
{
  
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1");

  
  startupEM();
  gEM.installItemFromFile(do_get_addon(ADDON), NS_INSTALL_LOCATION_APPPROFILE);
  var addon = gEM.getItemForID(ID);
  do_check_neq(addon, null);
  do_check_eq(addon.name, "Deutsches W\u00f6rterbuch");
  restartEM();

  addon = gEM.getItemForID(ID);
  do_check_neq(addon, null);
  do_check_eq(addon.name, "Deutsches W\u00f6rterbuch");
  do_check_eq(addon.name.length, 20);

  shutdownEM();
}
