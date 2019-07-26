







const TEST_CONTENT_HELPER = "chrome://mochitests/content/browser/browser/base/content/test/social/social_crash_content_helper.js";

let {getFrameWorkerHandle} = Cu.import("resource://gre/modules/FrameWorker.jsm", {});

function test() {
  waitForExplicitFinish();

  Services.prefs.setBoolPref("social.allowMultipleWorkers", true);
  
  Services.prefs.setIntPref("dom.ipc.processCount", 1);

  runSocialTestWithProvider(gProviders, function (finishcb) {
    Social.enabled = true;
    runSocialTests(tests, undefined, undefined, function() {
      Services.prefs.clearUserPref("dom.ipc.processCount");
      Services.prefs.clearUserPref("social.sidebar.open");
      Services.prefs.clearUserPref("social.allowMultipleWorkers");
      finishcb();
    });
  });
}

let gProviders = [
  {
    name: "provider 1",
    origin: "https://example.com",
    sidebarURL: "https://example.com/browser/browser/base/content/test/social/social_sidebar.html?provider1",
    workerURL: "https://example.com/browser/browser/base/content/test/social/social_worker.js",
    iconURL: "chrome://branding/content/icon48.png"
  },
  {
    name: "provider 2",
    origin: "https://test1.example.com",
    sidebarURL: "https://test1.example.com/browser/browser/base/content/test/social/social_sidebar.html?provider2",
    workerURL: "https://test1.example.com/browser/browser/base/content/test/social/social_worker.js",
    iconURL: "chrome://branding/content/icon48.png"
  }
];

var tests = {
  testCrash: function(next) {
    
    let sbrowser = document.getElementById("social-sidebar-browser");
    onSidebarLoad(function() {
      
      let fw = getFrameWorkerHandle(gProviders[0].workerURL);
      fw.port.close();
      fw._worker.browserPromise.then(browser => {
        let mm = browser.messageManager;
        mm.loadFrameScript(TEST_CONTENT_HELPER, false);
        
        
        let observer = new crashObserver(function() {
          info("Saw the process crash.")
          Services.obs.removeObserver(observer, 'ipc:content-shutdown');
          
          onSidebarLoad(function() {
            ok(sbrowser.contentDocument.location.href.indexOf("about:socialerror?")==0, "is on social error page");
            
            onSidebarLoad(function() {
              
              ensureWorkerLoaded(gProviders[0], function() {
                ensureWorkerLoaded(gProviders[1], function() {
                  
                  next();
                });
              });
            });
            
            sbrowser.contentDocument.getElementById("btnTryAgain").click();
          });
        });
        Services.obs.addObserver(observer, 'ipc:content-shutdown', false);
        
        mm.sendAsyncMessage("social-test:crash");
      });
    })
    Services.prefs.setBoolPref("social.sidebar.open", true);
  },
}

function onSidebarLoad(callback) {
  let sbrowser = document.getElementById("social-sidebar-browser");
  sbrowser.addEventListener("load", function load() {
    sbrowser.removeEventListener("load", load, true);
    callback();
  }, true);
}

function ensureWorkerLoaded(manifest, callback) {
  let fw = getFrameWorkerHandle(manifest.workerURL);
  
  let port = fw.port;
  port.onmessage = function(msg) {
    if (msg.data.topic == "pong") {
      port.close();
      callback();
    }
  }
  port.postMessage({topic: "ping"})
}






let crashObserver = function(callback) {
  this.callback = callback;
}
crashObserver.prototype = {
  observe: function(subject, topic, data) {
    is(topic, 'ipc:content-shutdown', 'Received correct observer topic.');
    ok(subject instanceof Components.interfaces.nsIPropertyBag2,
       'Subject implements nsIPropertyBag2.');
    
    
    if (!subject.hasKey("abnormal")) {
      info("This is a normal termination and isn't the one we are looking for...");
      return;
    }

    var dumpID;
    if ('nsICrashReporter' in Components.interfaces) {
      dumpID = subject.getPropertyAsAString('dumpID');
      ok(dumpID, "dumpID is present and not an empty string");
    }

    if (dumpID) {
      var minidumpDirectory = getMinidumpDirectory();
      removeFile(minidumpDirectory, dumpID + '.dmp');
      removeFile(minidumpDirectory, dumpID + '.extra');
    }
    this.callback();
  }
}

function getMinidumpDirectory() {
  var dir = Services.dirsvc.get('ProfD', Components.interfaces.nsIFile);
  dir.append("minidumps");
  return dir;
}
function removeFile(directory, filename) {
  var file = directory.clone();
  file.append(filename);
  if (file.exists()) {
    file.remove(false);
  }
}
