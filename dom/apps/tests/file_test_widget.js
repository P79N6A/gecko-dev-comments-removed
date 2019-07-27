var gWidgetManifestURL = 'http://test/tests/dom/apps/tests/file_app.sjs?apptype=widget&getmanifest=true';
var gInvalidWidgetManifestURL = 'http://test/tests/dom/apps/tests/file_app.sjs?apptype=invalidWidget&getmanifest=true';
var gApp;

function onError() {
  ok(false, "Error callback invoked");
  finish();
}

function installApp(path) {
  var request = navigator.mozApps.install(path);
  request.onerror = onError;
  request.onsuccess = function() {
    gApp = request.result;

    runTest();
  }
}

function uninstallApp() {
  
  var request = navigator.mozApps.mgmt.uninstall(gApp);
  request.onerror = onError;
  request.onsuccess = function() {
    
    info("All done");

    runTest();
  }
}

function testApp(isValidWidget) {
  info("Test widget feature. IsValidWidget: " + isValidWidget);

  var ifr = document.createElement('iframe');
  ifr.setAttribute('mozbrowser', 'true');
  ifr.setAttribute('mozwidget', gApp.manifestURL);
  ifr.setAttribute('src', gApp.origin+gApp.manifest.launch_path);

  var domParent = document.getElementById('container');
  domParent.appendChild(ifr);

  var mm = SpecialPowers.getBrowserFrameMessageManager(ifr);
  mm.addMessageListener('OK', function(msg) {
    ok(isValidWidget, "Message from widget: " + SpecialPowers.wrap(msg).json);
  });
  mm.addMessageListener('KO', function(msg) {
    ok(!isValidWidget, "Message from widget: " + SpecialPowers.wrap(msg).json);
  });
  mm.addMessageListener('DONE', function(msg) {
    ok(true, "Message from widget complete: "+SpecialPowers.wrap(msg).json);
    domParent.removeChild(ifr);
    runTest();
  });

  ifr.addEventListener('mozbrowserloadend', function() {
    ok(true, "receive mozbrowserloadend");

    
    if (isValidWidget) {
      testLimitedBrowserAPI(ifr);
    }
    SimpleTest.executeSoon(()=>loadFrameScript(mm));
  }, false);

  
  if (!isValidWidget) {
    return;
  }

  [
    'mozbrowsertitlechange',
    'mozbrowseropenwindow',
    'mozbrowserscroll',
    'mozbrowserasyncscroll'
  ].forEach( function(topic) {
    ifr.addEventListener(topic, function() {
      ok(false, topic + " should be hidden");
    }, false);
  });
}

function testLimitedBrowserAPI(ifr) {
  var securitySensitiveCalls = [
    'sendMouseEvent',
    'sendTouchEvent',
    'goBack',
    'goForward',
    'reload',
    'stop',
    'download',
    'purgeHistory',
    'getScreenshot',
    'zoom',
    'getCanGoBack',
    'getCanGoForward'
  ];
  securitySensitiveCalls.forEach( function(call) {
    is(typeof ifr[call], "undefined", call + " should be hidden for widget");
  });
}

function loadFrameScript(mm) {
  var script = 'data:,\
  function ok(p, msg) { \
  if (p) { \
  sendAsyncMessage("OK", msg); \
} else { \
  sendAsyncMessage("KO", msg); \
} \
} \
  \
  function is(a, b, msg) { \
  if (a == b) { \
  sendAsyncMessage("OK", a + " == " + b + " - " + msg); \
} else { \
  sendAsyncMessage("KO", a + " != " + b + " - " + msg); \
} \
} \
  \
  function finish() { \
  sendAsyncMessage("DONE",""); \
} \
  \
  function onError() { \
  ok(false, "Error callback invoked"); \
  finish(); \
} \
  \
  function checkWidget(widget) { \
  /*For invalid widget case, ignore the following check*/\
  if (widget) { \
  var widgetName = "Really Rapid Release (APPTYPETOKEN)"; \
  is(widget.origin, "http://test", "Widget origin should be correct"); \
  is(widget.installOrigin, "http://mochi.test:8888", "Install origin should be correct"); \
} \
  finish(); \
} \
  \
  var request = content.window.navigator.mozApps.getSelf(); \
  request.onsuccess = function() { \
  var widget = request.result; \
  ok(widget,"Should be a widget"); \
  checkWidget(widget); \
}; \
  request.onerror = onError; \
  content.window.open("about:blank"); /*test mozbrowseropenwindow*/ \
  content.window.scrollTo(4000, 4000); /*test mozbrowser(async)scroll*/ \
  ';
  mm.loadFrameScript(script,  false);
}

var tests = [
  
  function() {
    SpecialPowers.pushPermissions(
      [{ "type": "browser", "allow": 1, "context": document },
       { "type": "embed-widgets", "allow": 1, "context": document },
       { "type": "webapps-manage", "allow": 1, "context": document }], runTest);
  },

  
  function() {
    SpecialPowers.pushPrefEnv({"set": [["dom.mozBrowserFramesEnabled", true],
                                       ["dom.enable_widgets", true],
                                       ["dom.datastore.sysMsgOnChangeShortTimeoutSec", 1],
                                       ["dom.datastore.sysMsgOnChangeLongTimeoutSec", 3]]}, runTest);
  },

  function() {
    if (SpecialPowers.isMainProcess()) {
      SpecialPowers.Cu.import("resource://gre/modules/DataStoreChangeNotifier.jsm");
    }

    SpecialPowers.setAllAppsLaunchable(true);
    runTest();
  },

  
  function() {
    SpecialPowers.autoConfirmAppInstall(() => {
      SpecialPowers.autoConfirmAppUninstall(runTest);
    });
  },

  
  ()=>installApp(gWidgetManifestURL),

  
  ()=>testApp(true),

  
  uninstallApp,

  
  ()=>installApp(gInvalidWidgetManifestURL),

  
  ()=>testApp(false),

  
  uninstallApp
];

function runTest() {
  if (!tests.length) {
    finish();
    return;
  }

  var test = tests.shift();
  test();
}

function finish() {
  SimpleTest.finish();
}
