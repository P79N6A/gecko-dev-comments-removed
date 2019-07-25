





































const CC = Components.classes;
const CI = Components.interfaces;
const CR = Components.results;







var gNextTimeoutId = 0;
var gTimeoutTable = { };        

function setTimeout(callbackFn, delayMs) {
    var id = gNextTimeoutId++;
    var timer = CC["@mozilla.org/timer;1"].createInstance(CI.nsITimer);
    timer.initWithCallback({
        notify: function notify_callback() {
                    clearTimeout(id);
                    callbackFn();
                }
        },
        delayMs,
        timer.TYPE_ONE_SHOT);

    gTimeoutTable[id] = timer;

    return id;
}

function clearTimeout(id) {
    var timer = gTimeoutTable[id];
    if (timer) {
        timer.cancel();
        delete gTimeoutTable[id];
    }
}

const XHTML_NS = "http://www.w3.org/1999/xhtml";

const DEBUG_CONTRACTID = "@mozilla.org/xpcom/debug;1";
const PRINTSETTINGS_CONTRACTID = "@mozilla.org/gfx/printsettings-service;1";


const BLANK_URL_FOR_CLEARING = "data:text/html,%3C%21%2D%2DCLEAR%2D%2D%3E";

var gBrowserIsRemote;
var gHaveCanvasSnapshot = false;







var gExplicitPendingPaintCount = 0;
var gExplicitPendingPaintsCompleteHook;
var gCurrentURL;
var gCurrentTestType;
var gTimeoutHook = null;
var gFailureTimeout = null;
var gFailureReason;
var gAssertionCount = 0;

var gDebug;

var gCurrentTestStartTime;
var gClearingForAssertionCheck = false;

const TYPE_SCRIPT = 'script'; 

function markupDocumentViewer() { 
    return docShell.contentViewer.QueryInterface(CI.nsIMarkupDocumentViewer);
}

function webNavigation() {
    return docShell.QueryInterface(CI.nsIWebNavigation);
}

function windowUtils() {
    return content.QueryInterface(CI.nsIInterfaceRequestor)
                  .getInterface(CI.nsIDOMWindowUtils);
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

function OnInitialLoad()
{
    removeEventListener("load", OnInitialLoad, true);

    gDebug = CC[DEBUG_CONTRACTID].getService(CI.nsIDebug2);

    RegisterMessageListeners();

    var initInfo = SendContentReady();
    gBrowserIsRemote = initInfo.remote;

    addEventListener("load", OnDocumentLoad, true);

    addEventListener("MozPaintWait", PaintWaitListener, true);
    addEventListener("MozPaintWaitFinished", PaintWaitFinishedListener, true);
 
    LogWarning("Using browser remote="+ gBrowserIsRemote +"\n");
}

function StartTestURI(type, uri, timeout)
{
    
    
    if (gExplicitPendingPaintCount != 0) {
        LogWarning("Resetting gExplicitPendingPaintCount to zero (currently " +
                   gExplicitPendingPaintCount + "\n");
        gExplicitPendingPaintCount = 0;
    }

    gCurrentTestType = type;
    gCurrentURL = uri;

    gCurrentTestStartTime = Date.now();
    if (gFailureTimeout != null) {
        SendException("program error managing timeouts\n");
    }
    gFailureTimeout = setTimeout(LoadFailed, timeout);

    LoadURI(gCurrentURL);
}

function setupZoom(contentRootElement) {
    if (!contentRootElement || !contentRootElement.hasAttribute('reftest-zoom'))
        return;
    markupDocumentViewer().fullZoom =
        contentRootElement.getAttribute('reftest-zoom');
}

function resetZoom() {
    markupDocumentViewer().fullZoom = 1.0;
}

function doPrintMode(contentRootElement) {
    
    return contentRootElement &&
           contentRootElement.hasAttribute('class') &&
           contentRootElement.getAttribute('class').split(/\s+/)
                             .indexOf("reftest-print") != -1;
}

function setupPrintMode() {
   var PSSVC =
       CC[PRINTSETTINGS_CONTRACTID].getService(CI.nsIPrintSettingsService);
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
   docShell.contentViewer.setPageMode(true, ps);
}

function setupDisplayport(contentRootElement) {
    if (!contentRootElement) {
        return;
    }

    function attrOrDefault(attr, def) {
        return contentRootElement.hasAttribute(attr) ?
            contentRootElement.getAttribute(attr) : def;
    }

    var vw = attrOrDefault("reftest-viewport-w", 0);
    var vh = attrOrDefault("reftest-viewport-h", 0);
    if (vw !== 0 || vh !== 0) {
        LogInfo("Setting viewport to <w="+ vw +", h="+ vh +">");
        windowUtils().setCSSViewport(vw, vh);
    }

    
    var dpw = attrOrDefault("reftest-displayport-w", 0);
    var dph = attrOrDefault("reftest-displayport-h", 0);
    var dpx = attrOrDefault("reftest-displayport-x", 0);
    var dpy = attrOrDefault("reftest-displayport-y", 0);
    if (dpw !== 0 || dph !== 0) {
        LogInfo("Setting displayport to <x="+ dpx +", y="+ dpy +", w="+ dpw +", h="+ dph +">");
        windowUtils().setDisplayPortForElement(dpx, dpy, dpw, dph, content.document.documentElement);
    }

    

    
}

function resetDisplayport() {
    
    
}

function shouldWaitForExplicitPaintWaiters() {
    return gExplicitPendingPaintCount > 0;
}

function shouldWaitForPendingPaints() {
    
    
    return gHaveCanvasSnapshot && windowUtils().isMozAfterPaintPending;
}

function shouldWaitForReftestWaitRemoval(contentRootElement) {
    
    return contentRootElement &&
           contentRootElement.hasAttribute('class') &&
           contentRootElement.getAttribute('class').split(/\s+/)
                             .indexOf("reftest-wait") != -1;
}




const STATE_WAITING_TO_FIRE_INVALIDATE_EVENT = 0;


const STATE_WAITING_FOR_REFTEST_WAIT_REMOVAL = 1;


const STATE_WAITING_TO_FINISH = 2;
const STATE_COMPLETED = 3;

function WaitForTestEnd(contentRootElement, inPrintMode) {
    var stopAfterPaintReceived = false;
    var currentDoc = content.document;
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

        flushWindow(content);

        if (anyPendingPaintsGeneratedInDescendants &&
            !windowUtils().isMozAfterPaintPending) {
            LogWarning("Internal error: descendant frame generated a MozAfterPaint event, but the root document doesn't have one!");
        }
    }

    function AfterPaintListener(event) {
        LogInfo("AfterPaintListener in " + event.target.document.location.href);
        if (event.target.document != currentDoc) {
            
            
            return;
        }

        SendUpdateCanvasForEvent(event);
        
        
        
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
        
        removeEventListener("MozAfterPaint", AfterPaintListener, false);
        if (contentRootElement) {
            contentRootElement.removeEventListener("DOMAttrModified", AttrModifiedListener, false);
        }
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
            var hasReftestWait = shouldWaitForReftestWaitRemoval(contentRootElement);            
            
            LogInfo("MakeProgress: dispatching MozReftestInvalidate");
            if (contentRootElement) {
                var notification = content.document.createEvent("Events");
                notification.initEvent("MozReftestInvalidate", true, false);
                contentRootElement.dispatchEvent(notification);
            }
            if (hasReftestWait && !shouldWaitForReftestWaitRemoval(contentRootElement)) {
                
                
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
            if (shouldWaitForReftestWaitRemoval(contentRootElement)) {
                gFailureReason = "timed out waiting for reftest-wait to be removed";
                LogInfo("MakeProgress: waiting for reftest-wait to be removed");
                return;
            }
            state = STATE_WAITING_TO_FINISH;
            if (!inPrintMode && doPrintMode(contentRootElement)) {
                LogInfo("MakeProgress: setting up print mode");
                setupPrintMode();
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
            CheckForProcessCrashExpectation();
            setTimeout(RecordResult, 0);
            return;
        }
    }

    LogInfo("WaitForTestEnd: Adding listeners");
    addEventListener("MozAfterPaint", AfterPaintListener, false);
    
    
    if (contentRootElement) {
      contentRootElement.addEventListener("DOMAttrModified", AttrModifiedListener, false);
    }
    gExplicitPendingPaintsCompleteHook = ExplicitPaintsCompleteListener;
    gTimeoutHook = RemoveListeners;

    
    
    
    SendInitCanvasWithSnapshot();
    MakeProgress();
}

function OnDocumentLoad(event)
{
    var currentDoc = content.document;
    if (event.target != currentDoc)
        
        return;

    if (gClearingForAssertionCheck &&
        currentDoc.location.href == BLANK_URL_FOR_CLEARING) {
        DoAssertionCheck();
        return;
    }

    if (currentDoc.location.href != gCurrentURL) {
        LogInfo("OnDocumentLoad fired for previous document");
        
        return;
    }

    var contentRootElement = currentDoc ? currentDoc.documentElement : null;
    setupZoom(contentRootElement);
    setupDisplayport(contentRootElement);
    var inPrintMode = false;

    function AfterOnLoadScripts() {
        
        
        
        
        var painted = SendInitCanvasWithSnapshot();

        if (shouldWaitForExplicitPaintWaiters() ||
            (!inPrintMode && doPrintMode(contentRootElement)) ||
            
            
            
            !painted) {
            LogInfo("AfterOnLoadScripts belatedly entering WaitForTestEnd");
            
            WaitForTestEnd(contentRootElement, inPrintMode);
        } else {
            CheckForProcessCrashExpectation();
            RecordResult();
        }
    }

    if (shouldWaitForReftestWaitRemoval(contentRootElement) ||
        shouldWaitForExplicitPaintWaiters()) {
        
        
        gFailureReason = "timed out waiting for test to complete (trying to get into WaitForTestEnd)";
        LogInfo("OnDocumentLoad triggering WaitForTestEnd");
        setTimeout(function () { WaitForTestEnd(contentRootElement, inPrintMode); }, 0);
    } else {
        if (doPrintMode(contentRootElement)) {
            LogInfo("OnDocumentLoad setting up print mode");
            setupPrintMode();
            inPrintMode = true;
        }

        
        
        
        
        gFailureReason = "timed out waiting for test to complete (waiting for onload scripts to complete)";
        LogInfo("OnDocumentLoad triggering AfterOnLoadScripts");
        setTimeout(function () { setTimeout(AfterOnLoadScripts, 0); }, 0);
    }
}

function CheckForProcessCrashExpectation()
{
    var contentRootElement = content.document.documentElement;
    if (contentRootElement &&
        contentRootElement.hasAttribute('class') &&
        contentRootElement.getAttribute('class').split(/\s+/)
                          .indexOf("reftest-expect-process-crash") != -1) {
        SendExpectProcessCrash();
    }
}

function RecordResult()
{
    LogInfo("RecordResult fired");

    var currentTestRunTime = Date.now() - gCurrentTestStartTime;

    clearTimeout(gFailureTimeout);
    gFailureReason = null;
    gFailureTimeout = null;

    if (gCurrentTestType == TYPE_SCRIPT) {
        var error = '';
        var testwindow = content;

        if (testwindow.wrappedJSObject)
            testwindow = testwindow.wrappedJSObject;

        var testcases;
        if (!testwindow.getTestCases || typeof testwindow.getTestCases != "function") {
            
            error = "test must provide a function getTestCases(). (SCRIPT)\n";
        }
        else if (!(testcases = testwindow.getTestCases())) {
            
            error = "test's getTestCases() must return an Array-like Object. (SCRIPT)\n";
        }
        else if (testcases.length == 0) {
            
            
            
        }

        var results = [ ];
        if (!error) {
            
            for (var i = 0; i < testcases.length; ++i) {
                var test = testcases[i];
                results.push({ passed: test.testPassed(),
                               description: test.testDescription() });
            }
            
            
            
        }

        SendScriptResults(currentTestRunTime, error, results);
        FinishTestItem();
        return;
    }

    SendTestDone(currentTestRunTime);
    FinishTestItem();
}

function LoadFailed()
{
    if (gTimeoutHook) {
        gTimeoutHook();
    }
    gFailureTimeout = null;
    SendFailedLoad(gFailureReason);
}

function FinishTestItem()
{
    gHaveCanvasSnapshot = false;
}

function DoAssertionCheck()
{
    gClearingForAssertionCheck = false;

    var numAsserts = 0;
    if (gDebug.isDebugBuild) {
        var newAssertionCount = gDebug.assertionCount;
        numAsserts = newAssertionCount - gAssertionCount;
        gAssertionCount = newAssertionCount;
    }
    SendAssertionCount(numAsserts);
}

function LoadURI(uri)
{
    var flags = webNavigation().LOAD_FLAGS_NONE;
    webNavigation().loadURI(uri, flags, null, null, null);
}

function LogWarning(str)
{
    sendAsyncMessage("reftest:Log", { type: "warning", msg: str });
}

function LogInfo(str)
{
    sendAsyncMessage("reftest:Log", { type: "info", msg: str });
}

const SYNC_DEFAULT = 0x0;
const SYNC_ALLOW_DISABLE = 0x1;
var gDummyCanvas = null;
function SynchronizeForSnapshot(flags)
{
    if (flags & SYNC_ALLOW_DISABLE) {
        var docElt = content.document.documentElement;
        if (docElt && docElt.hasAttribute("reftest-no-sync-layers")) {
            LogInfo("Test file chose to skip SynchronizeForSnapshot");
            return;
        }
    }

    if (gDummyCanvas == null) {
        gDummyCanvas = content.document.createElementNS(XHTML_NS, "canvas");
        gDummyCanvas.setAttribute("width", 1);
        gDummyCanvas.setAttribute("height", 1);
    }

    var ctx = gDummyCanvas.getContext("2d");
    var flags = ctx.DRAWWINDOW_DRAW_CARET | ctx.DRAWWINDOW_DRAW_VIEW | ctx.DRAWWINDOW_USE_WIDGET_LAYERS;
    ctx.drawWindow(content, 0, 0, 1, 1, "rgb(255,255,255)", flags);
}

function RegisterMessageListeners()
{
    addMessageListener(
        "reftest:Clear",
        function (m) { RecvClear() }
    );
    addMessageListener(
        "reftest:LoadScriptTest",
        function (m) { RecvLoadScriptTest(m.json.uri, m.json.timeout); }
    );
    addMessageListener(
        "reftest:LoadTest",
        function (m) { RecvLoadTest(m.json.type, m.json.uri, m.json.timeout); }
    );
    addMessageListener(
        "reftest:ResetRenderingState",
        function (m) { RecvResetRenderingState(); }
    );
}

function RecvClear()
{
    gClearingForAssertionCheck = true;
    LoadURI(BLANK_URL_FOR_CLEARING);
}

function RecvLoadTest(type, uri, timeout)
{
    StartTestURI(type, uri, timeout);
}

function RecvLoadScriptTest(uri, timeout)
{
    StartTestURI(TYPE_SCRIPT, uri, timeout);
}

function RecvResetRenderingState()
{
    resetZoom();
    resetDisplayport();
}

function SendAssertionCount(numAssertions)
{
    sendAsyncMessage("reftest:AssertionCount", { count: numAssertions });
}

function SendContentReady()
{
    return sendSyncMessage("reftest:ContentReady")[0];
}

function SendException(what)
{
    sendAsyncMessage("reftest:Exception", { what: what });
}

function SendFailedLoad(why)
{
    sendAsyncMessage("reftest:FailedLoad", { why: why });
}


function SendInitCanvasWithSnapshot()
{
    
    
    
    
    
    
    if (gBrowserIsRemote) {
        SynchronizeForSnapshot(SYNC_DEFAULT);
    }

    
    
    
    
    
    
    var ret = sendSyncMessage("reftest:InitCanvasWithSnapshot")[0];
 
    gHaveCanvasSnapshot = ret.painted;
    return ret.painted;
}

function SendScriptResults(runtimeMs, error, results)
{
    sendAsyncMessage("reftest:ScriptResults",
                     { runtimeMs: runtimeMs, error: error, results: results });
}

function SendExpectProcessCrash(runtimeMs)
{
    sendAsyncMessage("reftest:ExpectProcessCrash");
}

function SendTestDone(runtimeMs)
{
    sendAsyncMessage("reftest:TestDone", { runtimeMs: runtimeMs });
}

function roundTo(x, fraction)
{
    return Math.round(x/fraction)*fraction;
}

function SendUpdateCanvasForEvent(event)
{
    var win = content;
    var scale = markupDocumentViewer().fullZoom;
 
    var rects = [ ];
    var rectList = event.clientRects;
    for (var i = 0; i < rectList.length; ++i) {
        var r = rectList[i];
        
        var left = Math.floor(roundTo(r.left*scale, 0.001));
        var top = Math.floor(roundTo(r.top*scale, 0.001));
        var right = Math.ceil(roundTo(r.right*scale, 0.001));
        var bottom = Math.ceil(roundTo(r.bottom*scale, 0.001));

        rects.push({ left: left, top: top, right: right, bottom: bottom });
    }

    
    
    if (!gBrowserIsRemote) {
        sendSyncMessage("reftest:UpdateCanvasForInvalidation", { rects: rects });
    } else {
        SynchronizeForSnapshot(SYNC_ALLOW_DISABLE);
        sendAsyncMessage("reftest:UpdateCanvasForInvalidation", { rects: rects });
    }
}

addEventListener("load", OnInitialLoad, true);
