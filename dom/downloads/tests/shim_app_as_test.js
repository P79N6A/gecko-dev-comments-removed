










































  var gManifestURL;
  var gApp;
  var gOptions;

  
  var gChromeHelper = SpecialPowers.loadChromeScript(
                        SimpleTest.getTestFileURL('shim_app_as_test_chrome.js'));

  function installApp() {
    info("installing app");
    var useOrigin = document.location.origin;
    gChromeHelper.sendAsyncMessage(
      'install',
      {
        origin: useOrigin,
        manifestURL: SimpleTest.getTestFileURL(gOptions.appManifest),
      });
  }

  function installedApp(appInfo) {
    gApp = appInfo;
    ok(!!appInfo, 'installed app');
    runTests();
  }
  gChromeHelper.addMessageListener('installed', installedApp);

  function uninstallApp() {
    info('uninstalling app');
    gChromeHelper.sendAsyncMessage('uninstall', gApp);
  }

  function uninstalledApp(success) {
    ok(success, 'uninstalled app');
    runTests();
  }
  gChromeHelper.addMessageListener('uninstalled', uninstalledApp);

  function testApp() {
    var cleanupFrame;
    var handleTestMessage = function(message) {
      if (/^OK/.exec(message)) {
        ok(true, "Message from app: " + message);
      } else if (/^KO/.exec(message)) {
        ok(false, "Message from app: " + message);
      } else if (/^INFO/.exec(message)) {
        info("Message from app: " + message.substring(5));
      } else if (/^DONE$/.exec(message)) {
        ok(true, "Messaging from app complete");
        cleanupFrame();
        runTests();
      }
    };

    
    
    
    
    
    var needSiblingIframeHack = true;

    if (needSiblingIframeHack) {
      gChromeHelper.sendAsyncMessage('run', gApp);

      gChromeHelper.addMessageListener('appMessage', handleTestMessage);
      gChromeHelper.addMessageListener('appError', function(data) {
        ok(false, "Error in app frame: " + data.message);
      });

      cleanupFrame = function() {
        gChromeHelper.sendAsyncMessage('close', {});
      };
    } else {
      var ifr = document.createElement('iframe');
      ifr.setAttribute('mozbrowser', 'true');
      ifr.setAttribute('mozapp', gApp.manifestURL);

      cleanupFrame = function() {
        ifr.removeEventListener('mozbrowsershowmodalprompt', listener);
        domParent.removeChild(ifr);
      };

      
      var listener = function(e) {
        var message = e.detail.message; 
        handleTestMessage(message);
      };

      
      ifr.addEventListener('mozbrowsershowmodalprompt', listener, false);
      ifr.addEventListener('mozbrowsererror', function(evt) {
        ok(false, "Error in app frame: " + evt.detail);
      });

      ifr.setAttribute('src', gApp.manifest.launch_path);
      var domParent = document.getElementById('content');
      if (!domParent) {
        document.createElement('div');
        document.body.insertBefore(domParent, document.body.firstChild);
      }
      domParent.appendChild(ifr);
    }
  }

  var tests = [
    
    function() {
      info("pushing permissions");
      SpecialPowers.pushPermissions(
        [{ "type": "browser", "allow": 1, "context": document },
         { "type": "embed-apps", "allow": 1, "context": document },
         { "type": "webapps-manage", "allow": 1, "context": document }
        ],
        runTests);
    },

    
    function() {
      info("pushing preferences: " + gOptions.extraPrefs.set);
      SpecialPowers.pushPrefEnv({
        "set": gOptions.extraPrefs.set
      }, runTests);
    },

    function() {
      info("enabling use of mozbrowser");
      
      SpecialPowers.setBoolPref("dom.mozBrowserFramesEnabled", true);
      runTests();
    },

    
    function() {
      SpecialPowers.autoConfirmAppInstall(function() {
        SpecialPowers.autoConfirmAppUninstall(runTests);
      });
    },

    
    installApp,

    
    testApp,

    
    uninstallApp,
  ];

  function runTests() {
    if (!tests.length) {
      ok(true, 'DONE!');
      SimpleTest.finish();
      return;
    }

    var test = tests.shift();
    test();
  }

  SimpleTest.waitForExplicitFinish();

  function runAppTest(options) {
    gOptions = options;
    var href = document.location.href;
    gManifestURL = href.substring(0, href.lastIndexOf('/') + 1) +
      options.appManifest;
    runTests();
  }
