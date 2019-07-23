

































































do_load_httpd_js();
var server;


var promptService = {
  alert: function(aParent, aDialogTitle, aText) {
  },
  
  alertCheck: function(aParent, aDialogTitle, aText, aCheckMsg, aCheckState) {
  },
  
  confirm: function(aParent, aDialogTitle, aText) {
  },
  
  confirmCheck: function(aParent, aDialogTitle, aText, aCheckMsg, aCheckState) {
  },
  
  confirmEx: function(aParent, aDialogTitle, aText, aButtonFlags, aButton0Title, aButton1Title, aButton2Title, aCheckMsg, aCheckState) {
  },
  
  prompt: function(aParent, aDialogTitle, aText, aValue, aCheckMsg, aCheckState) {
  },
  
  promptUsernameAndPassword: function(aParent, aDialogTitle, aText, aUsername, aPassword, aCheckMsg, aCheckState) {
  },

  promptPassword: function(aParent, aDialogTitle, aText, aPassword, aCheckMsg, aCheckState) {
  },
  
  select: function(aParent, aDialogTitle, aText, aCount, aSelectList, aOutSelection) {
  },
  
  QueryInterface: function(iid) {
    if (iid.equals(Components.interfaces.nsIPromptService)
     || iid.equals(Components.interfaces.nsISupports))
      return this;
  
    throw Components.results.NS_ERROR_NO_INTERFACE;
  }
};

var PromptServiceFactory = {
  createInstance: function (outer, iid) {
    if (outer != null)
      throw Components.results.NS_ERROR_NO_AGGREGATION;
    return promptService.QueryInterface(iid);
  }
};
var registrar = Components.manager.QueryInterface(Components.interfaces.nsIComponentRegistrar);
registrar.registerFactory(Components.ID("{6cc9c9fe-bc0b-432b-a410-253ef8bcc699}"), "PromptService",
                          "@mozilla.org/embedcomp/prompt-service;1", PromptServiceFactory);


var updateListener = {
onUpdateStarted: function()
{
},

onUpdateEnded: function()
{
  server.stop(do_test_finished);
},

onAddonUpdateStarted: function(addon)
{
},

onAddonUpdateEnded: function(addon, status)
{
  var nsIAddonUpdateCheckListener = Components.interfaces.nsIAddonUpdateCheckListener;
  switch (addon.id)
  {
    case "test_bug378216_5@tests.mozilla.org":
      
      do_check_eq(status, nsIAddonUpdateCheckListener.STATUS_FAILURE);
      break;
    case "test_bug378216_7@tests.mozilla.org":
      
      do_check_eq(status, nsIAddonUpdateCheckListener.STATUS_FAILURE);
      break;
    case "test_bug378216_8@tests.mozilla.org":
      
      do_check_eq(status, nsIAddonUpdateCheckListener.STATUS_NO_UPDATE);
      break;
    case "test_bug378216_9@tests.mozilla.org":
      
      do_check_eq(status, nsIAddonUpdateCheckListener.STATUS_UPDATE);
      do_check_eq(addon.version, "2.0");
      break;
    case "test_bug378216_10@tests.mozilla.org":
      
      do_check_eq(status, nsIAddonUpdateCheckListener.STATUS_UPDATE);
      do_check_eq(addon.version, "2.0");
      break;
    case "test_bug378216_11@tests.mozilla.org":
      
      do_check_eq(status, nsIAddonUpdateCheckListener.STATUS_UPDATE);
      do_check_eq(addon.version, "2.0");
      break;
    case "test_bug378216_12@tests.mozilla.org":
      
      do_check_eq(status, nsIAddonUpdateCheckListener.STATUS_NO_UPDATE);
      break;
    case "test_bug378216_13@tests.mozilla.org":
      
      do_check_eq(status, nsIAddonUpdateCheckListener.STATUS_UPDATE);
      do_check_eq(addon.version, "2.0");
      break;
    default:
      do_throw("Update check for unknown "+addon.id);
  }
}
}

function run_test()
{
  
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1");
  startupEM();
  gEM.installItemFromFile(do_get_addon("test_bug378216_1"), NS_INSTALL_LOCATION_APPPROFILE);
  gEM.installItemFromFile(do_get_addon("test_bug378216_2"), NS_INSTALL_LOCATION_APPPROFILE);
  gEM.installItemFromFile(do_get_addon("test_bug378216_3"), NS_INSTALL_LOCATION_APPPROFILE);
  gEM.installItemFromFile(do_get_addon("test_bug378216_4"), NS_INSTALL_LOCATION_APPPROFILE);
  gEM.installItemFromFile(do_get_addon("test_bug378216_5"), NS_INSTALL_LOCATION_APPPROFILE);
  gEM.installItemFromFile(do_get_addon("test_bug378216_6"), NS_INSTALL_LOCATION_APPPROFILE);
  gEM.installItemFromFile(do_get_addon("test_bug378216_7"), NS_INSTALL_LOCATION_APPPROFILE);
  gEM.installItemFromFile(do_get_addon("test_bug378216_8"), NS_INSTALL_LOCATION_APPPROFILE);
  gEM.installItemFromFile(do_get_addon("test_bug378216_9"), NS_INSTALL_LOCATION_APPPROFILE);
  gEM.installItemFromFile(do_get_addon("test_bug378216_10"), NS_INSTALL_LOCATION_APPPROFILE);
  gEM.installItemFromFile(do_get_addon("test_bug378216_11"), NS_INSTALL_LOCATION_APPPROFILE);
  gEM.installItemFromFile(do_get_addon("test_bug378216_12"), NS_INSTALL_LOCATION_APPPROFILE);
  gEM.installItemFromFile(do_get_addon("test_bug378216_13"), NS_INSTALL_LOCATION_APPPROFILE);
  
  restartEM();
  
  do_check_neq(gEM.getItemForID("test_bug378216_1@tests.mozilla.org"), null);
  
  do_check_eq(gEM.getItemForID("test_bug378216_2@tests.mozilla.org"), null);
  do_check_neq(gEM.getItemForID("test_bug378216_3@tests.mozilla.org"), null);
  do_check_neq(gEM.getItemForID("test_bug378216_4@tests.mozilla.org"), null);
  do_check_neq(gEM.getItemForID("test_bug378216_5@tests.mozilla.org"), null);
  do_check_neq(gEM.getItemForID("test_bug378216_6@tests.mozilla.org"), null);
  do_check_neq(gEM.getItemForID("test_bug378216_7@tests.mozilla.org"), null);
  do_check_neq(gEM.getItemForID("test_bug378216_8@tests.mozilla.org"), null);
  do_check_neq(gEM.getItemForID("test_bug378216_9@tests.mozilla.org"), null);
  do_check_neq(gEM.getItemForID("test_bug378216_10@tests.mozilla.org"), null);
  do_check_neq(gEM.getItemForID("test_bug378216_11@tests.mozilla.org"), null);
  do_check_neq(gEM.getItemForID("test_bug378216_12@tests.mozilla.org"), null);
  do_check_neq(gEM.getItemForID("test_bug378216_13@tests.mozilla.org"), null);

  server = new nsHttpServer();
  server.registerDirectory("/", do_get_file("data"));
  server.start(4444);
  
  var updates = [
    gEM.getItemForID("test_bug378216_5@tests.mozilla.org"),
    gEM.getItemForID("test_bug378216_7@tests.mozilla.org"),
    gEM.getItemForID("test_bug378216_8@tests.mozilla.org"),
    gEM.getItemForID("test_bug378216_9@tests.mozilla.org"),
    gEM.getItemForID("test_bug378216_10@tests.mozilla.org"),
    gEM.getItemForID("test_bug378216_11@tests.mozilla.org"),
    gEM.getItemForID("test_bug378216_12@tests.mozilla.org"),
    gEM.getItemForID("test_bug378216_13@tests.mozilla.org"),
  ];
  
  gEM.update(updates, updates.length,
             Components.interfaces.nsIExtensionManager.UPDATE_CHECK_NEWVERSION,
             updateListener);
  do_test_pending();
}
