



let outOfProcess = __marionetteParams[0]
let mochitestUrl = __marionetteParams[1]

const CHILD_SCRIPT = "chrome://specialpowers/content/specialpowers.js";
const CHILD_SCRIPT_API = "chrome://specialpowers/content/specialpowersAPI.js";
const CHILD_LOGGER_SCRIPT = "chrome://specialpowers/content/MozillaLogger.js";

let homescreen = document.getElementById('systemapp');
let container = homescreen.contentWindow.document.getElementById('test-container');

function openWindow(aEvent) {
  var popupIframe = aEvent.detail.frameElement;
  popupIframe.setAttribute('style', 'position: absolute; left: 0; top: 300px; background: white; ');

  popupIframe.addEventListener('mozbrowserclose', function(e) {
    container.parentNode.removeChild(popupIframe);
    container.focus();
  });

  
  popupIframe.addEventListener('mozbrowseropenwindow', openWindow);

  popupIframe.addEventListener('mozbrowserloadstart', function(e) {
    popupIframe.focus();
  });

  container.parentNode.appendChild(popupIframe);
}
container.addEventListener('mozbrowseropenwindow', openWindow);

if (outOfProcess) {
  let specialpowers = {};
  let loader = Cc["@mozilla.org/moz/jssubscript-loader;1"].getService(Ci.mozIJSSubScriptLoader);
  loader.loadSubScript("chrome://specialpowers/content/SpecialPowersObserver.js", specialpowers);
  let specialPowersObserver = new specialpowers.SpecialPowersObserver();
  specialPowersObserver.init();

  let mm = container.QueryInterface(Ci.nsIFrameLoaderOwner).frameLoader.messageManager;
  mm.addMessageListener("SPPrefService", specialPowersObserver);
  mm.addMessageListener("SPProcessCrashService", specialPowersObserver);
  mm.addMessageListener("SPPingService", specialPowersObserver);
  mm.addMessageListener("SpecialPowers.Quit", specialPowersObserver);
  mm.addMessageListener("SpecialPowers.Focus", specialPowersObserver);
  mm.addMessageListener("SPPermissionManager", specialPowersObserver);

  mm.loadFrameScript(CHILD_LOGGER_SCRIPT, true);
  mm.loadFrameScript(CHILD_SCRIPT_API, true);
  mm.loadFrameScript(CHILD_SCRIPT, true);
  
  mm.loadFrameScript('data:,addEventListener%28%22DOMWindowCreated%22%2C%20function%28e%29%20%7B%0A%20%20removeEventListener%28%22DOMWindowCreated%22%2C%20arguments.callee%2C%20false%29%3B%0A%20%20var%20window%20%3D%20e.target.defaultView%3B%0A%20%20window.wrappedJSObject.SpecialPowers.addPermission%28%22allowXULXBL%22%2C%20true%2C%20window.document%29%3B%0A%7D%0A%29%3B', true);

  specialPowersObserver._isFrameScriptLoaded = true;
}

container.src = mochitestUrl;
