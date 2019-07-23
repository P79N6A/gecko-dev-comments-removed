





































const Cc = Components.classes;
const Ci = Components.interfaces;

const URI_XPINSTALL_CONFIRM_DIALOG = "chrome://mozapps/content/xpinstall/xpinstallConfirm.xul";


function findXPI(dpb, name) {
  for (var i = 0; i < 5; i++) {
    if (dpb.GetString(i * 4 + 1).substr(-(name.length + 1)) == "/" + name)
      return i * 4;
  }
  do_throw(name + " wasn't in the list");
}


var WindowWatcher = {
  openWindow: function(parent, url, name, features, arguments) {
    do_check_eq(url, URI_XPINSTALL_CONFIRM_DIALOG);
    var dpb = arguments.QueryInterface(Ci.nsISupportsInterfacePointer)
                       .data.QueryInterface(Ci.nsIDialogParamBlock);
    do_check_eq(dpb.GetInt(1), 20);

    
    var unsigned = findXPI(dpb, "unsigned.xpi");
    var signed = findXPI(dpb, "signed.xpi");
    var untrusted = findXPI(dpb, "signed-untrusted.xpi");
    var no_o = findXPI(dpb, "signed-no-o.xpi");
    var no_cn = findXPI(dpb, "signed-no-cn.xpi");

    
    do_check_eq(dpb.GetString(unsigned), "XPI Test");
    do_check_eq(dpb.GetString(unsigned + 3), "");

    do_check_eq(dpb.GetString(signed), "Signed XPI Test");
    do_check_eq(dpb.GetString(signed + 3), "Object Signer");
    do_check_eq(dpb.GetString(no_o), "Signed XPI Test (No Org)");
    do_check_eq(dpb.GetString(no_o + 3), "Object Signer");
    do_check_eq(dpb.GetString(no_cn), "Signed XPI Test (No Common Name)");
    do_check_eq(dpb.GetString(no_cn + 3), "Mozilla Testing");

    
    do_check_eq(dpb.GetString(untrusted), "Signed XPI Test - Untrusted");
    do_check_eq(dpb.GetString(untrusted + 3), "");

    
    dpb.SetInt(0, 0);
  },

  QueryInterface: function(iid) {
    if (iid.equals(Ci.nsIWindowWatcher)
     || iid.equals(Ci.nsISupports))
      return this;

    throw Cr.NS_ERROR_NO_INTERFACE;
  }
}

var WindowWatcherFactory = {
  createInstance: function createInstance(outer, iid) {
    if (outer != null)
      throw Components.results.NS_ERROR_NO_AGGREGATION;
    return WindowWatcher.QueryInterface(iid);
  }
};

var registrar = Components.manager.QueryInterface(Ci.nsIComponentRegistrar);
registrar.registerFactory(Components.ID("{1dfeb90a-2193-45d5-9cb8-864928b2af55}"),
                          "Fake Window Watcher",
                          "@mozilla.org/embedcomp/window-watcher;1", WindowWatcherFactory);

function run_test()
{
  
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1");

  
  do_get_file("data/cert8.db").copyTo(gProfD, null);
  do_get_file("data/key3.db").copyTo(gProfD, null);
  do_get_file("data/secmod.db").copyTo(gProfD, null);

  
  var il = gProfD.clone();
  il.append("extensions");
  if (!il.exists())
    il.create(Ci.nsIFile.DIRECTORY_TYPE, 0755);
  do_get_file("data/unsigned.xpi").copyTo(il, null);
  do_get_file("data/signed.xpi").copyTo(il, null);
  do_get_file("data/signed-untrusted.xpi").copyTo(il, null);
  do_get_file("data/signed-tampered.xpi").copyTo(il, null);
  do_get_file("data/signed-no-o.xpi").copyTo(il, null);
  do_get_file("data/signed-no-cn.xpi").copyTo(il, null);

  
  startupEM();
  do_check_neq(gEM.getItemForID("signed-xpi@tests.mozilla.org"), null);
  do_check_neq(gEM.getItemForID("unsigned-xpi@tests.mozilla.org"), null);
  do_check_neq(gEM.getItemForID("untrusted-xpi@tests.mozilla.org"), null);
  do_check_eq(gEM.getItemForID("tampered-xpi@tests.mozilla.org"), null);
  do_check_neq(gEM.getItemForID("signed-xpi-no-o@tests.mozilla.org"), null);
  do_check_neq(gEM.getItemForID("signed-xpi-no-cn@tests.mozilla.org"), null);

  shutdownEM();
}
