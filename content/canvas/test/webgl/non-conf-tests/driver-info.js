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
    const Cc = SpecialPowers.Cc;
    const Ci = SpecialPowers.Ci;
    var doc = Cc["@mozilla.org/xmlextras/domparser;1"].createInstance(Ci.nsIDOMParser).parseFromString("<html/>", "text/html");

    var canvas = doc.createElement("canvas");
    canvas.width = 1;
    canvas.height = 1;

    var type = "";
    var gl;
    try {
      gl = canvas.getContext("experimental-webgl");
    } catch(e) {
      ok(false, "Failed to create a WebGL context for getting driver info.");
      return ["", ""]
    }
    var ext = gl.getExtension("WEBGL_debug_renderer_info");
    
    
    var webglRenderer = gl.getParameter(ext.UNMASKED_RENDERER_WEBGL);
    var webglVendor = gl.getParameter(ext.UNMASKED_VENDOR_WEBGL);
    return [webglVendor, webglRenderer];
  }
  
  var OS = {
    WINDOWS: 'windows',
    MAC: 'mac',
    LINUX: 'linux',
    ANDROID: 'android',
  };
  
  var DRIVER = {
    MESA: 'mesa',
    NVIDIA: 'nvidia',
    ANDROID_X86_EMULATOR: 'android x86 emulator',
  };

  var kOS = null;
  var kOSVersion = null;
  var kDriver = null;

  if (navigator.platform.indexOf('Win') == 0) {
    kOS = OS.WINDOWS;
    
    
    var version = SpecialPowers.Services.sysinfo.getProperty('version');
    kOSVersion = parseFloat(version);
    
    
  } else if (navigator.platform.indexOf('Mac') == 0) {
    kOS = OS.MAC;
    
    var versionMatch = /Mac OS X (\d+.\d+)/.exec(navigator.userAgent);
    kOSVersion = versionMatch ? parseFloat(versionMatch[1]) : null;
    
  } else if (navigator.appVersion.indexOf('Android') != -1) {
    kOS = OS.ANDROID;
    
    kOSVersion = SpecialPowers.Services.sysinfo.getProperty('version');
    
  } else if (navigator.platform.indexOf('Linux') == 0) {
    
    kOS = OS.LINUX;
  }
  
  var glVendor, glRenderer;
  [glVendor, glRenderer] = detectDriverInfo();
  info('GL vendor: ' + glVendor);
  info('GL renderer: ' + glRenderer);
  
  if (glRenderer.contains('llvmpipe')) {
    kDriver = DRIVER.MESA;
  } else if (glRenderer.contains('Android Emulator')) {
    kGLDriver = DRIVER.ANDROID_X86_EMULATOR;
  } else if (glVendor.contains('NVIDIA')) {
    kDriver = DRIVER.NVIDIA;
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
    
