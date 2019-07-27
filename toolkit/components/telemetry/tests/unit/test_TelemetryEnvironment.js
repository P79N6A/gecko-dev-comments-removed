


Cu.import("resource://gre/modules/AddonManager.jsm");
Cu.import("resource://gre/modules/TelemetryEnvironment.jsm", this);
Cu.import("resource://gre/modules/Preferences.jsm", this);
Cu.import("resource://gre/modules/PromiseUtils.jsm", this);
Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);
Cu.import("resource://testing-common/AddonManagerTesting.jsm");
Cu.import("resource://testing-common/httpd.js");
Cu.import("resource://testing-common/MockRegistrar.jsm", this);


XPCOMUtils.defineLazyModuleGetter(this, "LightweightThemeManager",
                                  "resource://gre/modules/LightweightThemeManager.jsm");


XPCOMUtils.defineLazyModuleGetter(this, "ProfileAge",
                                  "resource://gre/modules/ProfileAge.jsm");


let gHttpServer = null;

let gHttpRoot = null;

let gDataRoot = null;

let gNow = new Date(2010, 1, 1, 12, 0, 0);
fakeNow(gNow);

const PLATFORM_VERSION = "1.9.2";
const APP_VERSION = "1";
const APP_ID = "xpcshell@tests.mozilla.org";
const APP_NAME = "XPCShell";
const APP_HOTFIX_VERSION = "2.3.4a";

const DISTRIBUTION_ID = "distributor-id";
const DISTRIBUTION_VERSION = "4.5.6b";
const DISTRIBUTOR_NAME = "Some Distributor";
const DISTRIBUTOR_CHANNEL = "A Channel";
const PARTNER_NAME = "test";
const PARTNER_ID = "NicePartner-ID-3785";

const GFX_VENDOR_ID = "0xabcd";
const GFX_DEVICE_ID = "0x1234";


const PROFILE_RESET_DATE_MS = Date.now();

const PROFILE_CREATION_DATE_MS = PROFILE_RESET_DATE_MS - MILLISECONDS_PER_DAY;

const FLASH_PLUGIN_NAME = "Shockwave Flash";
const FLASH_PLUGIN_DESC = "A mock flash plugin";
const FLASH_PLUGIN_VERSION = "\u201c1.1.1.1\u201d";
const PLUGIN_MIME_TYPE1 = "application/x-shockwave-flash";
const PLUGIN_MIME_TYPE2 = "text/plain";

const PLUGIN2_NAME = "Quicktime";
const PLUGIN2_DESC = "A mock Quicktime plugin";
const PLUGIN2_VERSION = "2.3";

const PERSONA_ID = "3785";

const PERSONA_ID_SUFFIX = "@personas.mozilla.org";
const PERSONA_NAME = "Test Theme";
const PERSONA_DESCRIPTION = "A nice theme/persona description.";

const PLUGIN_UPDATED_TOPIC     = "plugins-list-updated";




function PluginTag(aName, aDescription, aVersion, aEnabled) {
  this.name = aName;
  this.description = aDescription;
  this.version = aVersion;
  this.disabled = !aEnabled;
}

PluginTag.prototype = {
  name: null,
  description: null,
  version: null,
  filename: null,
  fullpath: null,
  disabled: false,
  blocklisted: false,
  clicktoplay: true,

  mimeTypes: [ PLUGIN_MIME_TYPE1, PLUGIN_MIME_TYPE2 ],

  getMimeTypes: function(count) {
    count.value = this.mimeTypes.length;
    return this.mimeTypes;
  }
};


let gInstalledPlugins = [
  new PluginTag("Java", "A mock Java plugin", "1.0", false ),
  new PluginTag(FLASH_PLUGIN_NAME, FLASH_PLUGIN_DESC, FLASH_PLUGIN_VERSION, true),
];


let PluginHost = {
  getPluginTags: function(countRef) {
    countRef.value = gInstalledPlugins.length;
    return gInstalledPlugins;
  },

  QueryInterface: function(iid) {
    if (iid.equals(Ci.nsIPluginHost)
     || iid.equals(Ci.nsISupports))
      return this;

    throw Components.results.NS_ERROR_NO_INTERFACE;
  }
}

function registerFakePluginHost() {
  MockRegistrar.register("@mozilla.org/plugin/host;1", PluginHost);
}




function spoofTheme(aId, aName, aDesc) {
  return {
    id: aId,
    name: aName,
    description: aDesc,
    headerURL: "http://lwttest.invalid/a.png",
    footerURL: "http://lwttest.invalid/b.png",
    textcolor: Math.random().toString(),
    accentcolor: Math.random().toString()
  };
}

function spoofGfxAdapter() {
  try {
    let gfxInfo = Cc["@mozilla.org/gfx/info;1"].getService(Ci.nsIGfxInfoDebug);
    gfxInfo.spoofVendorID(GFX_VENDOR_ID);
    gfxInfo.spoofDeviceID(GFX_DEVICE_ID);
  } catch (x) {
    
  }
}

function spoofProfileReset() {
  if (gIsAndroid) {
    
    return true;
  }

  let profileAccessor = new ProfileAge();

  return profileAccessor.writeTimes({
    created: PROFILE_CREATION_DATE_MS,
    reset: PROFILE_RESET_DATE_MS
  });
}

function spoofPartnerInfo() {
  let prefsToSpoof = {};
  prefsToSpoof["distribution.id"] = DISTRIBUTION_ID;
  prefsToSpoof["distribution.version"] = DISTRIBUTION_VERSION;
  prefsToSpoof["app.distributor"] = DISTRIBUTOR_NAME;
  prefsToSpoof["app.distributor.channel"] = DISTRIBUTOR_CHANNEL;
  prefsToSpoof["app.partner.test"] = PARTNER_NAME;
  prefsToSpoof["mozilla.partner.id"] = PARTNER_ID;

  
  for (let pref in prefsToSpoof) {
    Preferences.set(pref, prefsToSpoof[pref]);
  }
}







function checkString(aValue) {
  return (typeof aValue == "string") && (aValue != "");
}








function checkNullOrString(aValue) {
  if (aValue) {
    return checkString(aValue);
  } else if (aValue === null) {
    return true;
  }

  return false;
}








function checkNullOrBool(aValue) {
  if (aValue) {
    return (typeof aValue == "boolean");
  } else if (aValue === null) {
    return true;
  }

  return false;
}

function checkBuildSection(data) {
  const expectedInfo = {
    applicationId: APP_ID,
    applicationName: APP_NAME,
    buildId: "2007010101",
    version: APP_VERSION,
    vendor: "Mozilla",
    platformVersion: PLATFORM_VERSION,
    xpcomAbi: "noarch-spidermonkey",
  };

  Assert.ok("build" in data, "There must be a build section in Environment.");

  for (let f in expectedInfo) {
    Assert.ok(checkString(data.build[f]), f + " must be a valid string.");
    Assert.equal(data.build[f], expectedInfo[f], f + " must have the correct value.");
  }

  
  Assert.ok(checkString(data.build.architecture));
  Assert.ok(checkString(data.build.hotfixVersion));
  Assert.equal(data.build.hotfixVersion, APP_HOTFIX_VERSION);

  if (gIsMac) {
    let macUtils = Cc["@mozilla.org/xpcom/mac-utils;1"].getService(Ci.nsIMacUtils);
    if (macUtils && macUtils.isUniversalBinary) {
      Assert.ok(checkString(data.build.architecturesInBinary));
    }
  }
}

function checkSettingsSection(data) {
  const EXPECTED_FIELDS_TYPES = {
    blocklistEnabled: "boolean",
    e10sEnabled: "boolean",
    telemetryEnabled: "boolean",
    locale: "string",
    update: "object",
    userPrefs: "object",
  };

  Assert.ok("settings" in data, "There must be a settings section in Environment.");

  for (let f in EXPECTED_FIELDS_TYPES) {
    Assert.equal(typeof data.settings[f], EXPECTED_FIELDS_TYPES[f],
                 f + " must have the correct type.");
  }

  
  
  if (gIsAndroid) {
    Assert.ok(!("isDefaultBrowser" in data.settings), "Must not be available on Android.");
  } else {
    Assert.ok(checkNullOrBool(data.settings.isDefaultBrowser));
  }

  
  let update = data.settings.update;
  Assert.ok(checkNullOrString(update.channel));
  Assert.equal(typeof update.enabled, "boolean");
  Assert.equal(typeof update.autoDownload, "boolean");

  
  if ("defaultSearchEngine" in data.settings) {
    checkString(data.settings.defaultSearchEngine);
    Assert.equal(typeof data.settings.defaultSearchEngineData, "object");
  }
}

function checkProfileSection(data) {
  if (gIsAndroid) {
    Assert.ok(!("profile" in data),
              "There must be no profile section in Environment on Android.");
    return;
  }

  Assert.ok("profile" in data, "There must be a profile section in Environment.");
  Assert.equal(data.profile.creationDate, truncateToDays(PROFILE_CREATION_DATE_MS));
  Assert.equal(data.profile.resetDate, truncateToDays(PROFILE_RESET_DATE_MS));
}

function checkPartnerSection(data) {
  const EXPECTED_FIELDS = {
    distributionId: DISTRIBUTION_ID,
    distributionVersion: DISTRIBUTION_VERSION,
    partnerId: PARTNER_ID,
    distributor: DISTRIBUTOR_NAME,
    distributorChannel: DISTRIBUTOR_CHANNEL,
  };

  Assert.ok("partner" in data, "There must be a partner section in Environment.");

  for (let f in EXPECTED_FIELDS) {
    Assert.equal(data.partner[f], EXPECTED_FIELDS[f], f + " must have the correct value.");
  }

  
  Assert.ok(Array.isArray(data.partner.partnerNames));
  Assert.ok(data.partner.partnerNames.indexOf(PARTNER_NAME) >= 0);
}

function checkGfxAdapter(data) {
  const EXPECTED_ADAPTER_FIELDS_TYPES = {
    description: "string",
    vendorID: "string",
    deviceID: "string",
    subsysID: "string",
    RAM: "number",
    driver: "string",
    driverVersion: "string",
    driverDate: "string",
    GPUActive: "boolean",
  };

  for (let f in EXPECTED_ADAPTER_FIELDS_TYPES) {
    Assert.ok(f in data, f + " must be available.");

    if (data[f]) {
      
      Assert.equal(typeof data[f], EXPECTED_ADAPTER_FIELDS_TYPES[f],
                   f + " must have the correct type.");
    }
  }
}

function checkSystemSection(data) {
  const EXPECTED_FIELDS = [ "memoryMB", "cpu", "os", "hdd", "gfx" ];
  const EXPECTED_HDD_FIELDS = [ "profile", "binary", "system" ];

  Assert.ok("system" in data, "There must be a system section in Environment.");

  
  for (let f of EXPECTED_FIELDS) {
    Assert.ok(f in data.system, f + " must be available.");
  }

  Assert.ok(Number.isFinite(data.system.memoryMB), "MemoryMB must be a number.");
  if (gIsWindows) {
    Assert.equal(typeof data.system.isWow64, "boolean",
              "isWow64 must be available on Windows and have the correct type.");
  }

  let cpuData = data.system.cpu;
  Assert.ok(Number.isFinite(cpuData.count), "CPU count must be a number.");
  Assert.ok(Array.isArray(cpuData.extensions), "CPU extensions must be available.");

  
  if (gIsAndroid || gIsGonk) {
    let deviceData = data.system.device;
    Assert.ok(checkNullOrString(deviceData.model));
    Assert.ok(checkNullOrString(deviceData.manufacturer));
    Assert.ok(checkNullOrString(deviceData.hardware));
    Assert.ok(checkNullOrBool(deviceData.isTablet));
  }

  let osData = data.system.os;
  Assert.ok(checkNullOrString(osData.name));
  Assert.ok(checkNullOrString(osData.version));
  Assert.ok(checkNullOrString(osData.locale));

  
  if (gIsWindows) {
    Assert.ok(Number.isFinite(osData["servicePackMajor"]),
              "ServicePackMajor must be a number.");
    Assert.ok(Number.isFinite(osData["servicePackMinor"]),
              "ServicePackMinor must be a number.");
  } else if (gIsAndroid || gIsGonk) {
    Assert.ok(checkNullOrString(osData.kernelVersion));
  }

  let check = gIsWindows ? checkString : checkNullOrString;
  for (let disk of EXPECTED_HDD_FIELDS) {
    Assert.ok(check(data.system.hdd[disk].model));
    Assert.ok(check(data.system.hdd[disk].revision));
  }

  let gfxData = data.system.gfx;
  Assert.ok("D2DEnabled" in gfxData);
  Assert.ok("DWriteEnabled" in gfxData);
  
  
  
  if (gIsWindows) {
    Assert.equal(typeof gfxData.D2DEnabled, "boolean");
    Assert.equal(typeof gfxData.DWriteEnabled, "boolean");
    
    
  }

  Assert.ok("adapters" in gfxData);
  Assert.ok(gfxData.adapters.length > 0, "There must be at least one GFX adapter.");
  for (let adapter of gfxData.adapters) {
    checkGfxAdapter(adapter);
  }
  Assert.equal(typeof gfxData.adapters[0].GPUActive, "boolean");
  Assert.ok(gfxData.adapters[0].GPUActive, "The first GFX adapter must be active.");

  Assert.ok(Array.isArray(gfxData.monitors));
  if (gIsWindows || gIsMac) {
    Assert.ok(gfxData.monitors.length >= 1, "There is at least one monitor.");
    Assert.equal(typeof gfxData.monitors[0].screenWidth, "number");
    Assert.equal(typeof gfxData.monitors[0].screenHeight, "number");
    if (gIsWindows) {
      Assert.equal(typeof gfxData.monitors[0].refreshRate, "number");
      Assert.equal(typeof gfxData.monitors[0].pseudoDisplay, "boolean");
    }
    if (gIsMac) {
      Assert.equal(typeof gfxData.monitors[0].scale, "number");
    }
  }

  try {
    
    
    let gfxInfo = Cc["@mozilla.org/gfx/info;1"].getService(Ci.nsIGfxInfoDebug);

    if (gIsWindows || gIsMac) {
      Assert.equal(GFX_VENDOR_ID, gfxData.adapters[0].vendorID);
      Assert.equal(GFX_DEVICE_ID, gfxData.adapters[0].deviceID);
    }
  }
  catch (e) {}
}

function checkActiveAddon(data){
  const EXPECTED_ADDON_FIELDS_TYPES = {
    blocklisted: "boolean",
    name: "string",
    userDisabled: "boolean",
    appDisabled: "boolean",
    version: "string",
    scope: "number",
    type: "string",
    foreignInstall: "boolean",
    hasBinaryComponents: "boolean",
    installDay: "number",
    updateDay: "number",
    signedState: "number",
  };

  for (let f in EXPECTED_ADDON_FIELDS_TYPES) {
    Assert.ok(f in data, f + " must be available.");
    Assert.equal(typeof data[f], EXPECTED_ADDON_FIELDS_TYPES[f],
                 f + " must have the correct type.");
  }

  
  Assert.ok(checkNullOrString(data.description));
}

function checkPlugin(data) {
  const EXPECTED_PLUGIN_FIELDS_TYPES = {
    name: "string",
    version: "string",
    description: "string",
    blocklisted: "boolean",
    disabled: "boolean",
    clicktoplay: "boolean",
    updateDay: "number",
  };

  for (let f in EXPECTED_PLUGIN_FIELDS_TYPES) {
    Assert.ok(f in data, f + " must be available.");
    Assert.equal(typeof data[f], EXPECTED_PLUGIN_FIELDS_TYPES[f],
                 f + " must have the correct type.");
  }

  Assert.ok(Array.isArray(data.mimeTypes));
  for (let type of data.mimeTypes) {
    Assert.ok(checkString(type));
  }
}

function checkTheme(data) {
  
  const EXPECTED_THEME_FIELDS_TYPES = {
    id: "string",
    blocklisted: "boolean",
    name: "string",
    userDisabled: "boolean",
    appDisabled: "boolean",
    version: "string",
    scope: "number",
    foreignInstall: "boolean",
    installDay: "number",
    updateDay: "number",
  };

  for (let f in EXPECTED_THEME_FIELDS_TYPES) {
    Assert.ok(f in data, f + " must be available.");
    Assert.equal(typeof data[f], EXPECTED_THEME_FIELDS_TYPES[f],
                 f + " must have the correct type.");
  }

  
  Assert.ok(checkNullOrString(data.description));
}

function checkActiveGMPlugin(data) {
  Assert.equal(typeof data.version, "string");
  Assert.equal(typeof data.userDisabled, "boolean");
  Assert.equal(typeof data.applyBackgroundUpdates, "boolean");
}

function checkAddonsSection(data) {
  const EXPECTED_FIELDS = [
    "activeAddons", "theme", "activePlugins", "activeGMPlugins", "activeExperiment",
    "persona",
  ];

  Assert.ok("addons" in data, "There must be an addons section in Environment.");
  for (let f of EXPECTED_FIELDS) {
    Assert.ok(f in data.addons, f + " must be available.");
  }

  
  let activeAddons = data.addons.activeAddons;
  for (let addon in activeAddons) {
    checkActiveAddon(activeAddons[addon]);
  }

  
  if (Object.keys(data.addons.theme).length !== 0) {
    checkTheme(data.addons.theme);
  }

  
  Assert.ok(Array.isArray(data.addons.activePlugins));
  for (let plugin of data.addons.activePlugins) {
    checkPlugin(plugin);
  }

  
  let activeGMPlugins = data.addons.activeGMPlugins;
  if (!gIsAndroid) {
    
    
    for (let gmPlugin in activeGMPlugins) {
      checkActiveGMPlugin(activeGMPlugins[gmPlugin]);
    }
  }

  
  let experiment = data.addons.activeExperiment;
  if (Object.keys(experiment).length !== 0) {
    Assert.ok(checkString(experiment.id));
    Assert.ok(checkString(experiment.branch));
  }

  
  Assert.ok(checkNullOrString(data.addons.persona));
}

function checkEnvironmentData(data) {
  checkBuildSection(data);
  checkSettingsSection(data);
  checkProfileSection(data);
  checkPartnerSection(data);
  checkSystemSection(data);
  checkAddonsSection(data);
}

function run_test() {
  
  do_load_manifest("chrome.manifest");
  do_test_pending();
  spoofGfxAdapter();
  do_get_profile();
  loadAddonManager(APP_ID, APP_NAME, APP_VERSION, PLATFORM_VERSION);

  
  if (!gIsGonk) {
    LightweightThemeManager.currentTheme =
      spoofTheme(PERSONA_ID, PERSONA_NAME, PERSONA_DESCRIPTION);
  }
  
  registerFakePluginHost();

  
  gHttpServer = new HttpServer();
  gHttpServer.start(-1);
  let port = gHttpServer.identity.primaryPort;
  gHttpRoot = "http://localhost:" + port + "/";
  gDataRoot = gHttpRoot + "data/";
  gHttpServer.registerDirectory("/data/", do_get_cwd());
  do_register_cleanup(() => gHttpServer.stop(() => {}));

  spoofPartnerInfo();
  
  Preferences.set("extensions.hotfix.lastVersion", APP_HOTFIX_VERSION);

  run_next_test();
}

function isRejected(promise) {
  return new Promise((resolve, reject) => {
    promise.then(() => resolve(false), () => resolve(true));
  });
}

add_task(function* asyncSetup() {
  yield spoofProfileReset();
});

add_task(function* test_checkEnvironment() {
  let environmentData = yield TelemetryEnvironment.onInitialized();
  checkEnvironmentData(environmentData);
});

add_task(function* test_prefWatchPolicies() {
  const PREF_TEST_1 = "toolkit.telemetry.test.pref_new";
  const PREF_TEST_2 = "toolkit.telemetry.test.pref1";
  const PREF_TEST_3 = "toolkit.telemetry.test.pref2";
  const PREF_TEST_4 = "toolkit.telemetry.test.pref_old";

  const expectedValue = "some-test-value";
  gNow = futureDate(gNow, 10 * MILLISECONDS_PER_MINUTE);
  fakeNow(gNow);

  const PREFS_TO_WATCH = new Map([
    [PREF_TEST_1, TelemetryEnvironment.RECORD_PREF_VALUE],
    [PREF_TEST_2, TelemetryEnvironment.RECORD_PREF_STATE],
    [PREF_TEST_3, TelemetryEnvironment.RECORD_PREF_STATE],
    [PREF_TEST_4, TelemetryEnvironment.RECORD_PREF_VALUE],
  ]);

  Preferences.set(PREF_TEST_4, expectedValue);

  
  TelemetryEnvironment._watchPreferences(PREFS_TO_WATCH);
  let deferred = PromiseUtils.defer();

  
  Assert.strictEqual(TelemetryEnvironment.currentEnvironment.settings.userPrefs[PREF_TEST_1], undefined);
  Assert.strictEqual(TelemetryEnvironment.currentEnvironment.settings.userPrefs[PREF_TEST_4], expectedValue);

  TelemetryEnvironment.registerChangeListener("testWatchPrefs",
    (reason, data) => deferred.resolve(data));
  let oldEnvironmentData = TelemetryEnvironment.currentEnvironment;

  
  Preferences.set(PREF_TEST_1, expectedValue);
  Preferences.set(PREF_TEST_2, false);
  let eventEnvironmentData = yield deferred.promise;

  
  TelemetryEnvironment.unregisterChangeListener("testWatchPrefs");

  
  Assert.deepEqual(oldEnvironmentData, eventEnvironmentData);
  let userPrefs = TelemetryEnvironment.currentEnvironment.settings.userPrefs;

  Assert.equal(userPrefs[PREF_TEST_1], expectedValue,
               "Environment contains the correct preference value.");
  Assert.equal(userPrefs[PREF_TEST_2], "<user-set>",
               "Report that the pref was user set but the value is not shown.");
  Assert.ok(!(PREF_TEST_3 in userPrefs),
            "Do not report if preference not user set.");
});

add_task(function* test_prefWatch_prefReset() {
  const PREF_TEST = "toolkit.telemetry.test.pref1";
  const PREFS_TO_WATCH = new Map([
    [PREF_TEST, TelemetryEnvironment.RECORD_PREF_STATE],
  ]);

  
  Preferences.set(PREF_TEST, false);

  gNow = futureDate(gNow, 10 * MILLISECONDS_PER_MINUTE);
  fakeNow(gNow);

  
  TelemetryEnvironment._watchPreferences(PREFS_TO_WATCH);
  let deferred = PromiseUtils.defer();
  TelemetryEnvironment.registerChangeListener("testWatchPrefs_reset", deferred.resolve);

  Assert.strictEqual(TelemetryEnvironment.currentEnvironment.settings.userPrefs[PREF_TEST], "<user-set>");

  
  Preferences.reset(PREF_TEST);
  yield deferred.promise;

  Assert.strictEqual(TelemetryEnvironment.currentEnvironment.settings.userPrefs[PREF_TEST], undefined);

  
  TelemetryEnvironment.unregisterChangeListener("testWatchPrefs_reset");
});

add_task(function* test_addonsWatch_InterestingChange() {
  const ADDON_INSTALL_URL = gDataRoot + "restartless.xpi";
  const ADDON_ID = "tel-restartless-xpi@tests.mozilla.org";
  
  const EXPECTED_NOTIFICATIONS = 4;

  let deferred = PromiseUtils.defer();
  let receivedNotifications = 0;

  let registerCheckpointPromise = (aExpected) => {
    gNow = futureDate(gNow, 10 * MILLISECONDS_PER_MINUTE);
    fakeNow(gNow);
    return new Promise(resolve => TelemetryEnvironment.registerChangeListener(
      "testWatchAddons_Changes" + aExpected, (reason, data) => {
        Assert.equal(reason, "addons-changed");
        receivedNotifications++;
        resolve();
      }));
  };

  let assertCheckpoint = (aExpected) => {
    Assert.equal(receivedNotifications, aExpected);
    TelemetryEnvironment.unregisterChangeListener("testWatchAddons_Changes" + aExpected);
  };

  
  let checkpointPromise = registerCheckpointPromise(1);
  yield AddonTestUtils.installXPIFromURL(ADDON_INSTALL_URL);
  yield checkpointPromise;
  assertCheckpoint(1);
  Assert.ok(ADDON_ID in TelemetryEnvironment.currentEnvironment.addons.activeAddons);

  checkpointPromise = registerCheckpointPromise(2);
  let addon = yield AddonTestUtils.getAddonById(ADDON_ID);
  addon.userDisabled = true;
  yield checkpointPromise;
  assertCheckpoint(2);
  Assert.ok(!(ADDON_ID in TelemetryEnvironment.currentEnvironment.addons.activeAddons));

  checkpointPromise = registerCheckpointPromise(3);
  addon.userDisabled = false;
  yield checkpointPromise;
  assertCheckpoint(3);
  Assert.ok(ADDON_ID in TelemetryEnvironment.currentEnvironment.addons.activeAddons);

  checkpointPromise = registerCheckpointPromise(4);
  yield AddonTestUtils.uninstallAddonByID(ADDON_ID);
  yield checkpointPromise;
  assertCheckpoint(4);
  Assert.ok(!(ADDON_ID in TelemetryEnvironment.currentEnvironment.addons.activeAddons));

  Assert.equal(receivedNotifications, EXPECTED_NOTIFICATIONS,
               "We must only receive the notifications we expect.");
});

add_task(function* test_pluginsWatch_Add() {
  if (gIsAndroid) {
    Assert.ok(true, "Skipping: there is no Plugin Manager on Android.");
    return;
  }

  gNow = futureDate(gNow, 10 * MILLISECONDS_PER_MINUTE);
  fakeNow(gNow);

  Assert.equal(TelemetryEnvironment.currentEnvironment.addons.activePlugins.length, 1);

  let newPlugin = new PluginTag(PLUGIN2_NAME, PLUGIN2_DESC, PLUGIN2_VERSION, true);
  gInstalledPlugins.push(newPlugin);

  let deferred = PromiseUtils.defer();
  let receivedNotifications = 0;
  let callback = (reason, data) => {
    receivedNotifications++;
    Assert.equal(reason, "addons-changed");
    deferred.resolve();
  };
  TelemetryEnvironment.registerChangeListener("testWatchPlugins_Add", callback);

  Services.obs.notifyObservers(null, PLUGIN_UPDATED_TOPIC, null);
  yield deferred.promise;

  Assert.equal(TelemetryEnvironment.currentEnvironment.addons.activePlugins.length, 2);

  TelemetryEnvironment.unregisterChangeListener("testWatchPlugins_Add");

  Assert.equal(receivedNotifications, 1, "We must only receive one notification.");
});

add_task(function* test_pluginsWatch_Remove() {
  if (gIsAndroid) {
    Assert.ok(true, "Skipping: there is no Plugin Manager on Android.");
    return;
  }

  gNow = futureDate(gNow, 10 * MILLISECONDS_PER_MINUTE);
  fakeNow(gNow);

  
  let plugin = gInstalledPlugins.find(plugin => (plugin.name == PLUGIN2_NAME));
  Assert.ok(plugin, "The test plugin must exist.");

  
  gInstalledPlugins = gInstalledPlugins.filter(p => p != plugin);

  let deferred = PromiseUtils.defer();
  let receivedNotifications = 0;
  let callback = () => {
    receivedNotifications++;
    deferred.resolve();
  };
  TelemetryEnvironment.registerChangeListener("testWatchPlugins_Remove", callback);

  Services.obs.notifyObservers(null, PLUGIN_UPDATED_TOPIC, null);
  yield deferred.promise;

  TelemetryEnvironment.unregisterChangeListener("testWatchPlugins_Remove");

  Assert.equal(receivedNotifications, 1, "We must only receive one notification.");
});

add_task(function* test_addonsWatch_NotInterestingChange() {
  
  const DICTIONARY_ADDON_INSTALL_URL = gDataRoot + "dictionary.xpi";
  const INTERESTING_ADDON_INSTALL_URL = gDataRoot + "restartless.xpi";

  gNow = futureDate(gNow, 10 * MILLISECONDS_PER_MINUTE);
  fakeNow(gNow);

  let receivedNotification = false;
  let deferred = PromiseUtils.defer();
  TelemetryEnvironment.registerChangeListener("testNotInteresting",
    () => {
      Assert.ok(!receivedNotification, "Should not receive multiple notifications");
      receivedNotification = true;
      deferred.resolve();
    });

  yield AddonTestUtils.installXPIFromURL(DICTIONARY_ADDON_INSTALL_URL);
  yield AddonTestUtils.installXPIFromURL(INTERESTING_ADDON_INSTALL_URL);

  yield deferred.promise;
  Assert.ok(!("telemetry-dictionary@tests.mozilla.org" in
              TelemetryEnvironment.currentEnvironment.addons.activeAddons),
            "Dictionaries should not appear in active addons.");

  TelemetryEnvironment.unregisterChangeListener("testNotInteresting");
});

add_task(function* test_addonsAndPlugins() {
  const ADDON_INSTALL_URL = gDataRoot + "restartless.xpi";
  const ADDON_ID = "tel-restartless-xpi@tests.mozilla.org";
  const ADDON_INSTALL_DATE = truncateToDays(Date.now());
  const EXPECTED_ADDON_DATA = {
    blocklisted: false,
    description: "A restartless addon which gets enabled without a reboot.",
    name: "XPI Telemetry Restartless Test",
    userDisabled: false,
    appDisabled: false,
    version: "1.0",
    scope: 1,
    type: "extension",
    foreignInstall: false,
    hasBinaryComponents: false,
    installDay: ADDON_INSTALL_DATE,
    updateDay: ADDON_INSTALL_DATE,
    signedState: AddonManager.SIGNEDSTATE_MISSING,
  };

  const EXPECTED_PLUGIN_DATA = {
    name: FLASH_PLUGIN_NAME,
    version: FLASH_PLUGIN_VERSION,
    description: FLASH_PLUGIN_DESC,
    blocklisted: false,
    disabled: false,
    clicktoplay: true,
  };

  
  yield AddonTestUtils.installXPIFromURL(ADDON_INSTALL_URL);

  let data = TelemetryEnvironment.currentEnvironment;
  checkEnvironmentData(data);

  
  Assert.ok(ADDON_ID in data.addons.activeAddons, "We must have one active addon.");
  let targetAddon = data.addons.activeAddons[ADDON_ID];
  for (let f in EXPECTED_ADDON_DATA) {
    Assert.equal(targetAddon[f], EXPECTED_ADDON_DATA[f], f + " must have the correct value.");
  }

  
  let theme = data.addons.theme;
  Assert.equal(theme.id, (PERSONA_ID + PERSONA_ID_SUFFIX));
  Assert.equal(theme.name, PERSONA_NAME);
  Assert.equal(theme.description, PERSONA_DESCRIPTION);

  
  Assert.equal(data.addons.activePlugins.length, 1, "We must have only one active plugin.");
  let targetPlugin = data.addons.activePlugins[0];
  for (let f in EXPECTED_PLUGIN_DATA) {
    Assert.equal(targetPlugin[f], EXPECTED_PLUGIN_DATA[f], f + " must have the correct value.");
  }

  
  Assert.ok(targetPlugin.mimeTypes.find(m => m == PLUGIN_MIME_TYPE1));
  Assert.ok(targetPlugin.mimeTypes.find(m => m == PLUGIN_MIME_TYPE2));
  Assert.ok(!targetPlugin.mimeTypes.find(m => m == "Not There."));

  let personaId = (gIsGonk) ? null : PERSONA_ID;
  Assert.equal(data.addons.persona, personaId, "The correct Persona Id must be reported.");
});

add_task(function* test_signedAddon() {
  const ADDON_INSTALL_URL = gDataRoot + "signed.xpi";
  const ADDON_ID = "tel-signed-xpi@tests.mozilla.org";
  const ADDON_INSTALL_DATE = truncateToDays(Date.now());
  const EXPECTED_ADDON_DATA = {
    blocklisted: false,
    description: "A signed addon which gets enabled without a reboot.",
    name: "XPI Telemetry Signed Test",
    userDisabled: false,
    appDisabled: false,
    version: "1.0",
    scope: 1,
    type: "extension",
    foreignInstall: false,
    hasBinaryComponents: false,
    installDay: ADDON_INSTALL_DATE,
    updateDay: ADDON_INSTALL_DATE,
    signedState: AddonManager.SIGNEDSTATE_SIGNED,
  };

  
  gNow = fakeNow(futureDate(gNow, 10 * MILLISECONDS_PER_MINUTE));
  let deferred = PromiseUtils.defer();
  TelemetryEnvironment.registerChangeListener("test_signedAddon", deferred.resolve);

  
  yield AddonTestUtils.installXPIFromURL(ADDON_INSTALL_URL);

  yield deferred.promise;
  
  TelemetryEnvironment.unregisterChangeListener("test_signedAddon");

  let data = TelemetryEnvironment.currentEnvironment;
  checkEnvironmentData(data);

  
  Assert.ok(ADDON_ID in data.addons.activeAddons, "Add-on should be in the environment.");
  let targetAddon = data.addons.activeAddons[ADDON_ID];
  for (let f in EXPECTED_ADDON_DATA) {
    Assert.equal(targetAddon[f], EXPECTED_ADDON_DATA[f], f + " must have the correct value.");
  }
});

add_task(function* test_changeThrottling() {
  const PREF_TEST = "toolkit.telemetry.test.pref1";
  const PREFS_TO_WATCH = new Map([
    [PREF_TEST, TelemetryEnvironment.RECORD_PREF_STATE],
  ]);
  Preferences.reset(PREF_TEST);

  gNow = futureDate(gNow, 10 * MILLISECONDS_PER_MINUTE);
  fakeNow(gNow);

  
  TelemetryEnvironment._watchPreferences(PREFS_TO_WATCH);
  let deferred = PromiseUtils.defer();
  let changeCount = 0;
  TelemetryEnvironment.registerChangeListener("testWatchPrefs_throttling", () => {
    ++changeCount;
    deferred.resolve();
  });

  
  Preferences.set(PREF_TEST, 1);
  yield deferred.promise;
  Assert.equal(changeCount, 1);

  
  deferred = PromiseUtils.defer();
  gNow = futureDate(gNow, MILLISECONDS_PER_MINUTE);
  fakeNow(gNow);
  Preferences.set(PREF_TEST, 2);
  gNow = futureDate(gNow, 5 * MILLISECONDS_PER_MINUTE);
  fakeNow(gNow);
  Preferences.set(PREF_TEST, 3);
  yield deferred.promise;

  Assert.equal(changeCount, 2);

  
  TelemetryEnvironment.unregisterChangeListener("testWatchPrefs_throttling");
});

add_task(function* test_defaultSearchEngine() {
  
  
  let data = TelemetryEnvironment.currentEnvironment;
  checkEnvironmentData(data);
  Assert.ok(!("defaultSearchEngine" in data.settings));
  Assert.ok(!("defaultSearchEngineData" in data.settings));

  
  
  let defaultBranch = Services.prefs.getDefaultBranch(null);
  defaultBranch.setCharPref("browser.search.jarURIs", "chrome://testsearchplugin/locale/searchplugins/");
  defaultBranch.setBoolPref("browser.search.loadFromJars", true);

  
  
  Preferences.set("browser.search.geoip.url", "");
  yield new Promise(resolve => Services.search.init(resolve));

  
  
  data = TelemetryEnvironment.currentEnvironment;
  checkEnvironmentData(data);
  Assert.equal(data.settings.defaultSearchEngine, "telemetrySearchIdentifier");
  let expectedSearchEngineData = {
    name: "telemetrySearchIdentifier",
    loadPath: "jar:[other]/searchTest.jar!testsearchplugin/telemetrySearchIdentifier.xml",
    submissionURL: "http://ar.wikipedia.org/wiki/%D8%AE%D8%A7%D8%B5:%D8%A8%D8%AD%D8%AB?search=&sourceid=Mozilla-search"
  };
  Assert.deepEqual(data.settings.defaultSearchEngineData, expectedSearchEngineData);

  
  for (let engine of Services.search.getEngines()) {
    Services.search.removeEngine(engine);
  }
  
  
  
  Services.obs.notifyObservers(null, "browser-search-engine-modified", "engine-default");

  
  data = TelemetryEnvironment.currentEnvironment;
  checkEnvironmentData(data);
  Assert.equal(data.settings.defaultSearchEngine, "NONE");
  Assert.deepEqual(data.settings.defaultSearchEngineData, {name:"NONE"});

  
  const SEARCH_ENGINE_ID = "telemetry_default";
  const SEARCH_ENGINE_URL = "http://www.example.org/?search={searchTerms}";
  Services.search.addEngineWithDetails(SEARCH_ENGINE_ID, "", null, "", "get", SEARCH_ENGINE_URL);

  
  gNow = fakeNow(futureDate(gNow, 10 * MILLISECONDS_PER_MINUTE));
  
  let deferred = PromiseUtils.defer();
  TelemetryEnvironment.registerChangeListener("testWatch_SearchDefault", deferred.resolve);
  Services.search.defaultEngine = Services.search.getEngineByName(SEARCH_ENGINE_ID);
  yield deferred.promise;

  data = TelemetryEnvironment.currentEnvironment;
  checkEnvironmentData(data);

  const EXPECTED_SEARCH_ENGINE = "other-" + SEARCH_ENGINE_ID;
  Assert.equal(data.settings.defaultSearchEngine, EXPECTED_SEARCH_ENGINE);

  const EXPECTED_SEARCH_ENGINE_DATA = {
    name: "telemetry_default",
    loadPath: "[profile]/searchplugins/telemetrydefault.xml"
  };
  Assert.deepEqual(data.settings.defaultSearchEngineData, EXPECTED_SEARCH_ENGINE_DATA);
  TelemetryEnvironment.unregisterChangeListener("testWatch_SearchDefault");

  
  const PREF_TEST = "toolkit.telemetry.test.pref1";
  const PREFS_TO_WATCH = new Map([
    [PREF_TEST, TelemetryEnvironment.RECORD_PREF_STATE],
  ]);
  Preferences.reset(PREF_TEST);

  
  gNow = fakeNow(futureDate(gNow, 10 * MILLISECONDS_PER_MINUTE));
  
  TelemetryEnvironment._watchPreferences(PREFS_TO_WATCH);
  deferred = PromiseUtils.defer();
  TelemetryEnvironment.registerChangeListener("testSearchEngine_pref", deferred.resolve);
  
  Preferences.set(PREF_TEST, 1);
  yield deferred.promise;
  TelemetryEnvironment.unregisterChangeListener("testSearchEngine_pref");

  
  data = TelemetryEnvironment.currentEnvironment;
  checkEnvironmentData(data);
  Assert.equal(data.settings.defaultSearchEngine, EXPECTED_SEARCH_ENGINE);
});

add_task(function*() {
  do_test_finished();
});
