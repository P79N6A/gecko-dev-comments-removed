





































const CC = Components.classes;
const CI = Components.interfaces;
const CR = Components.results;

const XHTML_NS = "http://www.w3.org/1999/xhtml";

const NS_LOCAL_FILE_CONTRACTID = "@mozilla.org/file/local;1";
const NS_GFXINFO_CONTRACTID = "@mozilla.org/gfx/info;1";
const IO_SERVICE_CONTRACTID = "@mozilla.org/network/io-service;1";
const DEBUG_CONTRACTID = "@mozilla.org/xpcom/debug;1";
const NS_LOCALFILEINPUTSTREAM_CONTRACTID =
          "@mozilla.org/network/file-input-stream;1";
const NS_SCRIPTSECURITYMANAGER_CONTRACTID =
          "@mozilla.org/scriptsecuritymanager;1";
const NS_REFTESTHELPER_CONTRACTID =
          "@mozilla.org/reftest-helper;1";
const NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX =
          "@mozilla.org/network/protocol;1?name=";
const NS_XREAPPINFO_CONTRACTID =
          "@mozilla.org/xre/app-info;1";

var gLoadTimeout = 0;
var gTimeoutHook = null;
var gRemote = false;
var gTotalChunks = 0;
var gThisChunk = 0;


const BLANK_URL_FOR_CLEARING = "data:text/html,%3C%21%2D%2DCLEAR%2D%2D%3E";

var gBrowser;
var gCanvas1, gCanvas2;


var gCurrentCanvas = null;
var gURLs;

var gURIUseCounts;

var gURICanvases;
var gTestResults = {
  
  Pass: 0,
  LoadOnly: 0,
  
  Exception: 0,
  FailedLoad: 0,
  UnexpectedFail: 0,
  UnexpectedPass: 0,
  AssertionUnexpected: 0,
  AssertionUnexpectedFixed: 0,
  
  KnownFail : 0,
  AssertionKnown: 0,
  Random : 0,
  Skip: 0,
  Slow: 0,
};
var gTotalTests = 0;
var gState;







var gExplicitPendingPaintCount = 0;
var gExplicitPendingPaintsCompleteHook;
var gCurrentURL;
var gFailureTimeout = null;
var gFailureReason;
var gTestLog = [];
var gServer;
var gCount = 0;
var gAssertionCount = 0;

var gIOService;
var gDebug;
var gWindowUtils;

var gCurrentTestStartTime;
var gSlowestTestTime = 0;
var gSlowestTestURL;
var gClearingForAssertionCheck = false;

var gDrawWindowFlags;

const TYPE_REFTEST_EQUAL = '==';
const TYPE_REFTEST_NOTEQUAL = '!=';
const TYPE_LOAD = 'load';     
                              
const TYPE_SCRIPT = 'script'; 

const EXPECTED_PASS = 0;
const EXPECTED_FAIL = 1;
const EXPECTED_RANDOM = 2;
const EXPECTED_DEATH = 3;  

const gProtocolRE = /^\w+:/;

var HTTP_SERVER_PORT = 4444;
const HTTP_SERVER_PORTS_TO_TRY = 50;


var gRunSlowTests = true;


var gNoCanvasCache = false;

var gRecycledCanvases = new Array();


var gDumpLog = dump;

function LogWarning(str)
{
    gDumpLog("REFTEST INFO | " + str + "\n");
    gTestLog.push(str);
}

function LogInfo(str)
{
    
    gTestLog.push(str);
}

function FlushTestLog()
{
    for (var i = 0; i < gTestLog.length; ++i) {
        gDumpLog("REFTEST INFO | Saved log: " + gTestLog[i] + "\n");
    }
    gTestLog = [];
}

function AllocateCanvas()
{
    var windowElem = document.documentElement;

    if (gRecycledCanvases.length > 0)
        return gRecycledCanvases.shift();

    var canvas = document.createElementNS(XHTML_NS, "canvas");
    var r = gBrowser.getBoundingClientRect();
    canvas.setAttribute("width", Math.ceil(r.width));
    canvas.setAttribute("height", Math.ceil(r.height));

    return canvas;
}

function ReleaseCanvas(canvas)
{
    
    if (!gNoCanvasCache || gRecycledCanvases.length < 2)
        gRecycledCanvases.push(canvas);
}

function IDForEventTarget(event)
{
    try {
        return "'" + event.target.getAttribute('id') + "'";
    } catch (ex) {
        return "<unknown>";
    }
}

function PaintWaitListener(event)
{
    LogInfo("MozPaintWait received for ID " + IDForEventTarget(event));
    gExplicitPendingPaintCount++;
}

function PaintWaitFinishedListener(event)
{
    LogInfo("MozPaintWaitFinished received for ID " + IDForEventTarget(event));
    gExplicitPendingPaintCount--;
    if (gExplicitPendingPaintCount < 0) {
        LogWarning("Underrun in gExplicitPendingPaintCount\n");
        gExplicitPendingPaintCount = 0;
    }
    if (gExplicitPendingPaintCount == 0 &&
        gExplicitPendingPaintsCompleteHook) {
        gExplicitPendingPaintsCompleteHook();
    }
}

function OnRefTestLoad()
{
    gBrowser = document.getElementById("browser");

    
    try {
      var prefs = Components.classes["@mozilla.org/preferences-service;1"].
                  getService(Components.interfaces.nsIPrefBranch2);
      gLoadTimeout = prefs.getIntPref("reftest.timeout");
      logFile = prefs.getCharPref("reftest.logFile");
      if (logFile) {
        try {
          MozillaFileLogger.init(logFile);
          
          gDumpLog = function (msg) {dump(msg); MozillaFileLogger.log(msg);};
        }
        catch(e) {
          
          gDumpLog = dump;
        }
      }
      gRemote = prefs.getBoolPref("reftest.remote");
    }
    catch(e) {
      gLoadTimeout = 5 * 60 * 1000; 
    }


    
    try {
      gTotalChunks = prefs.getIntPref("reftest.totalChunks");
      gThisChunk = prefs.getIntPref("reftest.thisChunk");
    }
    catch(e) {
      gTotalChunks = 0;
      gThisChunk = 0;
    }

    gBrowser.addEventListener("load", OnDocumentLoad, true);

    try {
        gWindowUtils = window.QueryInterface(CI.nsIInterfaceRequestor).getInterface(CI.nsIDOMWindowUtils);
        if (gWindowUtils && !gWindowUtils.compareCanvases)
            gWindowUtils = null;
    } catch (e) {
        gWindowUtils = null;
    }

    var windowElem = document.documentElement;

    gIOService = CC[IO_SERVICE_CONTRACTID].getService(CI.nsIIOService);
    gDebug = CC[DEBUG_CONTRACTID].getService(CI.nsIDebug2);
    
    if (gRemote) {
      gServer = null;
    } else {
      gServer = CC["@mozilla.org/server/jshttp;1"].
                    createInstance(CI.nsIHttpServer);
    }
    try {
        if (gServer)
            StartHTTPServer();
    } catch (ex) {
        
        ++gTestResults.Exception;
        gDumpLog("REFTEST TEST-UNEXPECTED-FAIL | | EXCEPTION: " + ex + "\n");
        DoneTests();
    }

    
    gBrowser.focus();

    
    gBrowser.addEventListener("MozPaintWait", PaintWaitListener, true);
    gBrowser.addEventListener("MozPaintWaitFinished", PaintWaitFinishedListener, true);

    StartTests();
}

function StartHTTPServer()
{
    gServer.registerContentType("sjs", "sjs");
    
    
    var tries = HTTP_SERVER_PORTS_TO_TRY;
    do {
        try {
            gServer.start(HTTP_SERVER_PORT);
            return;
        } catch (ex) {
            ++HTTP_SERVER_PORT;
            if (--tries == 0)
                throw ex;
        }
    } while (true);
}

function StartTests()
{
    try {
        
        var args = window.arguments[0].wrappedJSObject;

        if ("nocache" in args && args["nocache"])
            gNoCanvasCache = true;

        if ("skipslowtests" in args && args.skipslowtests)
            gRunSlowTests = false;

        ReadTopManifest(args.uri);
        BuildUseCounts();

        if (gTotalChunks > 0 && gThisChunk > 0) {
          var testsPerChunk = gURLs.length / gTotalChunks;
          var start = Math.round((gThisChunk-1) * testsPerChunk);
          var end = Math.round(gThisChunk * testsPerChunk);
          gURLs = gURLs.slice(start, end);
          gDumpLog("REFTEST INFO | Running chunk " + gThisChunk + " out of " + gTotalChunks + " chunks.  ")
          gDumpLog("tests " + (start+1) + "-" + end + "/" + gURLs.length + "\n");
        }
        gTotalTests = gURLs.length;

        if (!gTotalTests)
            throw "No tests to run";

        gURICanvases = {};
        StartCurrentTest();
    } catch (ex) {
        
        ++gTestResults.Exception;
        gDumpLog("REFTEST TEST-UNEXPECTED-FAIL | | EXCEPTION: " + ex + "\n");
        DoneTests();
    }
}

function OnRefTestUnload()
{
    
    var prefs = Components.classes["@mozilla.org/preferences-service;1"].
                getService(Components.interfaces.nsIPrefBranch2);
    prefs.clearUserPref("gfx.color_management.force_srgb");

    gBrowser.removeEventListener("load", OnDocumentLoad, true);
    MozillaFileLogger.close();
}



function getStreamContent(inputStream)
{
  var streamBuf = "";
  var sis = CC["@mozilla.org/scriptableinputstream;1"].
                createInstance(CI.nsIScriptableInputStream);
  sis.init(inputStream);

  var available;
  while ((available = sis.available()) != 0) {
    streamBuf += sis.read(available);
  }
  
  return streamBuf;
}


function BuildConditionSandbox(aURL) {
    var sandbox = new Components.utils.Sandbox(aURL.spec);
    var xr = CC[NS_XREAPPINFO_CONTRACTID].getService(CI.nsIXULRuntime);
    sandbox.isDebugBuild = gDebug.isDebugBuild;
    sandbox.xulRuntime = {widgetToolkit: xr.widgetToolkit, OS: xr.OS};

    
    
    try {
      sandbox.xulRuntime.XPCOMABI = xr.XPCOMABI;
    } catch(e) {
      sandbox.xulRuntime.XPCOMABI = "";
    }
  
    try {
      
      sandbox.d2d = CC[NS_GFXINFO_CONTRACTID].getService(CI.nsIGfxInfo).D2DEnabled;
    } catch(e) {
      sandbox.d2d = false;
    }

    if (gWindowUtils && gWindowUtils.layerManagerType != "Basic")
      sandbox.layersGPUAccelerated = true;
    else
      sandbox.layersGPUAccelerated = false;
 
    
    sandbox.cocoaWidget = xr.widgetToolkit == "cocoa";
    sandbox.gtk2Widget = xr.widgetToolkit == "gtk2";
    sandbox.qtWidget = xr.widgetToolkit == "qt";
    sandbox.winWidget = xr.widgetToolkit == "windows";

    var hh = CC[NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX + "http"].
                 getService(CI.nsIHttpProtocolHandler);
    sandbox.http = {};
    for each (var prop in [ "userAgent", "appName", "appVersion",
                            "vendor", "vendorSub",
                            "product", "productSub",
                            "platform", "oscpu", "language", "misc" ])
        sandbox.http[prop] = hh[prop];
    
    
    sandbox.haveTestPlugin = false;
    for (var i = 0; i < navigator.mimeTypes.length; i++) {
        if (navigator.mimeTypes[i].type == "application/x-test" &&
            navigator.mimeTypes[i].enabledPlugin != null &&
            navigator.mimeTypes[i].enabledPlugin.name == "Test Plug-in") {
            sandbox.haveTestPlugin = true;
            break;
        }
    }

    
    var box = document.createElement("box");
    box.setAttribute("id", "_box_windowsDefaultTheme");
    document.documentElement.appendChild(box);
    sandbox.windowsDefaultTheme = (getComputedStyle(box, null).display == "none");
    document.documentElement.removeChild(box);

    var prefs = CC["@mozilla.org/preferences-service;1"].
                getService(CI.nsIPrefBranch2);
    try {
        sandbox.nativeThemePref = !prefs.getBoolPref("mozilla.widget.disable-native-theme");
    } catch (e) {
        sandbox.nativeThemePref = true;
    }

    sandbox.prefs = {
        __exposedProps__: {
            getBoolPref: 'r',
            getIntPref: 'r',
        },
        _prefs:      prefs,
        getBoolPref: function(p) { return this._prefs.getBoolPref(p); },
        getIntPref:  function(p) { return this._prefs.getIntPref(p); }
    }

    sandbox.testPluginIsOOP = function () {
        netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
        var prefservice = Components.classes["@mozilla.org/preferences-service;1"]
                                    .getService(CI.nsIPrefBranch);

        var testPluginIsOOP = false;
        if (navigator.platform.indexOf("Mac") == 0) {
            var xulRuntime = Components.classes["@mozilla.org/xre/app-info;1"]
                                       .getService(CI.nsIXULAppInfo)
                                       .QueryInterface(CI.nsIXULRuntime);
            if (xulRuntime.XPCOMABI.match(/x86-/)) {
                try {
                    testPluginIsOOP = prefservice.getBoolPref("dom.ipc.plugins.enabled.i386.test.plugin");
                } catch (e) {
                    testPluginIsOOP = prefservice.getBoolPref("dom.ipc.plugins.enabled.i386");
                }
            }
            else if (xulRuntime.XPCOMABI.match(/x86_64-/)) {
                try {
                    testPluginIsOOP = prefservice.getBoolPref("dom.ipc.plugins.enabled.x86_64.test.plugin");
                } catch (e) {
                    testPluginIsOOP = prefservice.getBoolPref("dom.ipc.plugins.enabled.x86_64");
                }
            }
        }
        else {
            testPluginIsOOP = prefservice.getBoolPref("dom.ipc.plugins.enabled");
        }

        return testPluginIsOOP;
    };

    gDumpLog("REFTEST INFO | Dumping JSON representation of sandbox \n");
    gDumpLog("REFTEST INFO | " + JSON.stringify(sandbox) + " \n");

    return sandbox;
}

function ReadTopManifest(aFileURL)
{
    gURLs = new Array();
    var url = gIOService.newURI(aFileURL, null, null);
    if (!url)
      throw "Expected a file or http URL for the manifest.";
    ReadManifest(url);
}



function ReadManifest(aURL)
{
    var secMan = CC[NS_SCRIPTSECURITYMANAGER_CONTRACTID]
                     .getService(CI.nsIScriptSecurityManager);

    var listURL = aURL;
    var channel = gIOService.newChannelFromURI(aURL);
    var inputStream = channel.open();
    if (channel instanceof Components.interfaces.nsIHttpChannel
        && channel.responseStatus != 200) {
      gDumpLog("REFTEST TEST-UNEXPECTED-FAIL | | HTTP ERROR : " + 
        channel.responseStatus + "\n");
    }
    var streamBuf = getStreamContent(inputStream);
    inputStream.close();
    var lines = streamBuf.split(/(\n|\r|\r\n)/);

    
    var sandbox = BuildConditionSandbox(aURL);

    var lineNo = 0;
    var urlprefix = "";
    for each (var str in lines) {
        ++lineNo;
        if (str.charAt(0) == "#")
            continue; 
        var i = str.search(/\s+#/);
        if (i >= 0)
            str = str.substring(0, i);
        
        str = str.replace(/^\s*/, '').replace(/\s*$/, '');
        if (!str || str == "")
            continue;
        var items = str.split(/\s+/); 

        if (items[0] == "url-prefix") {
            if (items.length != 2)
                throw "url-prefix requires one url in manifest file " + aURL.spec + " line " + lineNo;
            urlprefix = items[1];
            continue;
        }

        var expected_status = EXPECTED_PASS;
        var allow_silent_fail = false;
        var minAsserts = 0;
        var maxAsserts = 0;
        var slow = false;
        while (items[0].match(/^(fails|random|skip|asserts|slow|silentfail)/)) {
            var item = items.shift();
            var stat;
            var cond;
            var m = item.match(/^(fails|random|skip|silentfail)-if(\(.*\))$/);
            if (m) {
                stat = m[1];
                
                cond = Components.utils.evalInSandbox(m[2], sandbox);
            } else if (item.match(/^(fails|random|skip)$/)) {
                stat = item;
                cond = true;
            } else if ((m = item.match(/^asserts\((\d+)(-\d+)?\)$/))) {
                cond = false;
                minAsserts = Number(m[1]);
                maxAsserts = (m[2] == undefined) ? minAsserts
                                                 : Number(m[2].substring(1));
            } else if ((m = item.match(/^asserts-if\((.*?),(\d+)(-\d+)?\)$/))) {
                cond = false;
                if (Components.utils.evalInSandbox("(" + m[1] + ")", sandbox)) {
                    minAsserts = Number(m[2]);
                    maxAsserts =
                      (m[3] == undefined) ? minAsserts
                                          : Number(m[3].substring(1));
                }
            } else if (item == "slow") {
                cond = false;
                slow = true;
            } else if ((m = item.match(/^slow-if\((.*?)\)$/))) {
                cond = false;
                if (Components.utils.evalInSandbox("(" + m[1] + ")", sandbox))
                    slow = true;
            } else if (item == "silentfail") {
                cond = false;
                allow_silent_fail = true;
            } else {
                throw "Error 1 in manifest file " + aURL.spec + " line " + lineNo;
            }

            if (cond) {
                if (stat == "fails") {
                    expected_status = EXPECTED_FAIL;
                } else if (stat == "random") {
                    expected_status = EXPECTED_RANDOM;
                } else if (stat == "skip") {
                    expected_status = EXPECTED_DEATH;
                } else if (stat == "silentfail") {
                    allow_silent_fail = true;
                }
            }
        }

        if (minAsserts > maxAsserts) {
            throw "Bad range in manifest file " + aURL.spec + " line " + lineNo;
        }

        var runHttp = false;
        var httpDepth;
        if (items[0] == "HTTP") {
            runHttp = (aURL.scheme == "file"); 
                                               
            httpDepth = 0;
            items.shift();
        } else if (items[0].match(/HTTP\(\.\.(\/\.\.)*\)/)) {
            
            runHttp = (aURL.scheme == "file"); 
                                               
            httpDepth = (items[0].length - 5) / 3;
            items.shift();
        }

        
        
        if (urlprefix && items[0] != "include") {
            if (items.length > 1 && !items[1].match(gProtocolRE)) {
                items[1] = urlprefix + items[1];
            }
            if (items.length > 2 && !items[2].match(gProtocolRE)) {
                items[2] = urlprefix + items[2];
            }
        }

        if (items[0] == "include") {
            if (items.length != 2 || runHttp)
                throw "Error 2 in manifest file " + aURL.spec + " line " + lineNo;
            var incURI = gIOService.newURI(items[1], null, listURL);
            secMan.checkLoadURI(aURL, incURI,
                                CI.nsIScriptSecurityManager.DISALLOW_SCRIPT);
            ReadManifest(incURI);
        } else if (items[0] == TYPE_LOAD) {
            if (items.length != 2 ||
                (expected_status != EXPECTED_PASS &&
                 expected_status != EXPECTED_DEATH))
                throw "Error 3 in manifest file " + aURL.spec + " line " + lineNo;
            var [testURI] = runHttp
                            ? ServeFiles(aURL, httpDepth,
                                         listURL, [items[1]])
                            : [gIOService.newURI(items[1], null, listURL)];
            var prettyPath = runHttp
                           ? gIOService.newURI(items[1], null, listURL).spec
                           : testURI.spec;
            secMan.checkLoadURI(aURL, testURI,
                                CI.nsIScriptSecurityManager.DISALLOW_SCRIPT);
            gURLs.push( { type: TYPE_LOAD,
                          expected: expected_status,
                          allowSilentFail: allow_silent_fail,
                          prettyPath: prettyPath,
                          minAsserts: minAsserts,
                          maxAsserts: maxAsserts,
                          slow: slow,
                          url1: testURI,
                          url2: null } );
        } else if (items[0] == TYPE_SCRIPT) {
            if (items.length != 2)
                throw "Error 4 in manifest file " + aURL.spec + " line " + lineNo;
            var [testURI] = runHttp
                            ? ServeFiles(aURL, httpDepth,
                                         listURL, [items[1]])
                            : [gIOService.newURI(items[1], null, listURL)];
            var prettyPath = runHttp
                           ? gIOService.newURI(items[1], null, listURL).spec
                           : testURI.spec;
            secMan.checkLoadURI(aURL, testURI,
                                CI.nsIScriptSecurityManager.DISALLOW_SCRIPT);
            gURLs.push( { type: TYPE_SCRIPT,
                          expected: expected_status,
                          allowSilentFail: allow_silent_fail,
                          prettyPath: prettyPath,
                          minAsserts: minAsserts,
                          maxAsserts: maxAsserts,
                          slow: slow,
                          url1: testURI,
                          url2: null } );
        } else if (items[0] == TYPE_REFTEST_EQUAL || items[0] == TYPE_REFTEST_NOTEQUAL) {
            if (items.length != 3)
                throw "Error 5 in manifest file " + aURL.spec + " line " + lineNo;
            var [testURI, refURI] = runHttp
                                  ? ServeFiles(aURL, httpDepth,
                                               listURL, [items[1], items[2]])
                                  : [gIOService.newURI(items[1], null, listURL),
                                     gIOService.newURI(items[2], null, listURL)];
            var prettyPath = runHttp
                           ? gIOService.newURI(items[1], null, listURL).spec
                           : testURI.spec;
            secMan.checkLoadURI(aURL, testURI,
                                CI.nsIScriptSecurityManager.DISALLOW_SCRIPT);
            secMan.checkLoadURI(aURL, refURI,
                                CI.nsIScriptSecurityManager.DISALLOW_SCRIPT);
            gURLs.push( { type: items[0],
                          expected: expected_status,
                          allowSilentFail: allow_silent_fail,
                          prettyPath: prettyPath,
                          minAsserts: minAsserts,
                          maxAsserts: maxAsserts,
                          slow: slow,
                          url1: testURI,
                          url2: refURI } );
        } else {
            throw "Error 6 in manifest file " + aURL.spec + " line " + lineNo;
        }
    }
}

function AddURIUseCount(uri)
{
    if (uri == null)
        return;

    var spec = uri.spec;
    if (spec in gURIUseCounts) {
        gURIUseCounts[spec]++;
    } else {
        gURIUseCounts[spec] = 1;
    }
}

function BuildUseCounts()
{
    gURIUseCounts = {};
    for (var i = 0; i < gURLs.length; ++i) {
        var url = gURLs[i];
        if (url.expected != EXPECTED_DEATH &&
            (url.type == TYPE_REFTEST_EQUAL ||
             url.type == TYPE_REFTEST_NOTEQUAL)) {
            AddURIUseCount(gURLs[i].url1);
            AddURIUseCount(gURLs[i].url2);
        }
    }
}

function ServeFiles(manifestURL, depth, aURL, files)
{
    var listURL = aURL.QueryInterface(CI.nsIFileURL);
    var directory = listURL.file.parent;

    
    
    var dirPath = "/";
    while (depth > 0) {
        dirPath = "/" + directory.leafName + dirPath;
        directory = directory.parent;
        --depth;
    }

    gCount++;
    var path = "/" + Date.now() + "/" + gCount;
    gServer.registerDirectory(path + "/", directory);

    var secMan = CC[NS_SCRIPTSECURITYMANAGER_CONTRACTID]
                     .getService(CI.nsIScriptSecurityManager);

    var testbase = gIOService.newURI("http://localhost:" + HTTP_SERVER_PORT +
                                         path + dirPath,
                                     null, null);

    function FileToURI(file)
    {
        
        
        var testURI = gIOService.newURI(file, null, testbase);

        
        secMan.checkLoadURI(manifestURL, testURI,
                            CI.nsIScriptSecurityManager.DISALLOW_SCRIPT);

        return testURI;
    }

    return files.map(FileToURI);
}

function StartCurrentTest()
{
    gTestLog = [];

    
    while (gURLs.length > 0) {
        var test = gURLs[0];
        if (test.expected == EXPECTED_DEATH) {
            ++gTestResults.Skip;
            gDumpLog("REFTEST TEST-KNOWN-FAIL | " + test.url1.spec + " | (SKIP)\n");
            gURLs.shift();
        } else if (test.slow && !gRunSlowTests) {
            ++gTestResults.Slow;
            gDumpLog("REFTEST TEST-KNOWN-SLOW | " + test.url1.spec + " | (SLOW)\n");
            gURLs.shift();
        } else {
            break;
        }
    }

    if (gURLs.length == 0) {
        DoneTests();
    }
    else {
        var currentTest = gTotalTests - gURLs.length;
        document.title = "reftest: " + currentTest + " / " + gTotalTests +
            " (" + Math.floor(100 * (currentTest / gTotalTests)) + "%)";
        StartCurrentURI(1);
    }
}

function StartCurrentURI(aState)
{
    gCurrentTestStartTime = Date.now();
    if (gFailureTimeout != null) {
        gDumpLog("REFTEST TEST-UNEXPECTED-FAIL | " +
             "| program error managing timeouts\n");
        ++gTestResults.Exception;
    }
    gFailureTimeout = setTimeout(LoadFailed, gLoadTimeout);
    gFailureReason = "timed out waiting for onload to fire";

    gState = aState;
    gCurrentURL = gURLs[0]["url" + aState].spec;
    
    
    if (gExplicitPendingPaintCount != 0) {
        LogWarning("Resetting gExplicitPendingPaintCount to zero (currently " +
                   gExplicitPendingPaintCount + "\n");
        gExplicitPendingPaintCount = 0;
    }

    if (gURICanvases[gCurrentURL] &&
        (gURLs[0].type == TYPE_REFTEST_EQUAL ||
         gURLs[0].type == TYPE_REFTEST_NOTEQUAL) &&
        gURLs[0].maxAsserts == 0) {
        
        
        setTimeout(RecordResult, 0);
    } else {
        gDumpLog("REFTEST TEST-START | " + gCurrentURL + "\n");
        LogInfo("START " + gCurrentURL);
        gBrowser.loadURI(gCurrentURL);
    }
}

function DoneTests()
{
    gDumpLog("REFTEST FINISHED: Slowest test took " + gSlowestTestTime +
         "ms (" + gSlowestTestURL + ")\n");

    gDumpLog("REFTEST INFO | Result summary:\n");
    var count = gTestResults.Pass + gTestResults.LoadOnly;
    gDumpLog("REFTEST INFO | Successful: " + count + " (" +
             gTestResults.Pass + " pass, " +
             gTestResults.LoadOnly + " load only)\n");
    count = gTestResults.Exception + gTestResults.FailedLoad +
            gTestResults.UnexpectedFail + gTestResults.UnexpectedPass +
            gTestResults.AssertionUnexpected +
            gTestResults.AssertionUnexpectedFixed;
    gDumpLog("REFTEST INFO | Unexpected: " + count + " (" +
             gTestResults.UnexpectedFail + " unexpected fail, " +
             gTestResults.UnexpectedPass + " unexpected pass, " +
             gTestResults.AssertionUnexpected + " unexpected asserts, " +
             gTestResults.AssertionUnexpectedFixed + " unexpected fixed asserts, " +
             gTestResults.FailedLoad + " failed load, " +
             gTestResults.Exception + " exception)\n");
    count = gTestResults.KnownFail + gTestResults.AssertionKnown +
            gTestResults.Random + gTestResults.Skip + gTestResults.Slow;
    gDumpLog("REFTEST INFO | Known problems: " + count + " (" +
             gTestResults.KnownFail + " known fail, " +
             gTestResults.AssertionKnown + " known asserts, " +
             gTestResults.Random + " random, " +
             gTestResults.Skip + " skipped, " +
             gTestResults.Slow + " slow)\n");

    gDumpLog("REFTEST INFO | Total canvas count = " + gRecycledCanvases.length + "\n");

    gDumpLog("REFTEST TEST-START | Shutdown\n");
    function onStopped() {
        goQuitApplication();
    }
    if (gServer)
        gServer.stop(onStopped);
    else
        onStopped();
}

function setupZoom(contentRootElement) {
    if (!contentRootElement || !contentRootElement.hasAttribute('reftest-zoom'))
        return;
    gBrowser.markupDocumentViewer.fullZoom =
        contentRootElement.getAttribute('reftest-zoom');
}

function resetZoom() {
    gBrowser.markupDocumentViewer.fullZoom = 1.0;
}

function doPrintMode(contentRootElement) {
    
    return contentRootElement &&
           contentRootElement.hasAttribute('class') &&
           contentRootElement.getAttribute('class').split(/\s+/)
                             .indexOf("reftest-print") != -1;
}

function setupPrintMode() {
   var PSSVC = Components.classes["@mozilla.org/gfx/printsettings-service;1"]
               .getService(Components.interfaces.nsIPrintSettingsService);
   var ps = PSSVC.newPrintSettings;
   ps.paperWidth = 5;
   ps.paperHeight = 3;

   
   ps.unwriteableMarginTop = 0;
   ps.unwriteableMarginLeft = 0;
   ps.unwriteableMarginBottom = 0;
   ps.unwriteableMarginRight = 0;

   ps.headerStrLeft = "";
   ps.headerStrCenter = "";
   ps.headerStrRight = "";
   ps.footerStrLeft = "";
   ps.footerStrCenter = "";
   ps.footerStrRight = "";
   gBrowser.docShell.contentViewer.setPageMode(true, ps);
}

function shouldWaitForExplicitPaintWaiters() {
    return gExplicitPendingPaintCount > 0;
}

function shouldWaitForPendingPaints() {
    return gWindowUtils.isMozAfterPaintPending;
}

function shouldWaitForReftestWaitRemoval() {
    var contentRootElement = gBrowser.contentDocument.documentElement;
    
    return contentRootElement &&
           contentRootElement.hasAttribute('class') &&
           contentRootElement.getAttribute('class').split(/\s+/)
                             .indexOf("reftest-wait") != -1;
}




const STATE_WAITING_TO_FIRE_INVALIDATE_EVENT = 0;


const STATE_WAITING_FOR_REFTEST_WAIT_REMOVAL = 1;


const STATE_WAITING_TO_FINISH = 2;
const STATE_COMPLETED = 3;

function WaitForTestEnd() {
    var contentRootElement = gBrowser.contentDocument.documentElement;

    var stopAfterPaintReceived = false;
    var currentDoc = gBrowser.contentDocument;
    var state = STATE_WAITING_TO_FIRE_INVALIDATE_EVENT;

    function FlushRendering() {
        var anyPendingPaintsGeneratedInDescendants = false;

        function flushWindow(win) {
            var utils = win.QueryInterface(CI.nsIInterfaceRequestor)
                        .getInterface(CI.nsIDOMWindowUtils);
            var afterPaintWasPending = utils.isMozAfterPaintPending;

            try {
                
                win.document.documentElement.getBoundingClientRect();
            } catch (e) {
                LogWarning("flushWindow failed: " + e + "\n");
            }
            
            if (!afterPaintWasPending && utils.isMozAfterPaintPending) {
                LogInfo("FlushRendering generated paint for window " + win.location.href);
                anyPendingPaintsGeneratedInDescendants = true;
            }

            for (var i = 0; i < win.frames.length; ++i) {
                flushWindow(win.frames[i]);
            }
        }

        flushWindow(gBrowser.contentWindow);

        if (anyPendingPaintsGeneratedInDescendants &&
            !gWindowUtils.isMozAfterPaintPending) {
            LogWarning("Internal error: descendant frame generated a MozAfterPaint event, but the root document doesn't have one!");
        }
    }

    function AfterPaintListener(event) {
        LogInfo("AfterPaintListener in " + event.target.document.location.href);
        if (event.target.document != document) {
            
            
            return;
        }
        UpdateCurrentCanvasForEvent(event);
        
        
        
        setTimeout(MakeProgress, 0);
    }

    function AttrModifiedListener() {
        LogInfo("AttrModifiedListener fired");
        
        
        
        
        setTimeout(MakeProgress, 0);
    }

    function ExplicitPaintsCompleteListener() {
        LogInfo("ExplicitPaintsCompleteListener fired");
        
        
        setTimeout(MakeProgress, 0);
    }

    function RemoveListeners() {
        
        window.removeEventListener("MozAfterPaint", AfterPaintListener, false);
        contentRootElement.removeEventListener("DOMAttrModified", AttrModifiedListener, false);
        gExplicitPendingPaintsCompleteHook = null;
        gTimeoutHook = null;
        
        
        state = STATE_COMPLETED;
    }

    
    
    
    function MakeProgress() {
        if (state >= STATE_COMPLETED) {
            LogInfo("MakeProgress: STATE_COMPLETED");
            return;
        }

        FlushRendering();

        switch (state) {
        case STATE_WAITING_TO_FIRE_INVALIDATE_EVENT: {
            LogInfo("MakeProgress: STATE_WAITING_TO_FIRE_INVALIDATE_EVENT");
            if (shouldWaitForExplicitPaintWaiters() || shouldWaitForPendingPaints()) {
                gFailureReason = "timed out waiting for pending paint count to reach zero";
                if (shouldWaitForExplicitPaintWaiters()) {
                    gFailureReason += " (waiting for MozPaintWaitFinished)";
                    LogInfo("MakeProgress: waiting for MozPaintWaitFinished");
                }
                if (shouldWaitForPendingPaints()) {
                    gFailureReason += " (waiting for MozAfterPaint)";
                    LogInfo("MakeProgress: waiting for MozAfterPaint");
                }
                return;
            }

            state = STATE_WAITING_FOR_REFTEST_WAIT_REMOVAL;
            var hasReftestWait = shouldWaitForReftestWaitRemoval();            
            
            var notification = document.createEvent("Events");
            notification.initEvent("MozReftestInvalidate", true, false);
            contentRootElement.dispatchEvent(notification);
            if (hasReftestWait && !shouldWaitForReftestWaitRemoval()) {
                
                
                FlushRendering();
                if (!shouldWaitForPendingPaints() && !shouldWaitForExplicitPaintWaiters()) {
                    LogWarning("MozInvalidateEvent didn't invalidate");
                }
            }
            
            MakeProgress();
            return;
        }

        case STATE_WAITING_FOR_REFTEST_WAIT_REMOVAL:
            LogInfo("MakeProgress: STATE_WAITING_FOR_REFTEST_WAIT_REMOVAL");
            if (shouldWaitForReftestWaitRemoval()) {
                gFailureReason = "timed out waiting for reftest-wait to be removed";
                LogInfo("MakeProgress: waiting for reftest-wait to be removed");
                return;
            }
            state = STATE_WAITING_TO_FINISH;
            if (doPrintMode(contentRootElement)) {
                LogInfo("MakeProgress: setting up print mode");
                setupPrintMode();
                didPrintMode = true;
            }
            
            MakeProgress();
            return;

        case STATE_WAITING_TO_FINISH:
            LogInfo("MakeProgress: STATE_WAITING_TO_FINISH");
            if (shouldWaitForExplicitPaintWaiters() || shouldWaitForPendingPaints()) {
                gFailureReason = "timed out waiting for pending paint count to " +
                    "reach zero (after reftest-wait removed and switch to print mode)";
                if (shouldWaitForExplicitPaintWaiters()) {
                    gFailureReason += " (waiting for MozPaintWaitFinished)";
                    LogInfo("MakeProgress: waiting for MozPaintWaitFinished");
                }
                if (shouldWaitForPendingPaints()) {
                    gFailureReason += " (waiting for MozAfterPaint)";
                    LogInfo("MakeProgress: waiting for MozAfterPaint");
                }
                return;
            }
            LogInfo("MakeProgress: Completed");
            state = STATE_COMPLETED;
            gFailureReason = "timed out while taking snapshot (bug in harness?)";
            RemoveListeners();
            setTimeout(RecordResult, 0);
            return;
        }
    }

    window.addEventListener("MozAfterPaint", AfterPaintListener, false);
    contentRootElement.addEventListener("DOMAttrModified", AttrModifiedListener, false);
    gExplicitPendingPaintsCompleteHook = ExplicitPaintsCompleteListener;
    gTimeoutHook = RemoveListeners;

    
    
    
    InitCurrentCanvasWithSnapshot();
    MakeProgress();
}

function OnDocumentLoad(event)
{
    if (event.target != gBrowser.contentDocument)
        
        return;

    if (gClearingForAssertionCheck &&
        gBrowser.contentDocument.location.href == BLANK_URL_FOR_CLEARING) {
        DoAssertionCheck();
        return;
    }

    if (gBrowser.contentDocument.location.href != gCurrentURL) {
        LogInfo("OnDocumentLoad fired for previous document");
        
        return;
    }

    var contentRootElement = gBrowser.contentDocument.documentElement;

    setupZoom(contentRootElement);

    function AfterOnLoadScripts() {
        if (doPrintMode(contentRootElement)) {
            LogInfo("AfterOnLoadScripts setting up print mode");
            setupPrintMode();
        }

        
        
        
        
        InitCurrentCanvasWithSnapshot();

        if (shouldWaitForExplicitPaintWaiters()) {
            LogInfo("AfterOnLoadScripts belatedly entering WaitForTestEnd");
            
            WaitForTestEnd();
        } else {
            RecordResult();
        }
    }

    if (shouldWaitForReftestWaitRemoval() || shouldWaitForExplicitPaintWaiters()) {
        
        
        gFailureReason = "timed out waiting for test to complete (trying to get into WaitForTestEnd)";
        LogInfo("OnDocumentLoad triggering WaitForTestEnd");
        setTimeout(WaitForTestEnd, 0);
    } else {
        
        
        
        
        gFailureReason = "timed out waiting for test to complete (waiting for onload scripts to complete)";
        LogInfo("OnDocumentLoad triggering AfterOnLoadScripts");
        setTimeout(setTimeout, 0, AfterOnLoadScripts, 0);
    }
}

function UpdateCanvasCache(url, canvas)
{
    var spec = url.spec;

    --gURIUseCounts[spec];

    if (gNoCanvasCache || gURIUseCounts[spec] == 0) {
        ReleaseCanvas(canvas);
        delete gURICanvases[spec];
    } else if (gURIUseCounts[spec] > 0) {
        gURICanvases[spec] = canvas;
    } else {
        throw "Use counts were computed incorrectly";
    }
}






function DoDrawWindow(ctx, x, y, w, h)
{
    var flags = ctx.DRAWWINDOW_DRAW_CARET | ctx.DRAWWINDOW_DRAW_VIEW;
    var testRect = gBrowser.getBoundingClientRect();
    if (0 <= testRect.left &&
        0 <= testRect.top &&
        window.innerWidth >= testRect.right &&
        window.innerHeight >= testRect.bottom) {
        
        
        
        flags |= ctx.DRAWWINDOW_USE_WIDGET_LAYERS;
    }

    if (gDrawWindowFlags != flags) {
        
        gDrawWindowFlags = flags;
        var flagsStr = "DRAWWINDOW_DRAW_CARET | DRAWWINDOW_DRAW_VIEW";
        if (flags & ctx.DRAWWINDOW_USE_WIDGET_LAYERS) {
            flagsStr += " | DRAWWINDOW_USE_WIDGET_LAYERS";
        } else {
            
            
            gDumpLog("REFTEST INFO | WARNING: USE_WIDGET_LAYERS disabled\n");
        }
        gDumpLog("REFTEST INFO | drawWindow flags = " + flagsStr +
                 "; window size = " + window.innerWidth + "," + window.innerHeight +
                 "; test browser size = " + testRect.width + "," + testRect.height +
                 "\n");
    }

    LogInfo("DoDrawWindow " + x + "," + y + "," + w + "," + h);
    ctx.drawWindow(window, x, y, w, h, "rgb(255,255,255)",
                   gDrawWindowFlags);
}

function InitCurrentCanvasWithSnapshot()
{
    if (gURLs[0].type == TYPE_LOAD || gURLs[0].type == TYPE_SCRIPT) {
        
        return;
    }

    if (!gCurrentCanvas) {
        gCurrentCanvas = AllocateCanvas();
    }

    var ctx = gCurrentCanvas.getContext("2d");
    DoDrawWindow(ctx, 0, 0, gCurrentCanvas.width, gCurrentCanvas.height);
}

function roundTo(x, fraction)
{
    return Math.round(x/fraction)*fraction;
}

function UpdateCurrentCanvasForEvent(event)
{
    if (!gCurrentCanvas)
        return;

    var ctx = gCurrentCanvas.getContext("2d");
    var rectList = event.clientRects;
    for (var i = 0; i < rectList.length; ++i) {
        var r = rectList[i];
        
        var left = Math.floor(r.left);
        var top = Math.floor(r.top);
        var right = Math.ceil(r.right);
        var bottom = Math.ceil(r.bottom);

        ctx.save();
        ctx.translate(left, top);
        DoDrawWindow(ctx, left, top, right - left, bottom - top);
        ctx.restore();
    }
}

function RecordResult()
{
    LogInfo("RecordResult fired");

    
    var currentTestRunTime = Date.now() - gCurrentTestStartTime;
    if (currentTestRunTime > gSlowestTestTime) {
        gSlowestTestTime = currentTestRunTime;
        gSlowestTestURL  = gCurrentURL;
    }

    clearTimeout(gFailureTimeout);
    gFailureReason = null;
    gFailureTimeout = null;

    
    var outputs = {};
    const randomMsg = "(EXPECTED RANDOM)";
    outputs[EXPECTED_PASS] = {
        true:  {s: "TEST-PASS"                  , n: "Pass"},
        false: {s: "TEST-UNEXPECTED-FAIL"       , n: "UnexpectedFail"}
    };
    outputs[EXPECTED_FAIL] = {
        true:  {s: "TEST-UNEXPECTED-PASS"       , n: "UnexpectedPass"},
        false: {s: "TEST-KNOWN-FAIL"            , n: "KnownFail"}
    };
    outputs[EXPECTED_RANDOM] = {
        true:  {s: "TEST-PASS" + randomMsg      , n: "Random"},
        false: {s: "TEST-KNOWN-FAIL" + randomMsg, n: "Random"}
    };
    var output;

    if (gURLs[0].type == TYPE_LOAD) {
        ++gTestResults.LoadOnly;
        gDumpLog("REFTEST TEST-PASS | " + gURLs[0].prettyPath + " | (LOAD ONLY)\n");
        gCurrentCanvas = null;
        FinishTestItem();
        return;
    }
    if (gURLs[0].type == TYPE_SCRIPT) {
        var missing_msg = false;
        var testwindow = gBrowser.contentWindow;
        expected = gURLs[0].expected;

        if (testwindow.wrappedJSObject)
            testwindow = testwindow.wrappedJSObject;

        var testcases;

        if (!testwindow.getTestCases || typeof testwindow.getTestCases != "function") {
            
            expected = EXPECTED_PASS;
            missing_msg = "test must provide a function getTestCases(). (SCRIPT)\n";
        }
        else if (!(testcases = testwindow.getTestCases())) {
            
            expected = EXPECTED_PASS;
            missing_msg = "test's getTestCases() must return an Array-like Object. (SCRIPT)\n";
        }
        else if (testcases.length == 0) {
            
            
            
            if (!gURLs[0].allowSilentFail)
                missing_msg = "No test results reported. (SCRIPT)\n";
            else
                gDumpLog("REFTEST INFO | An expected silent failure occurred \n");
        }

        if (missing_msg) {
            output = outputs[expected][false];
            ++gTestResults[output.n];
            var result = "REFTEST " + output.s + " | " +
                gURLs[0].prettyPath + " | " + 
                missing_msg;

            gDumpLog(result);
            FinishTestItem();
            return;
        }

        var results = testcases.map(function(test) {
                return { passed: test.testPassed(), description: test.testDescription()};
            });
        var anyFailed = results.some(function(result) { return !result.passed; });
        var outputPair;
        if (anyFailed && expected == EXPECTED_FAIL) {
            
            
            
            
            outputPair = { true: outputs[EXPECTED_RANDOM][true],
                           false: outputs[expected][false] };
        } else {
            outputPair = outputs[expected];
        }
        var index = 0;
        results.forEach(function(result) {
                var output = outputPair[result.passed];

                ++gTestResults[output.n];
                result = "REFTEST " + output.s + " | " +
                    gURLs[0].prettyPath + " | " + 
                    result.description + " item " + (++index) + "\n";
                gDumpLog(result);
            });

        if (anyFailed && expected == EXPECTED_PASS) {
            FlushTestLog();
        }

        FinishTestItem();
        return;
    }

    if (gURICanvases[gCurrentURL]) {
        gCurrentCanvas = gURICanvases[gCurrentURL];
    }
    if (gState == 1) {
        gCanvas1 = gCurrentCanvas;
    } else {
        gCanvas2 = gCurrentCanvas;
    }
    gCurrentCanvas = null;

    resetZoom();

    switch (gState) {
        case 1:
            
            

            StartCurrentURI(2);
            break;
        case 2:
            
            
            

            
            var differences;
            
            var equal;

            if (gWindowUtils) {
                differences = gWindowUtils.compareCanvases(gCanvas1, gCanvas2, {});
                equal = (differences == 0);
            } else {
                differences = -1;
                var k1 = gCanvas1.toDataURL();
                var k2 = gCanvas2.toDataURL();
                equal = (k1 == k2);
            }

            
            var test_passed = (equal == (gURLs[0].type == TYPE_REFTEST_EQUAL));
            
            var expected = gURLs[0].expected;
            output = outputs[expected][test_passed];

            ++gTestResults[output.n];

            var result = "REFTEST " + output.s + " | " +
                         gURLs[0].prettyPath + " | "; 
            switch (gURLs[0].type) {
                case TYPE_REFTEST_NOTEQUAL:
                    result += "image comparison (!=) ";
                    break;
                case TYPE_REFTEST_EQUAL:
                    result += "image comparison (==) ";
                    break;
            }
            gDumpLog(result + "\n");

            if (!test_passed && expected == EXPECTED_PASS ||
                test_passed && expected == EXPECTED_FAIL) {
                if (!equal) {
                    gDumpLog("REFTEST   IMAGE 1 (TEST): " + gCanvas1.toDataURL() + "\n");
                    gDumpLog("REFTEST   IMAGE 2 (REFERENCE): " + gCanvas2.toDataURL() + "\n");
                    gDumpLog("REFTEST number of differing pixels: " + differences + "\n");
                } else {
                    gDumpLog("REFTEST   IMAGE: " + gCanvas1.toDataURL() + "\n");
                }
            }

            if (!test_passed && expected == EXPECTED_PASS) {
                FlushTestLog();
            }

            UpdateCanvasCache(gURLs[0].url1, gCanvas1);
            UpdateCanvasCache(gURLs[0].url2, gCanvas2);

            FinishTestItem();
            break;
        default:
            throw "Unexpected state.";
    }
}

function LoadFailed()
{
    if (gTimeoutHook) {
        gTimeoutHook();
    }
    gFailureTimeout = null;
    ++gTestResults.FailedLoad;
    gDumpLog("REFTEST TEST-UNEXPECTED-FAIL | " +
         gURLs[0]["url" + gState].spec + " | " + gFailureReason + "\n");
    FlushTestLog();
    FinishTestItem();
}

function FinishTestItem()
{
    
    
    gDumpLog("REFTEST INFO | Loading a blank page\n");
    gClearingForAssertionCheck = true;
    gBrowser.loadURI(BLANK_URL_FOR_CLEARING);
}

function DoAssertionCheck()
{
    gClearingForAssertionCheck = false;

    if (gDebug.isDebugBuild) {
        var newAssertionCount = gDebug.assertionCount;
        var numAsserts = newAssertionCount - gAssertionCount;
        gAssertionCount = newAssertionCount;

        var minAsserts = gURLs[0].minAsserts;
        var maxAsserts = gURLs[0].maxAsserts;

        var expectedAssertions = "expected " + minAsserts;
        if (minAsserts != maxAsserts) {
            expectedAssertions += " to " + maxAsserts;
        }
        expectedAssertions += " assertions";

        if (numAsserts < minAsserts) {
            ++gTestResults.AssertionUnexpectedFixed;
            gDumpLog("REFTEST TEST-UNEXPECTED-PASS | " + gURLs[0].prettyPath +
                 " | assertion count " + numAsserts + " is less than " +
                 expectedAssertions + "\n");
        } else if (numAsserts > maxAsserts) {
            ++gTestResults.AssertionUnexpected;
            gDumpLog("REFTEST TEST-UNEXPECTED-FAIL | " + gURLs[0].prettyPath +
                 " | assertion count " + numAsserts + " is more than " +
                 expectedAssertions + "\n");
        } else if (numAsserts != 0) {
            ++gTestResults.AssertionKnown;
            gDumpLog("REFTEST TEST-KNOWN-FAIL | " + gURLs[0].prettyPath +
                 " | assertion count " + numAsserts + " matches " +
                 expectedAssertions + "\n");
        }
    }

    
    gURLs.shift();
    StartCurrentTest();
}
