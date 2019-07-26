





#if BOOTSTRAP
this.EXPORTED_SYMBOLS = ["OnRefTestLoad"];
#endif


const CC = Components.classes;
const CI = Components.interfaces;
const CR = Components.results;

const XHTML_NS = "http://www.w3.org/1999/xhtml";
const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";

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
const NS_DIRECTORY_SERVICE_CONTRACTID =
          "@mozilla.org/file/directory_service;1";
const NS_OBSERVER_SERVICE_CONTRACTID =
          "@mozilla.org/observer-service;1";

Components.utils.import("resource://gre/modules/FileUtils.jsm");

var gLoadTimeout = 0;
var gTimeoutHook = null;
var gRemote = false;
var gIgnoreWindowSize = false;
var gTotalChunks = 0;
var gThisChunk = 0;
var gContainingWindow = null;
var gFilter = null;


const BLANK_URL_FOR_CLEARING = "data:text/html;charset=UTF-8,%3C%21%2D%2DCLEAR%2D%2D%3E";

var gBrowser;

var gBrowserIsRemote;           

var gBrowserIsIframe;           
var gBrowserMessageManager;
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
var gCurrentURL;
var gTestLog = [];
var gServer;
var gCount = 0;
var gAssertionCount = 0;

var gIOService;
var gDebug;
var gWindowUtils;

var gSlowestTestTime = 0;
var gSlowestTestURL;

var gDrawWindowFlags;

var gExpectingProcessCrash = false;
var gExpectedCrashDumpFiles = [];
var gUnexpectedCrashDumpFiles = { };
var gCrashDumpDir;

const TYPE_REFTEST_EQUAL = '==';
const TYPE_REFTEST_NOTEQUAL = '!=';
const TYPE_LOAD = 'load';     
                              
const TYPE_SCRIPT = 'script'; 







const EXPECTED_PASS = 0;
const EXPECTED_FAIL = 1;
const EXPECTED_RANDOM = 2;
const EXPECTED_DEATH = 3;  
const EXPECTED_FUZZY = 4;


const PREF_BOOLEAN = 0;
const PREF_STRING  = 1;
const PREF_INTEGER = 2;

var gPrefsToRestore = [];

const gProtocolRE = /^\w+:/;
const gPrefItemRE = /^(|test-|ref-)pref\((.+?),(.*)\)$/;

var HTTP_SERVER_PORT = 4444;
const HTTP_SERVER_PORTS_TO_TRY = 50;


var gRunSlowTests = true;


var gNoCanvasCache = false;

var gRecycledCanvases = new Array();


var gDumpLog = dump;
var gVerbose = false;



var gDumpedConditionSandbox = false;

function LogWarning(str)
{
    gDumpLog("REFTEST INFO | " + str + "\n");
    gTestLog.push(str);
}

function LogInfo(str)
{
    if (gVerbose)
        gDumpLog("REFTEST INFO | " + str + "\n");
    gTestLog.push(str);
}

function FlushTestLog()
{
    if (!gVerbose) {
        
        for (var i = 0; i < gTestLog.length; ++i) {
            gDumpLog("REFTEST INFO | Saved log: " + gTestLog[i] + "\n");
        }
    }
    gTestLog = [];
}

function AllocateCanvas()
{
    if (gRecycledCanvases.length > 0)
        return gRecycledCanvases.shift();

    var canvas = gContainingWindow.document.createElementNS(XHTML_NS, "canvas");
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

this.OnRefTestLoad = function OnRefTestLoad(win)
{
    gCrashDumpDir = CC[NS_DIRECTORY_SERVICE_CONTRACTID]
                    .getService(CI.nsIProperties)
                    .get("ProfD", CI.nsIFile);
    gCrashDumpDir.append("minidumps");

    var env = CC["@mozilla.org/process/environment;1"].
              getService(CI.nsIEnvironment);
    gVerbose = !!env.get("MOZ_REFTEST_VERBOSE");

    var prefs = Components.classes["@mozilla.org/preferences-service;1"].
                getService(Components.interfaces.nsIPrefBranch);
    try {
        gBrowserIsRemote = prefs.getBoolPref("browser.tabs.remote");
    } catch (e) {
        gBrowserIsRemote = false;
    }

    try {
      gBrowserIsIframe = prefs.getBoolPref("reftest.browser.iframe.enabled");
    } catch (e) {
      gBrowserIsIframe = false;
    }

    if (win === undefined || win == null) {
      win = window;
    }
    if (gContainingWindow == null && win != null) {
      gContainingWindow = win;
    }

    if (gBrowserIsIframe) {
      gBrowser = gContainingWindow.document.createElementNS(XHTML_NS, "iframe");
      gBrowser.setAttribute("mozbrowser", "");
    } else {
      gBrowser = gContainingWindow.document.createElementNS(XUL_NS, "xul:browser");
    }
    gBrowser.setAttribute("id", "browser");
    gBrowser.setAttribute("type", "content-primary");
    gBrowser.setAttribute("remote", gBrowserIsRemote ? "true" : "false");
    
    
    gBrowser.setAttribute("style", "min-width: 800px; min-height: 1000px; max-width: 800px; max-height: 1000px");

#if BOOTSTRAP
#if REFTEST_B2G
    var doc = gContainingWindow.document.getElementsByTagName("window")[0];
#else
    var doc = gContainingWindow.document.getElementById('main-window');
#endif
    while (doc.hasChildNodes()) {
      doc.removeChild(doc.firstChild);
    }
    doc.appendChild(gBrowser);
#else
    document.getElementById("reftest-window").appendChild(gBrowser);
#endif

    gBrowserMessageManager = gBrowser.QueryInterface(CI.nsIFrameLoaderOwner)
                                     .frameLoader.messageManager;
    
    
    RegisterMessageListenersAndLoadContentScript();
}

function InitAndStartRefTests()
{
    
    try {
        var prefs = Components.classes["@mozilla.org/preferences-service;1"].
                    getService(Components.interfaces.nsIPrefBranch);
    } catch(e) {
        gDumpLog("REFTEST TEST-UNEXPECTED-FAIL | | EXCEPTION: " + e + "\n");
    }

    try {
      prefs.setBoolPref("android.widget_paints_background", false);
    } catch (e) {}

    
    try {
        gLoadTimeout = prefs.getIntPref("reftest.timeout");
    } catch(e) {
        gLoadTimeout = 5 * 60 * 1000; 
    }

    
    try {
        var logFile = prefs.getCharPref("reftest.logFile");
        if (logFile) {
            try {
                var f = FileUtils.File(logFile);
                var mfl = FileUtils.openFileOutputStream(f, FileUtils.MODE_WRONLY | FileUtils.MODE_CREATE);
                
                gDumpLog = function (msg) {
#if BOOTSTRAP
#if REFTEST_B2G
                    dump(msg);
#else
                    
#endif
#else
                    dump(msg);
#endif
                    mfl.write(msg, msg.length);
                };
            }
            catch(e) {
                
                gDumpLog = dump;
            }
        }
    } catch(e) {}

    try {
        gRemote = prefs.getBoolPref("reftest.remote");
    } catch(e) {
        gRemote = false;
    }

    try {
        gIgnoreWindowSize = prefs.getBoolPref("reftest.ignoreWindowSize");
    } catch(e) {
        gIgnoreWindowSize = false;
    }

    
    try {
        gTotalChunks = prefs.getIntPref("reftest.totalChunks");
        gThisChunk = prefs.getIntPref("reftest.thisChunk");
    }
    catch(e) {
        gTotalChunks = 0;
        gThisChunk = 0;
    }

    try {
        gFilter = new RegExp(prefs.getCharPref("reftest.filter"));
    } catch(e) {}

    gWindowUtils = gContainingWindow.QueryInterface(CI.nsIInterfaceRequestor).getInterface(CI.nsIDOMWindowUtils);
    if (!gWindowUtils || !gWindowUtils.compareCanvases)
        throw "nsIDOMWindowUtils inteface missing";

    gIOService = CC[IO_SERVICE_CONTRACTID].getService(CI.nsIIOService);
    gDebug = CC[DEBUG_CONTRACTID].getService(CI.nsIDebug2);

    RegisterProcessCrashObservers();

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
    var uri;
#if BOOTSTRAP
    
    try {
        var prefs = Components.classes["@mozilla.org/preferences-service;1"].
                    getService(Components.interfaces.nsIPrefBranch);
    } catch(e) {
        gDumpLog("REFTEST TEST-UNEXPECTED-FAIL | | EXCEPTION: " + e + "\n");
    }

    try {
        gNoCanvasCache = prefs.getIntPref("reftest.nocache");
    } catch(e) {
        gNoCanvasCache = false;
    }

    try {
        gRunSlowTests = prefs.getIntPref("reftest.skipslowtests");
    } catch(e) {
        gRunSlowTests = false;
    }

    try {
        uri = prefs.getCharPref("reftest.uri");
    } catch(e) {
        uri = "";
    }

    if (uri == "") {
        gDumpLog("REFTEST TEST-UNEXPECTED-FAIL | | Unable to find reftest.uri pref.  Please ensure your profile is setup properly\n");
        DoneTests();
    }
#else
    try {
        
        var args = window.arguments[0].wrappedJSObject;

        if ("nocache" in args && args["nocache"])
            gNoCanvasCache = true;

        if ("skipslowtests" in args && args.skipslowtests)
            gRunSlowTests = false;

        uri = args.uri;
    } catch (e) {
        ++gTestResults.Exception;
        gDumpLog("REFTEST TEST-UNEXPECTED-FAIL | | EXCEPTION: " + ex + "\n");
        DoneTests();
    }
#endif
    try {
        ReadTopManifest(uri);
        BuildUseCounts();

        
        
        var tURLs = new Array();
        for (var i = 0; i < gURLs.length; ++i) {
            if (gURLs[i].expected == EXPECTED_DEATH)
                continue;

            if (gURLs[i].needsFocus && !Focus())
                continue;

            if (gURLs[i].slow && !gRunSlowTests)
                continue;

            tURLs.push(gURLs[i]);
        }

        gDumpLog("REFTEST INFO | Discovered " + gURLs.length + " tests, after filtering SKIP tests, we have " + tURLs.length + "\n");

        if (gTotalChunks > 0 && gThisChunk > 0) {
            
            
            var testsPerChunk = tURLs.length / gTotalChunks;
            var start = Math.round((gThisChunk-1) * testsPerChunk);
            var end = Math.round(gThisChunk * testsPerChunk);

            
            
            start = gThisChunk == 1 ? 0 : gURLs.indexOf(tURLs[start]);
            end = gThisChunk == gTotalChunks ? gURLs.length : gURLs.indexOf(tURLs[end + 1]) - 1;
            gURLs = gURLs.slice(start, end);

            gDumpLog("REFTEST INFO | Running chunk " + gThisChunk + " out of " + gTotalChunks + " chunks.  ");
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
    sandbox.xulRuntime = {widgetToolkit: xr.widgetToolkit, OS: xr.OS, __exposedProps__: { widgetToolkit: "r", OS: "r", XPCOMABI: "r", shell: "r" } };

    
    
    try {
        sandbox.xulRuntime.XPCOMABI = xr.XPCOMABI;
    } catch(e) {
        sandbox.xulRuntime.XPCOMABI = "";
    }

    var testRect = gBrowser.getBoundingClientRect();
    sandbox.smallScreen = false;
    if (gContainingWindow.innerWidth < 800 || gContainingWindow.innerHeight < 1000) {
        sandbox.smallScreen = true;
    }

#if REFTEST_B2G
    
    sandbox.d2d = false;
    sandbox.azureQuartz = false;
    sandbox.azureSkia = false;
    sandbox.contentSameGfxBackendAsCanvas = false;
#else
    var gfxInfo = (NS_GFXINFO_CONTRACTID in CC) && CC[NS_GFXINFO_CONTRACTID].getService(CI.nsIGfxInfo);
    try {
      sandbox.d2d = gfxInfo.D2DEnabled;
    } catch (e) {
      sandbox.d2d = false;
    }
    var info = gfxInfo.getInfo();
    sandbox.azureQuartz = info.AzureCanvasBackend == "quartz";
    sandbox.azureSkia = info.AzureCanvasBackend == "skia";
    
    sandbox.contentSameGfxBackendAsCanvas = info.AzureContentBackend == info.AzureCanvasBackend
                                            || (info.AzureContentBackend == "none" && info.AzureCanvasBackend == "cairo");
#endif

    sandbox.layersGPUAccelerated =
      gWindowUtils.layerManagerType != "Basic";
    sandbox.layersOpenGL =
      gWindowUtils.layerManagerType == "OpenGL";

    
    sandbox.B2G = xr.widgetToolkit == "gonk";
    sandbox.Android = xr.OS == "Android" && !sandbox.B2G;
    sandbox.cocoaWidget = xr.widgetToolkit == "cocoa";
    sandbox.gtk2Widget = xr.widgetToolkit == "gtk2";
    sandbox.qtWidget = xr.widgetToolkit == "qt";
    sandbox.winWidget = xr.widgetToolkit == "windows";

#if MOZ_ASAN
    sandbox.AddressSanitizer = true;
#else
    sandbox.AddressSanitizer = false;
#endif

    var hh = CC[NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX + "http"].
                 getService(CI.nsIHttpProtocolHandler);
    sandbox.http = { __exposedProps__: {} };
    for each (var prop in [ "userAgent", "appName", "appVersion",
                            "vendor", "vendorSub",
                            "product", "productSub",
                            "platform", "oscpu", "language", "misc" ]) {
        sandbox.http[prop] = hh[prop];
        sandbox.http.__exposedProps__[prop] = "r";
    }

    
    var osxmatch = /Mac OS X (\d+.\d+)$/.exec(hh.oscpu);
    sandbox.OSX = osxmatch ? parseFloat(osxmatch[1]) : 0;

    
    
    sandbox.haveTestPlugin = false;

    var navigator = gContainingWindow.navigator;
    for (var i = 0; i < navigator.mimeTypes.length; i++) {
        if (navigator.mimeTypes[i].type == "application/x-test" &&
            navigator.mimeTypes[i].enabledPlugin != null &&
            navigator.mimeTypes[i].enabledPlugin.name == "Test Plug-in") {
            sandbox.haveTestPlugin = true;
            break;
        }
    }

    
    var box = gContainingWindow.document.createElement("box");
    box.setAttribute("id", "_box_windowsDefaultTheme");
    gContainingWindow.document.documentElement.appendChild(box);
    sandbox.windowsDefaultTheme = (gContainingWindow.getComputedStyle(box, null).display == "none");
    gContainingWindow.document.documentElement.removeChild(box);

    var prefs = CC["@mozilla.org/preferences-service;1"].
                getService(CI.nsIPrefBranch);
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
        try {
            netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
        } catch (ex) {}

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

    
    
    sandbox.browserIsRemote = gBrowserIsRemote;
    sandbox.bug685516 = sandbox.browserIsRemote && sandbox.Android;

    
    sandbox.xulFennec    = sandbox.Android &&  sandbox.browserIsRemote;
    sandbox.nativeFennec = sandbox.Android && !sandbox.browserIsRemote;

    if (!gDumpedConditionSandbox) {
        dump("REFTEST INFO | Dumping JSON representation of sandbox \n");
        dump("REFTEST INFO | " + JSON.stringify(sandbox) + " \n");
        gDumpedConditionSandbox = true;
    }
    return sandbox;
}

function AddPrefSettings(aWhere, aPrefName, aPrefValExpression, aSandbox, aTestPrefSettings, aRefPrefSettings)
{
    var prefVal = Components.utils.evalInSandbox("(" + aPrefValExpression + ")", aSandbox);
    var prefType;
    var valType = typeof(prefVal);
    if (valType == "boolean") {
        prefType = PREF_BOOLEAN;
    } else if (valType == "string") {
        prefType = PREF_STRING;
    } else if (valType == "number" && (parseInt(prefVal) == prefVal)) {
        prefType = PREF_INTEGER;
    } else {
        return false;
    }
    var setting = { name: aPrefName,
                    type: prefType,
                    value: prefVal };
    if (aWhere != "ref-") {
        aTestPrefSettings.push(setting);
    }
    if (aWhere != "test-") {
        aRefPrefSettings.push(setting);
    }
    return true;
}

function ReadTopManifest(aFileURL)
{
    gURLs = new Array();
    var url = gIOService.newURI(aFileURL, null, null);
    if (!url)
        throw "Expected a file or http URL for the manifest.";
    ReadManifest(url, EXPECTED_PASS);
}

function AddTestItem(aTest)
{
    if (gFilter && !gFilter.test(aTest.url1.spec))
        return;
    gURLs.push(aTest);
}



function ReadManifest(aURL, inherited_status)
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
    var lines = streamBuf.split(/\n|\r|\r\n/);

    
    var sandbox = BuildConditionSandbox(aURL);
    var lineNo = 0;
    var urlprefix = "";
    var defaultTestPrefSettings = [], defaultRefPrefSettings = [];
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

        if (items[0] == "default-preferences") {
            var m;
            var item;
            defaultTestPrefSettings = [];
            defaultRefPrefSettings = [];
            items.shift();
            while ((item = items.shift())) {
                if (!(m = item.match(gPrefItemRE))) {
                    throw "Unexpected item in default-preferences list in manifest file " + aURL.spec + " line " + lineNo;
                }
                if (!AddPrefSettings(m[1], m[2], m[3], sandbox, defaultTestPrefSettings, defaultRefPrefSettings)) {
                    throw "Error in pref value in manifest file " + aURL.spec + " line " + lineNo;
                }
            }
            continue;
        }

        var expected_status = EXPECTED_PASS;
        var allow_silent_fail = false;
        var minAsserts = 0;
        var maxAsserts = 0;
        var needs_focus = false;
        var slow = false;
        var testPrefSettings = defaultTestPrefSettings.concat();
        var refPrefSettings = defaultRefPrefSettings.concat();
        var fuzzy_max_delta = 2;
        var fuzzy_max_pixels = 1;

        while (items[0].match(/^(fails|needs-focus|random|skip|asserts|slow|require-or|silentfail|pref|test-pref|ref-pref|fuzzy)/)) {
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
            } else if (item == "needs-focus") {
                needs_focus = true;
                cond = false;
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
            } else if ((m = item.match(/^require-or\((.*?)\)$/))) {
                var args = m[1].split(/,/);
                if (args.length != 2) {
                    throw "Error 7 in manifest file " + aURL.spec + " line " + lineNo + ": wrong number of args to require-or";
                }
                var [precondition_str, fallback_action] = args;
                var preconditions = precondition_str.split(/&&/);
                cond = false;
                for each (var precondition in preconditions) {
                    if (precondition === "debugMode") {
                        
                        
                        stat = fallback_action;
                        cond = true;
                        break;
                    } else if (precondition === "true") {
                        
                    } else {
                        
                        stat = fallback_action;
                        cond = true;
                        break;
                    }
                }
            } else if ((m = item.match(/^slow-if\((.*?)\)$/))) {
                cond = false;
                if (Components.utils.evalInSandbox("(" + m[1] + ")", sandbox))
                    slow = true;
            } else if (item == "silentfail") {
                cond = false;
                allow_silent_fail = true;
            } else if ((m = item.match(gPrefItemRE))) {
                cond = false;
                if (!AddPrefSettings(m[1], m[2], m[3], sandbox, testPrefSettings, refPrefSettings)) {
                    throw "Error in pref value in manifest file " + aURL.spec + " line " + lineNo;
                }
            } else if ((m = item.match(/^fuzzy\((\d+),(\d+)\)$/))) {
              cond = false;
              expected_status = EXPECTED_FUZZY;
              fuzzy_max_delta = Number(m[1]);
              fuzzy_max_pixels = Number(m[2]);
            } else if ((m = item.match(/^fuzzy-if\((.*?),(\d+),(\d+)\)$/))) {
              cond = false;
              if (Components.utils.evalInSandbox("(" + m[1] + ")", sandbox)) {
                expected_status = EXPECTED_FUZZY;
                fuzzy_max_delta = Number(m[2]);
                fuzzy_max_pixels = Number(m[3]);
              }
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

        expected_status = Math.max(expected_status, inherited_status);

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

        var principal = secMan.getSimpleCodebasePrincipal(aURL);

        if (items[0] == "include") {
            if (items.length != 2 || runHttp)
                throw "Error 2 in manifest file " + aURL.spec + " line " + lineNo;
            var incURI = gIOService.newURI(items[1], null, listURL);
            secMan.checkLoadURIWithPrincipal(principal, incURI,
                                             CI.nsIScriptSecurityManager.DISALLOW_SCRIPT);
            ReadManifest(incURI, expected_status);
        } else if (items[0] == TYPE_LOAD) {
            if (items.length != 2 ||
                (expected_status != EXPECTED_PASS &&
                 expected_status != EXPECTED_DEATH))
                throw "Error 3 in manifest file " + aURL.spec + " line " + lineNo;
            var [testURI] = runHttp
                            ? ServeFiles(principal, httpDepth,
                                         listURL, [items[1]])
                            : [gIOService.newURI(items[1], null, listURL)];
            var prettyPath = runHttp
                           ? gIOService.newURI(items[1], null, listURL).spec
                           : testURI.spec;
            secMan.checkLoadURIWithPrincipal(principal, testURI,
                                             CI.nsIScriptSecurityManager.DISALLOW_SCRIPT);
            AddTestItem({ type: TYPE_LOAD,
                          expected: expected_status,
                          allowSilentFail: allow_silent_fail,
                          prettyPath: prettyPath,
                          minAsserts: minAsserts,
                          maxAsserts: maxAsserts,
                          needsFocus: needs_focus,
                          slow: slow,
                          prefSettings1: testPrefSettings,
                          prefSettings2: refPrefSettings,
                          fuzzyMaxDelta: fuzzy_max_delta,
                          fuzzyMaxPixels: fuzzy_max_pixels,
                          url1: testURI,
                          url2: null });
        } else if (items[0] == TYPE_SCRIPT) {
            if (items.length != 2)
                throw "Error 4 in manifest file " + aURL.spec + " line " + lineNo;
            var [testURI] = runHttp
                            ? ServeFiles(principal, httpDepth,
                                         listURL, [items[1]])
                            : [gIOService.newURI(items[1], null, listURL)];
            var prettyPath = runHttp
                           ? gIOService.newURI(items[1], null, listURL).spec
                           : testURI.spec;
            secMan.checkLoadURIWithPrincipal(principal, testURI,
                                             CI.nsIScriptSecurityManager.DISALLOW_SCRIPT);
            AddTestItem({ type: TYPE_SCRIPT,
                          expected: expected_status,
                          allowSilentFail: allow_silent_fail,
                          prettyPath: prettyPath,
                          minAsserts: minAsserts,
                          maxAsserts: maxAsserts,
                          needsFocus: needs_focus,
                          slow: slow,
                          prefSettings1: testPrefSettings,
                          prefSettings2: refPrefSettings,
                          fuzzyMaxDelta: fuzzy_max_delta,
                          fuzzyMaxPixels: fuzzy_max_pixels,
                          url1: testURI,
                          url2: null });
        } else if (items[0] == TYPE_REFTEST_EQUAL || items[0] == TYPE_REFTEST_NOTEQUAL) {
            if (items.length != 3)
                throw "Error 5 in manifest file " + aURL.spec + " line " + lineNo;
            var [testURI, refURI] = runHttp
                                  ? ServeFiles(principal, httpDepth,
                                               listURL, [items[1], items[2]])
                                  : [gIOService.newURI(items[1], null, listURL),
                                     gIOService.newURI(items[2], null, listURL)];
            var prettyPath = runHttp
                           ? gIOService.newURI(items[1], null, listURL).spec
                           : testURI.spec;
            secMan.checkLoadURIWithPrincipal(principal, testURI,
                                             CI.nsIScriptSecurityManager.DISALLOW_SCRIPT);
            secMan.checkLoadURIWithPrincipal(principal, refURI,
                                             CI.nsIScriptSecurityManager.DISALLOW_SCRIPT);
            AddTestItem({ type: items[0],
                          expected: expected_status,
                          allowSilentFail: allow_silent_fail,
                          prettyPath: prettyPath,
                          minAsserts: minAsserts,
                          maxAsserts: maxAsserts,
                          needsFocus: needs_focus,
                          slow: slow,
                          prefSettings1: testPrefSettings,
                          prefSettings2: refPrefSettings,
                          fuzzyMaxDelta: fuzzy_max_delta,
                          fuzzyMaxPixels: fuzzy_max_pixels,
                          url1: testURI,
                          url2: refURI });
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
            if (url.prefSettings1.length == 0) {
                AddURIUseCount(gURLs[i].url1);
            }
            if (url.prefSettings2.length == 0) {
                AddURIUseCount(gURLs[i].url2);
            }
        }
    }
}

function ServeFiles(manifestPrincipal, depth, aURL, files)
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

        
        secMan.checkLoadURIWithPrincipal(manifestPrincipal, testURI,
                                         CI.nsIScriptSecurityManager.DISALLOW_SCRIPT);

        return testURI;
    }

    return files.map(FileToURI);
}


function Focus()
{
    
    
    if (gBrowserIsRemote) {
        return false;
    }

    var fm = CC["@mozilla.org/focus-manager;1"].getService(CI.nsIFocusManager);
    fm.focusedWindow = gContainingWindow;
#ifdef XP_MACOSX
    try {
        var dock = CC["@mozilla.org/widget/macdocksupport;1"].getService(CI.nsIMacDockSupport);
        dock.activateApplication(true);
    } catch(ex) {
    }
#endif 
    return true;
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
        } else if (test.needsFocus && !Focus()) {
            
            
            ++gTestResults.Skip;
            gDumpLog("REFTEST TEST-KNOWN-FAIL | " + test.url1.spec + " | (SKIPPED; COULDN'T GET FOCUS)\n");
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
        RestoreChangedPreferences();
        DoneTests();
    }
    else {
        var currentTest = gTotalTests - gURLs.length;
        gContainingWindow.document.title = "reftest: " + currentTest + " / " + gTotalTests +
            " (" + Math.floor(100 * (currentTest / gTotalTests)) + "%)";
        StartCurrentURI(1);
    }
}

function StartCurrentURI(aState)
{
    gState = aState;
    gCurrentURL = gURLs[0]["url" + aState].spec;

    RestoreChangedPreferences();

    var prefSettings = gURLs[0]["prefSettings" + aState];
    if (prefSettings.length > 0) {
        var prefs = Components.classes["@mozilla.org/preferences-service;1"].
                    getService(Components.interfaces.nsIPrefBranch);
        var badPref = undefined;
        try {
            prefSettings.forEach(function(ps) {
                var oldVal;
                if (ps.type == PREF_BOOLEAN) {
                    try {
                        oldVal = prefs.getBoolPref(ps.name);
                    } catch (e) {
                        badPref = "boolean preference '" + ps.name + "'";
                        throw "bad pref";
                    }
                } else if (ps.type == PREF_STRING) {
                    try {
                        oldVal = prefs.getCharPref(ps.name);
                    } catch (e) {
                        badPref = "string preference '" + ps.name + "'";
                        throw "bad pref";
                    }
                } else if (ps.type == PREF_INTEGER) {
                    try {
                        oldVal = prefs.getIntPref(ps.name);
                    } catch (e) {
                        badPref = "integer preference '" + ps.name + "'";
                        throw "bad pref";
                    }
                } else {
                    throw "internal error - unknown preference type";
                }
                if (oldVal != ps.value) {
                    gPrefsToRestore.push( { name: ps.name,
                                            type: ps.type,
                                            value: oldVal } );
                    var value = ps.value;
                    if (ps.type == PREF_BOOLEAN) {
                        prefs.setBoolPref(ps.name, value);
                    } else if (ps.type == PREF_STRING) {
                        prefs.setCharPref(ps.name, value);
                        value = '"' + value + '"';
                    } else if (ps.type == PREF_INTEGER) {
                        prefs.setIntPref(ps.name, value);
                    }
                    gDumpLog("SET PREFERENCE pref(" + ps.name + "," + value + ")\n");
                }
            });
        } catch (e) {
            if (e == "bad pref") {
                var test = gURLs[0];
                if (test.expected == EXPECTED_FAIL) {
                    gDumpLog("REFTEST TEST-KNOWN-FAIL | " + test.url1.spec +
                             " | (SKIPPED; " + badPref + " not known or wrong type)\n");
                    ++gTestResults.Skip;
                } else {
                    gDumpLog("REFTEST TEST-UNEXPECTED-FAIL | " + test.url1.spec +
                             " | " + badPref + " not known or wrong type\n");
                    ++gTestResults.UnexpectedFail;
                }
            } else {
                throw e;
            }
        }
        if (badPref != undefined) {
            
            gURLs.shift();

            StartCurrentTest();
            return;
        }
    }

    if (prefSettings.length == 0 &&
        gURICanvases[gCurrentURL] &&
        (gURLs[0].type == TYPE_REFTEST_EQUAL ||
         gURLs[0].type == TYPE_REFTEST_NOTEQUAL) &&
        gURLs[0].maxAsserts == 0) {
        
        
        gContainingWindow.setTimeout(RecordResult, 0);
    } else {
        var currentTest = gTotalTests - gURLs.length;
        gDumpLog("REFTEST TEST-START | " + gCurrentURL + " | " + currentTest + " / " + gTotalTests +
            " (" + Math.floor(100 * (currentTest / gTotalTests)) + "%)\n");
        LogInfo("START " + gCurrentURL);
        var type = gURLs[0].type
        if (TYPE_SCRIPT == type) {
            SendLoadScriptTest(gCurrentURL, gLoadTimeout);
        } else {
            SendLoadTest(type, gCurrentURL, gLoadTimeout);
        }
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
        let appStartup = CC["@mozilla.org/toolkit/app-startup;1"].getService(CI.nsIAppStartup);
        appStartup.quit(CI.nsIAppStartup.eForceQuit);
    }
    if (gServer) {
        gServer.stop(onStopped);
    }
    else {
        onStopped();
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
    if (gIgnoreWindowSize ||
        (0 <= testRect.left &&
         0 <= testRect.top &&
         gContainingWindow.innerWidth >= testRect.right &&
         gContainingWindow.innerHeight >= testRect.bottom)) {
        
        
        
        flags |= ctx.DRAWWINDOW_USE_WIDGET_LAYERS;
    } else if (gBrowserIsRemote) {
        gDumpLog("REFTEST TEST-UNEXPECTED-FAIL | " + gCurrentURL + " | can't drawWindow remote content\n");
        ++gTestResults.Exception;
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
                 "; window size = " + gContainingWindow.innerWidth + "," + gContainingWindow.innerHeight +
                 "; test browser size = " + testRect.width + "," + testRect.height +
                 "\n");
    }

    LogInfo("DoDrawWindow " + x + "," + y + "," + w + "," + h);
    ctx.drawWindow(gContainingWindow, x, y, w, h, "rgb(255,255,255)",
                   gDrawWindowFlags);
}

function InitCurrentCanvasWithSnapshot()
{
    LogInfo("Initializing canvas snapshot");

    if (gURLs[0].type == TYPE_LOAD || gURLs[0].type == TYPE_SCRIPT) {
        
        return false;
    }

    if (!gCurrentCanvas) {
        gCurrentCanvas = AllocateCanvas();
    }

    var ctx = gCurrentCanvas.getContext("2d");
    DoDrawWindow(ctx, 0, 0, gCurrentCanvas.width, gCurrentCanvas.height);
    return true;
}

function UpdateCurrentCanvasForInvalidation(rects)
{
    LogInfo("Updating canvas for invalidation");

    if (!gCurrentCanvas) {
        return;
    }

    var ctx = gCurrentCanvas.getContext("2d");
    for (var i = 0; i < rects.length; ++i) {
        var r = rects[i];
        
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

function RecordResult(testRunTime, errorMsg, scriptResults)
{
    LogInfo("RecordResult fired");

    
    if (testRunTime > gSlowestTestTime) {
        gSlowestTestTime = testRunTime;
        gSlowestTestURL  = gCurrentURL;
    }

    
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
    outputs[EXPECTED_FUZZY] = outputs[EXPECTED_PASS];

    var output;

    if (gURLs[0].type == TYPE_LOAD) {
        ++gTestResults.LoadOnly;
        gDumpLog("REFTEST TEST-PASS | " + gURLs[0].prettyPath + " | (LOAD ONLY)\n");
        gCurrentCanvas = null;
        FinishTestItem();
        return;
    }
    if (gURLs[0].type == TYPE_SCRIPT) {
        var expected = gURLs[0].expected;

        if (errorMsg) {
            
            expected = EXPECTED_PASS;
        } else if (scriptResults.length == 0) {
             
             
             
             if (!gURLs[0].allowSilentFail)
                 errorMsg = "No test results reported. (SCRIPT)\n";
             else
                 gDumpLog("REFTEST INFO | An expected silent failure occurred \n");
        }

        if (errorMsg) {
            output = outputs[expected][false];
            ++gTestResults[output.n];
            var result = "REFTEST " + output.s + " | " +
                gURLs[0].prettyPath + " | " + 
                errorMsg;

            gDumpLog(result);
            FinishTestItem();
            return;
        }

        var anyFailed = scriptResults.some(function(result) { return !result.passed; });
        var outputPair;
        if (anyFailed && expected == EXPECTED_FAIL) {
            
            
            
            
            outputPair = { true: outputs[EXPECTED_RANDOM][true],
                           false: outputs[expected][false] };
        } else {
            outputPair = outputs[expected];
        }
        var index = 0;
        scriptResults.forEach(function(result) {
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

    if (gURLs[0]["prefSettings" + gState].length == 0 &&
        gURICanvases[gCurrentURL]) {
        gCurrentCanvas = gURICanvases[gCurrentURL];
    }
    if (gCurrentCanvas == null) {
        gDumpLog("REFTEST TEST-UNEXPECTED-FAIL | | program error managing snapshots\n");
        ++gTestResults.Exception;
    }
    if (gState == 1) {
        gCanvas1 = gCurrentCanvas;
    } else {
        gCanvas2 = gCurrentCanvas;
    }
    gCurrentCanvas = null;

    ResetRenderingState();

    switch (gState) {
        case 1:
            
            

            CleanUpCrashDumpFiles();
            StartCurrentURI(2);
            break;
        case 2:
            
            
            

            
            var differences;
            
            var equal;
            var maxDifference = {};

            differences = gWindowUtils.compareCanvases(gCanvas1, gCanvas2, maxDifference);
            equal = (differences == 0);

            
            var expected = gURLs[0].expected;

            if (maxDifference.value > 0 && maxDifference.value <= gURLs[0].fuzzyMaxDelta &&
                differences <= gURLs[0].fuzzyMaxPixels) {
                if (equal) {
                    throw "Inconsistent result from compareCanvases.";
                }
                equal = expected == EXPECTED_FUZZY;
                gDumpLog("REFTEST fuzzy match\n");
            }

            
            var test_passed = (equal == (gURLs[0].type == TYPE_REFTEST_EQUAL));

            output = outputs[expected][test_passed];

            ++gTestResults[output.n];

            var result = "REFTEST " + output.s + " | " +
                         gURLs[0].prettyPath + " | "; 
            switch (gURLs[0].type) {
                case TYPE_REFTEST_NOTEQUAL:
                    result += "image comparison (!=)";
                    break;
                case TYPE_REFTEST_EQUAL:
                    result += "image comparison (==)";
                    break;
            }

            if (!test_passed && expected == EXPECTED_PASS ||
                !test_passed && expected == EXPECTED_FUZZY ||
                test_passed && expected == EXPECTED_FAIL) {
                if (!equal) {
                    result += ", max difference: " + maxDifference.value + ", number of differing pixels: " + differences + "\n";
                    result += "REFTEST   IMAGE 1 (TEST): " + gCanvas1.toDataURL() + "\n";
                    result += "REFTEST   IMAGE 2 (REFERENCE): " + gCanvas2.toDataURL() + "\n";
                } else {
                    result += "\n";
                    gDumpLog("REFTEST   IMAGE: " + gCanvas1.toDataURL() + "\n");
                }
            } else {
                result += "\n";
            }

            gDumpLog(result);

            if (!test_passed && expected == EXPECTED_PASS) {
                FlushTestLog();
            }

            if (gURLs[0].prefSettings1.length == 0) {
                UpdateCanvasCache(gURLs[0].url1, gCanvas1);
            }
            if (gURLs[0].prefSettings2.length == 0) {
                UpdateCanvasCache(gURLs[0].url2, gCanvas2);
            }

            CleanUpCrashDumpFiles();
            FinishTestItem();
            break;
        default:
            throw "Unexpected state.";
    }
}

function LoadFailed(why)
{
    ++gTestResults.FailedLoad;
    gDumpLog("REFTEST TEST-UNEXPECTED-FAIL | " +
         gURLs[0]["url" + gState].spec + " | load failed: " + why + "\n");
    FlushTestLog();
    FinishTestItem();
}

function RemoveExpectedCrashDumpFiles()
{
    if (gExpectingProcessCrash) {
        for each (let crashFilename in gExpectedCrashDumpFiles) {
            let file = gCrashDumpDir.clone();
            file.append(crashFilename);
            if (file.exists()) {
                file.remove(false);
            }
        }
    }
    gExpectedCrashDumpFiles.length = 0;
}

function FindUnexpectedCrashDumpFiles()
{
    if (!gCrashDumpDir.exists()) {
        return;
    }

    let entries = gCrashDumpDir.directoryEntries;
    if (!entries) {
        return;
    }

    let foundCrashDumpFile = false;
    while (entries.hasMoreElements()) {
        let file = entries.getNext().QueryInterface(CI.nsIFile);
        let path = String(file.path);
        if (path.match(/\.(dmp|extra)$/) && !gUnexpectedCrashDumpFiles[path]) {
            if (!foundCrashDumpFile) {
                ++gTestResults.UnexpectedFail;
                foundCrashDumpFile = true;
                gDumpLog("REFTEST TEST-UNEXPECTED-FAIL | " + gCurrentURL +
                         " | This test left crash dumps behind, but we weren't expecting it to!\n");
            }
            gDumpLog("REFTEST INFO | Found unexpected crash dump file" + path +
                     ".\n");
            gUnexpectedCrashDumpFiles[path] = true;
        }
    }
}

function CleanUpCrashDumpFiles()
{
    RemoveExpectedCrashDumpFiles();
    FindUnexpectedCrashDumpFiles();
    gExpectingProcessCrash = false;
}

function FinishTestItem()
{
    
    
    gDumpLog("REFTEST INFO | Loading a blank page\n");
    
    
    SetAsyncScroll(false);
    SendClear();
}

function DoAssertionCheck(numAsserts)
{
    if (gDebug.isDebugBuild) {
        if (gBrowserIsRemote) {
            
            
            var newAssertionCount = gDebug.assertionCount;
            var numLocalAsserts = newAssertionCount - gAssertionCount;
            gAssertionCount = newAssertionCount;

            numAsserts += numLocalAsserts;
        }

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

function ResetRenderingState()
{
    SendResetRenderingState();
    
}

function RestoreChangedPreferences()
{
    if (gPrefsToRestore.length > 0) {
        var prefs = Components.classes["@mozilla.org/preferences-service;1"].
                    getService(Components.interfaces.nsIPrefBranch);
        gPrefsToRestore.reverse();
        gPrefsToRestore.forEach(function(ps) {
            var value = ps.value;
            if (ps.type == PREF_BOOLEAN) {
                prefs.setBoolPref(ps.name, value);
            } else if (ps.type == PREF_STRING) {
                prefs.setCharPref(ps.name, value);
                value = '"' + value + '"';
            } else if (ps.type == PREF_INTEGER) {
                prefs.setIntPref(ps.name, value);
            }
            gDumpLog("RESTORE PREFERENCE pref(" + ps.name + "," + value + ")\n");
        });
        gPrefsToRestore = [];
    }
}

function RegisterMessageListenersAndLoadContentScript()
{
    gBrowserMessageManager.addMessageListener(
        "reftest:AssertionCount",
        function (m) { RecvAssertionCount(m.json.count); }
    );
    gBrowserMessageManager.addMessageListener(
        "reftest:ContentReady",
        function (m) { return RecvContentReady() }
    );
    gBrowserMessageManager.addMessageListener(
        "reftest:Exception",
        function (m) { RecvException(m.json.what) }
    );
    gBrowserMessageManager.addMessageListener(
        "reftest:FailedLoad",
        function (m) { RecvFailedLoad(m.json.why); }
    );
    gBrowserMessageManager.addMessageListener(
        "reftest:InitCanvasWithSnapshot",
        function (m) { return RecvInitCanvasWithSnapshot(); }
    );
    gBrowserMessageManager.addMessageListener(
        "reftest:Log",
        function (m) { RecvLog(m.json.type, m.json.msg); }
    );
    gBrowserMessageManager.addMessageListener(
        "reftest:ScriptResults",
        function (m) { RecvScriptResults(m.json.runtimeMs, m.json.error, m.json.results); }
    );
    gBrowserMessageManager.addMessageListener(
        "reftest:TestDone",
        function (m) { RecvTestDone(m.json.runtimeMs); }
    );
    gBrowserMessageManager.addMessageListener(
        "reftest:UpdateCanvasForInvalidation",
        function (m) { RecvUpdateCanvasForInvalidation(m.json.rects); }
    );
    gBrowserMessageManager.addMessageListener(
        "reftest:ExpectProcessCrash",
        function (m) { RecvExpectProcessCrash(); }
    );
    gBrowserMessageManager.addMessageListener(
        "reftest:EnableAsyncScroll",
        function (m) { SetAsyncScroll(true); }
    );

    gBrowserMessageManager.loadFrameScript("chrome://reftest/content/reftest-content.js", true);
}

function SetAsyncScroll(enabled)
{
    gBrowser.QueryInterface(CI.nsIFrameLoaderOwner).frameLoader.renderMode =
        enabled ? CI.nsIFrameLoader.RENDER_MODE_ASYNC_SCROLL :
                  CI.nsIFrameLoader.RENDER_MODE_DEFAULT;
}

function RecvAssertionCount(count)
{
    DoAssertionCheck(count);
}

function RecvContentReady()
{
    InitAndStartRefTests();
    return { remote: gBrowserIsRemote };
}

function RecvException(what)
{
    gDumpLog("REFTEST TEST-UNEXPECTED-FAIL | | "+ what +"\n");
    ++gTestResults.Exception;
}

function RecvFailedLoad(why)
{
    LoadFailed(why);
}

function RecvInitCanvasWithSnapshot()
{
    var painted = InitCurrentCanvasWithSnapshot();
    return { painted: painted };
}

function RecvLog(type, msg)
{
    msg = "[CONTENT] "+ msg;
    if (type == "info") {
        LogInfo(msg);
    } else if (type == "warning") {
        LogWarning(msg);
    } else {
        gDumpLog("REFTEST TEST-UNEXPECTED-FAIL | | unknown log type "+ type +"\n");
        ++gTestResults.Exception;
    }
}

function RecvScriptResults(runtimeMs, error, results)
{
    RecordResult(runtimeMs, error, results);
}

function RecvTestDone(runtimeMs)
{
    RecordResult(runtimeMs, '', [ ]);
}

function RecvUpdateCanvasForInvalidation(rects)
{
    UpdateCurrentCanvasForInvalidation(rects);
}

function OnProcessCrashed(subject, topic, data)
{
    var id;
    subject = subject.QueryInterface(CI.nsIPropertyBag2);
    if (topic == "plugin-crashed") {
        id = subject.getPropertyAsAString("pluginDumpID");
    } else if (topic == "ipc:content-shutdown") {
        id = subject.getPropertyAsAString("dumpID");
    }
    if (id) {
        gExpectedCrashDumpFiles.push(id + ".dmp");
        gExpectedCrashDumpFiles.push(id + ".extra");
    }
}

function RegisterProcessCrashObservers()
{
    var os = CC[NS_OBSERVER_SERVICE_CONTRACTID]
             .getService(CI.nsIObserverService);
    os.addObserver(OnProcessCrashed, "plugin-crashed", false);
    os.addObserver(OnProcessCrashed, "ipc:content-shutdown", false);
}

function RecvExpectProcessCrash()
{
    gExpectingProcessCrash = true;
}

function SendClear()
{
    gBrowserMessageManager.sendAsyncMessage("reftest:Clear");
}

function SendLoadScriptTest(uri, timeout)
{
    gBrowserMessageManager.sendAsyncMessage("reftest:LoadScriptTest",
                                            { uri: uri, timeout: timeout });
}

function SendLoadTest(type, uri, timeout)
{
    gBrowserMessageManager.sendAsyncMessage("reftest:LoadTest",
                                            { type: type, uri: uri, timeout: timeout }
    );
}

function SendResetRenderingState()
{
    gBrowserMessageManager.sendAsyncMessage("reftest:ResetRenderingState");
}
