



let outOfProcess = __marionetteParams[0]
let mochitestUrl = __marionetteParams[1]
let onDevice = __marionetteParams[2]
let wifiSettings = __marionetteParams[3]
let chrome = __marionetteParams[4]
let prefs = Components.classes["@mozilla.org/preferences-service;1"].
                            getService(Components.interfaces.nsIPrefBranch)
let settings = window.navigator.mozSettings;
let cm = Components.classes["@mozilla.org/categorymanager;1"].
                    getService(Components.interfaces.nsICategoryManager);

if (wifiSettings)
  wifiSettings = JSON.parse(wifiSettings);

const CHILD_SCRIPT = "chrome://specialpowers/content/specialpowers.js";
const CHILD_SCRIPT_API = "chrome://specialpowers/content/specialpowersAPI.js";
const CHILD_LOGGER_SCRIPT = "chrome://specialpowers/content/MozillaLogger.js";

let homescreen = document.getElementById('systemapp');
let container = homescreen.contentWindow.document.getElementById('test-container');


if (cm) {
  cm.deleteCategoryEntry("update-timer", "WebappsUpdateTimer", false);
  cm.deleteCategoryEntry("update-timer", "nsUpdateService", false);
}

function openWindow(aEvent) {
  var popupIframe = aEvent.detail.frameElement;
  popupIframe.id = 'popupiframe';

  
  
  if (aEvent.detail.features.indexOf('width') != -1) {
    let width = aEvent.detail.features.substr(aEvent.detail.features.indexOf('width')+6);
    width = width.substr(0,width.indexOf(',') == -1 ? width.length : width.indexOf(','));
    popupIframe.setAttribute('width', width);
  }
  if (aEvent.detail.features.indexOf('height') != -1) {
    let height = aEvent.detail.features.substr(aEvent.detail.features.indexOf('height')+7);
    height = height.substr(0, height.indexOf(',') == -1 ? height.length : height.indexOf(','));
    popupIframe.setAttribute('height', height);
  }

  popupIframe.addEventListener('mozbrowserclose', function(e) {
    container.parentNode.removeChild(popupIframe);
    container.focus();
  });

  
  popupIframe.addEventListener('mozbrowseropenwindow', openWindow);

  popupIframe.addEventListener('mozbrowserloadstart', function(e) {
    popupIframe.focus();
    let mm = popupIframe.QueryInterface(Ci.nsIFrameLoaderOwner).frameLoader.messageManager;
    mm.loadFrameScript(CHILD_LOGGER_SCRIPT, true);
    mm.loadFrameScript(CHILD_SCRIPT_API, true);
    mm.loadFrameScript(CHILD_SCRIPT, true);
    mm.loadFrameScript('data:,attachSpecialPowersToWindow(content);', true);
  });

  container.parentNode.appendChild(popupIframe);
}
container.addEventListener('mozbrowseropenwindow', openWindow);
container.addEventListener('mozbrowsershowmodalprompt', function (e) {
  if (e.detail.message == 'setVisible::false') {
    container.setVisible(false);
  }
  else if (e.detail.message == 'setVisible::true') {
    container.setVisible(true);
  }
});

if (outOfProcess) {
  let specialpowers = {};
  let loader = Cc["@mozilla.org/moz/jssubscript-loader;1"].getService(Ci.mozIJSSubScriptLoader);
  loader.loadSubScript("chrome://specialpowers/content/SpecialPowersObserver.js", specialpowers);
  let specialPowersObserver = new specialpowers.SpecialPowersObserver();

  let mm = container.QueryInterface(Ci.nsIFrameLoaderOwner).frameLoader.messageManager;
  specialPowersObserver.init(mm);

  
  function contentScript() {
    addEventListener("DOMWindowCreated", function listener(e) {
      removeEventListener("DOMWindowCreated", listener, false);
      var window = e.target.defaultView;
      window.wrappedJSObject.SpecialPowers.addPermission("allowXULXBL", true, window.document);
    });
  }
  mm.loadFrameScript("data:,(" + encodeURI(contentScript.toSource()) + ")();", true);
}

if (chrome) {
  let loader = Cc["@mozilla.org/moz/jssubscript-loader;1"].getService(Ci.mozIJSSubScriptLoader);
  loader.loadSubScript("chrome://mochikit/content/browser-test.js");
  b2gStart();
}

if (onDevice) {
  var cpuLock = Cc["@mozilla.org/power/powermanagerservice;1"]
                      .getService(Ci.nsIPowerManagerService)
                      .newWakeLock("cpu");

  let manager = navigator.mozWifiManager;
  let con = manager.connection;
  manager.setPowerSavingMode(false);

  manager.onenabled = function () {
    if(wifiSettings) {
      var req = manager.getKnownNetworks();
      req.onsuccess = function () {
        var networks = req.result;
        for (var i = 0; i < networks.length; ++i){
          var network = networks[i];
          if(network.ssid == wifiSettings.ssid) {
            manager.forget(network);
          }
        }
        manager.associate(new window.MozWifiNetwork(wifiSettings));
      };
    }
  };

  manager.onstatuschange = function (event) {
    prefs.setIntPref("network.proxy.type", 2);
    if (event.status == 'connected') {
      container.src = mochitestUrl;
    }
  };

  if(wifiSettings) {
    var req = settings.createLock().set({
       'wifi.enabled': false,
       'wifi.suspended': false
     });

    req.onsuccess = function () {
      dump("----------------------enabling wifi------------------\n");
      var req1 = settings.createLock().set({
          'wifi.enabled': true,
          'wifi.suspended': false});
    };
  }
} else {
  if (!chrome) {
    container.src = mochitestUrl;
  }
}
