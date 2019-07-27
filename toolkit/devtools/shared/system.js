


"use strict";

const { Cc, Ci, Cu } = require("chrome");
const { Task } = require("resource://gre/modules/Task.jsm");

loader.lazyRequireGetter(this, "Services");
loader.lazyRequireGetter(this, "promise");
loader.lazyRequireGetter(this, "OS", "resource://gre/modules/commonjs/node/os");
loader.lazyRequireGetter(this, "DebuggerServer", "devtools/server/main", true);
loader.lazyRequireGetter(this, "AppConstants",
  "resource://gre/modules/AppConstants.jsm", true);
loader.lazyGetter(this, "screenManager", () => {
  return Cc["@mozilla.org/gfx/screenmanager;1"].getService(Ci.nsIScreenManager);
});
loader.lazyGetter(this, "oscpu", () => {
  return Cc["@mozilla.org/network/protocol;1?name=http"]
           .getService(Ci.nsIHttpProtocolHandler).oscpu;
});

const APP_MAP = {
  "{ec8030f7-c20a-464f-9b0e-13a3a9e97384}": "firefox",
  "{3550f703-e582-4d05-9a08-453d09bdfdc6}": "thunderbird",
  "{92650c4d-4b8e-4d2a-b7eb-24ecf4f6b63a}": "seamonkey",
  "{718e30fb-e89b-41dd-9da7-e25a45638b28}": "sunbird",
  "{3c2e2abc-06d4-11e1-ac3b-374f68613e61}": "b2g",
  "{aa3c5121-dab2-40e2-81ca-7ea25febc110}": "mobile/android",
  "{a23983c0-fd0e-11dc-95ff-0800200c9a66}": "mobile/xul"
};

let CACHED_INFO = null;

function *getSystemInfo() {
  if (CACHED_INFO) {
    return CACHED_INFO;
  }

  let appInfo = Services.appinfo;
  let win = Services.wm.getMostRecentWindow(DebuggerServer.chromeWindowType);
  let [processor, compiler] = appInfo.XPCOMABI.split("-");
  let dpi, useragent, width, height, os, hardware, version, brandName;
  let appid = appInfo.ID;
  let apptype = APP_MAP[appid];
  let geckoVersion = appInfo.platformVersion;

  
  if (apptype === "b2g") {
    os = "B2G";
    hardware = yield exports.getSetting("deviceinfo.hardware");
    version = yield exports.getSetting("deviceinfo.os");
  }
  
  else {
    os = appInfo.OS;
    version = appInfo.version;
    hardware = "unknown";
  }

  let bundle = Services.strings.createBundle("chrome://branding/locale/brand.properties");
  if (bundle) {
    brandName = bundle.GetStringFromName("brandFullName");
  } else {
    brandName = null;
  }

  if (win) {
    let utils = win.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils);
    dpi = utils.displayDPI;
    useragent = win.navigator.userAgent;
    width = win.screen.width;
    height = win.screen.height;
  }

  let info = {

    




    
    appid,

    
    apptype,

    
    vendor: appInfo.vendor,

    
    name: appInfo.name,

    
    
    
    
    version,

    
    appbuildid: appInfo.appBuildID,

    
    changeset: exports.getAppIniString("App", "SourceStamp"),

    
    platformbuildid: appInfo.platformBuildID,
    geckobuildid: appInfo.platformBuildID,

    
    
    
    platformversion: geckoVersion,
    geckoversion: geckoVersion,

    
    locale: Cc["@mozilla.org/chrome/chrome-registry;1"].getService(Ci.nsIXULChromeRegistry).getSelectedLocale("global"),

    



    
    endianness: OS.endianness(),

    
    hostname: OS.hostname(),

    
    
    
    os,
    platform: os,

    
    
    hardware,

    
    
    
    arch: processor,
    processor,

    
    
    compiler,

    
    profile: getProfileLocation(),

    
    channel: AppConstants.MOZ_UPDATE_CHANNEL,

    dpi,
    useragent,
    width,
    height,
    brandName,
  };

  CACHED_INFO = info;
  return info;
}

function getProfileLocation() {
  let profd = Services.dirsvc.get("ProfD", Ci.nsILocalFile);
  let profservice = Cc["@mozilla.org/toolkit/profile-service;1"].getService(Ci.nsIToolkitProfileService);
  var profiles = profservice.profiles;
  while (profiles.hasMoreElements()) {
    let profile = profiles.getNext().QueryInterface(Ci.nsIToolkitProfile);
    if (profile.rootDir.path == profd.path) {
      return profile = profile.name;
    }
  }

  return profd.leafName;
}

function getAppIniString(section, key) {
  let inifile = Services.dirsvc.get("GreD", Ci.nsIFile);
  inifile.append("application.ini");

  if (!inifile.exists()) {
    inifile = Services.dirsvc.get("CurProcD", Ci.nsIFile);
    inifile.append("application.ini");
  }

  if (!inifile.exists()) {
    return undefined;
  }

  let iniParser = Cc["@mozilla.org/xpcom/ini-parser-factory;1"].getService(Ci.nsIINIParserFactory).createINIParser(inifile);
  try {
    return iniParser.getString(section, key);
  } catch (e) {
    return undefined;
  }
}





function getScreenDimensions() {
  let width = {};
  let height = {};

  screenManager.primaryScreen.GetRect({}, {}, width, height);
  let dims = width.value + "x" + height.value;

  if (width.value < 800 || height.value < 600) {
    return 0;
  }
  if (dims === "800x600") {
    return 1;
  }
  if (dims === "1024x768") {
    return 2;
  }
  if (dims === "1280x800") {
    return 3;
  }
  if (dims === "1280x1024") {
    return 4;
  }
  if (dims === "1366x768") {
    return 5;
  }
  if (dims === "1440x900") {
    return 6;
  }
  if (dims === "1920x1080") {
    return 7;
  }
  if (dims === "2560×1440") {
    return 8;
  }
  if (dims === "2560×1600") {
    return 9;
  }
  if (dims === "2880x1800") {
    return 10;
  }
  if (width.value > 2880 || height.value > 1800) {
    return 12;
  }

  
  return 11;
}





function getOSCPU() {
  if (oscpu.includes("NT 5.1") || oscpu.includes("NT 5.2")) {
    return 0;
  }
  if (oscpu.includes("NT 6.0")) {
    return 1;
  }
  if (oscpu.includes("NT 6.1")) {
    return 2;
  }
  if (oscpu.includes("NT 6.2")) {
    return 3;
  }
  if (oscpu.includes("NT 6.3")) {
    return 4;
  }
  if (oscpu.includes("OS X")) {
    return 5;
  }
  if (oscpu.includes("Linux")) {
    return 6;
  }

  
  return 12;
}

function getSetting(name) {
  let deferred = promise.defer();

  if ("@mozilla.org/settingsService;1" in Cc) {
    let settingsService = Cc["@mozilla.org/settingsService;1"].getService(Ci.nsISettingsService);
    let req = settingsService.createLock().get(name, {
      handle: (name, value) => deferred.resolve(value),
      handleError: (error) => deferred.reject(error),
    });
  } else {
    deferred.reject(new Error("No settings service"));
  }
  return deferred.promise;
}

exports.is64Bit = Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsIXULAppInfo).is64Bit;
exports.getSystemInfo = Task.async(getSystemInfo);
exports.getAppIniString = getAppIniString;
exports.getSetting = getSetting;
exports.getScreenDimensions = getScreenDimensions;
exports.getOSCPU = getOSCPU;
exports.constants = AppConstants;
