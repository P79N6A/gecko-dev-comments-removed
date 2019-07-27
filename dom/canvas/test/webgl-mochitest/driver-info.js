DriverInfo = (function() {
  
  

  function defaultInfoFunc(str) {
    console.log('Info: ' + str);
  }

  var gInfoFunc = defaultInfoFunc;
  function setInfoFunc(func) {
    gInfoFunc = func;
  }

  function info(str) {
    gInfoFunc(str);
  }

  
  
  
  function detectDriverInfo() {
    try {
      var cc = SpecialPowers.Cc;
    } catch (e) {
      throw 'No SpecialPowers!';
    }

    const Cc = SpecialPowers.Cc;
    const Ci = SpecialPowers.Ci;
    var doc = Cc["@mozilla.org/xmlextras/domparser;1"].createInstance(Ci.nsIDOMParser).parseFromString("<html/>", "text/html");

    var canvas = doc.createElement("canvas");
    canvas.width = 1;
    canvas.height = 1;

    var type = "";
    var gl = null;
    try {
      gl = canvas.getContext("experimental-webgl");
    } catch(e) {}

    if (!gl) {
      info('Failed to create WebGL context for querying driver info.');
      throw 'WebGL failed';
    }

    var ext = gl.getExtension("WEBGL_debug_renderer_info");
    

    var webglRenderer = gl.getParameter(ext.UNMASKED_RENDERER_WEBGL);
    var webglVendor = gl.getParameter(ext.UNMASKED_VENDOR_WEBGL);
    return [webglVendor, webglRenderer];
  }

  function detectOSInfo() {
    try {
      var cc = SpecialPowers.Cc;
    } catch (e) {
      throw 'No SpecialPowers!';
    }

    const Cc = SpecialPowers.Cc;
    const Ci = SpecialPowers.Ci;

    
    var runtime = Cc['@mozilla.org/xre/app-info;1'].getService(Ci.nsIXULRuntime);

    var os = null;
    var version = null;
    if (navigator.platform.indexOf('Win') == 0) {
      os = OS.WINDOWS;

      
      version = SpecialPowers.Services.sysinfo.getProperty('version');
      version = parseFloat(version);
      

    } else if (navigator.platform.indexOf('Mac') == 0) {
      os = OS.MAC;

      var versionMatch = /Mac OS X (\d+.\d+)/.exec(navigator.userAgent);
      version = versionMatch ? parseFloat(versionMatch[1]) : null;

    } else if (runtime.widgetToolkit == 'gonk') {
      os = OS.B2G;

    } else if (navigator.appVersion.indexOf('Android') != -1) {
      os = OS.ANDROID;
      
      version = SpecialPowers.Services.sysinfo.getProperty('version');

    } else if (navigator.platform.indexOf('Linux') == 0) {
      
      os = OS.LINUX;
    }

    return [os, version];
  }

  var OS = {
    WINDOWS: 'windows',
    MAC: 'mac',
    LINUX: 'linux',
    ANDROID: 'android',
    B2G: 'b2g',
  };

  var DRIVER = {
    MESA: 'mesa',
    NVIDIA: 'nvidia',
    ANDROID_X86_EMULATOR: 'android x86 emulator',
    ANGLE: 'angle',
  };

  var kOS = null;
  var kOSVersion = null;
  var kDriver = null;

  try {
    [kOS, kOSVersion] = detectOSInfo();
  } catch (e) {
    
  }

  try {
    var glVendor, glRenderer;
    [glVendor, glRenderer] = detectDriverInfo();
    info('GL vendor: ' + glVendor);
    info('GL renderer: ' + glRenderer);

    if (glRenderer.includes('llvmpipe')) {
      kDriver = DRIVER.MESA;
    } else if (glRenderer.includes('Android Emulator')) {
      kDriver = DRIVER.ANDROID_X86_EMULATOR;
    } else if (glRenderer.includes('ANGLE')) {
      kDriver = DRIVER.ANGLE;
    } else if (glVendor.includes('NVIDIA')) {
      kDriver = DRIVER.NVIDIA;
    }
  } catch (e) {
    
  }

  if (kOS) {
    info('OS detected as: ' + kOS);
    info('  Version: ' + kOSVersion);
  } else {
    info('OS not detected.');
    info('  `platform`:   ' + navigator.platform);
    info('  `appVersion`: ' + navigator.appVersion);
    info('  `userAgent`:  ' + navigator.userAgent);
  }
  if (kDriver) {
    info('GL driver detected as: ' + kDriver);
  } else {
    info('GL driver not detected.');
  }

  return {
    setInfoFunc: setInfoFunc,

    OS: OS,
    DRIVER: DRIVER,
    getOS: function() { return kOS; },
    getDriver: function() { return kDriver; },
    getOSVersion: function() { return kOSVersion; },
  };
})();

