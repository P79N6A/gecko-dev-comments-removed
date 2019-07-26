



const LIST_UPDATED_TOPIC     = "plugins-list-updated";


var { getIDHashForString } = Components.utils.import("resource://gre/modules/PluginProvider.jsm");

function PluginTag(name, description) {
  this.name = name;
  this.description = description;
}

PluginTag.prototype = {
  name: null,
  description: null,
  version: "1.0",
  filename: null,
  fullpath: null,
  disabled: false,
  blocklisted: false,
  clicktoplay: false,

  mimeTypes: [],

  getMimeTypes: function(count) {
    count.value = this.mimeTypes.length;
    return this.mimeTypes;
  }
};

PLUGINS = [
  
  new PluginTag("Java", "A mock Java plugin"),

  
  new PluginTag("Flash", "A mock Flash plugin"),
  new PluginTag("Flash", "A mock Flash plugin")
];

gPluginHost = {
  
  getPluginTags: function(count) {
    count.value = PLUGINS.length;
    return PLUGINS;
  },

  QueryInterface: XPCOMUtils.generateQI([AM_Ci.nsIPluginHost])
};

var PluginHostFactory = {
  createInstance: function (outer, iid) {
    if (outer != null)
      throw Components.results.NS_ERROR_NO_AGGREGATION;
    return gPluginHost.QueryInterface(iid);
  }
};

var registrar = Components.manager.QueryInterface(AM_Ci.nsIComponentRegistrar);
registrar.registerFactory(Components.ID("{aa6f9fef-cbe2-4d55-a2fa-dcf5482068b9}"), "PluginHost",
                          "@mozilla.org/plugin/host;1", PluginHostFactory);



function run_test() {
  do_test_pending();
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9.2");

  startupManager();
  AddonManager.addAddonListener(AddonListener);
  AddonManager.addInstallListener(InstallListener);

  run_test_1();
}

function end_test() {
  do_test_finished();
}

function sortAddons(addons) {
  addons.sort(function(a, b) {
    return a.name.localeCompare(b.name);
  });
}


function run_test_1() {
  AddonManager.getAddonsByTypes(["plugin"], function(addons) {
    sortAddons(addons);

    do_check_eq(addons.length, 2);

    do_check_eq(addons[0].name, "Flash");
    do_check_false(addons[0].userDisabled);
    do_check_eq(addons[1].name, "Java");
    do_check_false(addons[1].userDisabled);

    run_test_2();
  });
}


function run_test_2() {
  
  let tag = PLUGINS[0];
  PLUGINS[0] = PLUGINS[2];
  PLUGINS[2] = PLUGINS[1];
  PLUGINS[1] = tag;

  Services.obs.notifyObservers(null, LIST_UPDATED_TOPIC, null);

  AddonManager.getAddonsByTypes(["plugin"], function(addons) {
    sortAddons(addons);

    do_check_eq(addons.length, 2);

    do_check_eq(addons[0].name, "Flash");
    do_check_false(addons[0].userDisabled);
    do_check_eq(addons[1].name, "Java");
    do_check_false(addons[1].userDisabled);

    run_test_3();
  });
}


function run_test_3() {
  let tag = new PluginTag("Quicktime", "A mock Quicktime plugin");
  PLUGINS.push(tag);
  let id = getIDHashForString(tag.name + tag.description);

  let test_params = {};
  test_params[id] = [
    ["onInstalling", false],
    "onInstalled"
  ];

  prepare_test(test_params, [
    "onExternalInstall"
  ]);

  Services.obs.notifyObservers(null, LIST_UPDATED_TOPIC, null);

  ensure_test_completed();

  AddonManager.getAddonsByTypes(["plugin"], function(addons) {
    sortAddons(addons);

    do_check_eq(addons.length, 3);

    do_check_eq(addons[0].name, "Flash");
    do_check_false(addons[0].userDisabled);
    do_check_eq(addons[1].name, "Java");
    do_check_false(addons[1].userDisabled);
    do_check_eq(addons[2].name, "Quicktime");
    do_check_false(addons[2].userDisabled);

    run_test_4();
  });
}


function run_test_4() {
  let tag = PLUGINS.splice(1, 1)[0];
  let id = getIDHashForString(tag.name + tag.description);

  let test_params = {};
  test_params[id] = [
    ["onUninstalling", false],
    "onUninstalled"
  ];

  prepare_test(test_params);

  Services.obs.notifyObservers(null, LIST_UPDATED_TOPIC, null);

  ensure_test_completed();

  AddonManager.getAddonsByTypes(["plugin"], function(addons) {
    sortAddons(addons);

    do_check_eq(addons.length, 2);

    do_check_eq(addons[0].name, "Flash");
    do_check_false(addons[0].userDisabled);
    do_check_eq(addons[1].name, "Quicktime");
    do_check_false(addons[1].userDisabled);

    run_test_5();
  });
}


function run_test_5() {
  PLUGINS.splice(0, 1);

  Services.obs.notifyObservers(null, LIST_UPDATED_TOPIC, null);

  ensure_test_completed();

  AddonManager.getAddonsByTypes(["plugin"], function(addons) {
    sortAddons(addons);

    do_check_eq(addons.length, 2);

    do_check_eq(addons[0].name, "Flash");
    do_check_false(addons[0].userDisabled);
    do_check_eq(addons[1].name, "Quicktime");
    do_check_false(addons[1].userDisabled);

    run_test_6();
  });
}


function run_test_6() {
  let oldTag = PLUGINS.splice(0, 1)[0];
  let newTag = new PluginTag("Flash 2", "A new crash-free Flash!");
  newTag.disabled = true;
  PLUGINS.push(newTag);

  let test_params = {};
  test_params[getIDHashForString(oldTag.name + oldTag.description)] = [
    ["onUninstalling", false],
    "onUninstalled"
  ];
  test_params[getIDHashForString(newTag.name + newTag.description)] = [
    ["onInstalling", false],
    "onInstalled"
  ];

  prepare_test(test_params, [
    "onExternalInstall"
  ]);

  Services.obs.notifyObservers(null, LIST_UPDATED_TOPIC, null);

  ensure_test_completed();

  AddonManager.getAddonsByTypes(["plugin"], function(addons) {
    sortAddons(addons);

    do_check_eq(addons.length, 2);

    do_check_eq(addons[0].name, "Flash 2");
    do_check_true(addons[0].userDisabled);
    do_check_eq(addons[1].name, "Quicktime");
    do_check_false(addons[1].userDisabled);

    run_test_7();
  });
}



function run_test_7() {
  PLUGINS[0] = new PluginTag("Quicktime", "A mock Quicktime plugin");
  PLUGINS[0].disabled = true;
  PLUGINS[1] = new PluginTag("Flash 2", "A new crash-free Flash!");

  let test_params = {};
  test_params[getIDHashForString(PLUGINS[0].name + PLUGINS[0].description)] = [
    ["onDisabling", false],
    "onDisabled"
  ];
  test_params[getIDHashForString(PLUGINS[1].name + PLUGINS[1].description)] = [
    ["onEnabling", false],
    "onEnabled"
  ];

  prepare_test(test_params);

  Services.obs.notifyObservers(null, LIST_UPDATED_TOPIC, null);

  ensure_test_completed();

  AddonManager.getAddonsByTypes(["plugin"], function(addons) {
    sortAddons(addons);

    do_check_eq(addons.length, 2);

    do_check_eq(addons[0].name, "Flash 2");
    do_check_false(addons[0].userDisabled);
    do_check_eq(addons[1].name, "Quicktime");
    do_check_true(addons[1].userDisabled);

    end_test();
  });
}
