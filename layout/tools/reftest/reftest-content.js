





const CC = Components.classes;
const CI = Components.interfaces;
const CR = Components.results;
const CU = Components.utils;

const XHTML_NS = "http://www.w3.org/1999/xhtml";

const DEBUG_CONTRACTID = "@mozilla.org/xpcom/debug;1";
const PRINTSETTINGS_CONTRACTID = "@mozilla.org/gfx/printsettings-service;1";
const ENVIRONMENT_CONTRACTID = "@mozilla.org/process/environment;1";


const BLANK_URL_FOR_CLEARING = "data:text/html;charset=UTF-8,%3C%21%2D%2DCLEAR%2D%2D%3E";

CU.import("resource://gre/modules/Timer.jsm");
CU.import("resource://gre/modules/AsyncSpellCheckTestHelper.jsm");

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
var gVerbose = false;

var gCurrentTestStartTime;
var gClearingForAssertionCheck = false;

const TYPE_LOAD = 'load';  
                           
const TYPE_SCRIPT = 'script'; 

function markupDocumentViewer() {
    return docShell.contentViewer;
}

function webNavigation() {
    return docShell.QueryInterface(CI.nsIWebNavigation);
}

function windowUtilsForWindow(w) {
    return w.QueryInterface(CI.nsIInterfaceRequestor)
            .getInterface(CI.nsIDOMWindowUtils);
}

function windowUtils() {
    return windowUtilsForWindow(content);
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
#ifndef REFTEST_B2G
    removeEventListener("load", OnInitialLoad, true);
#endif

    gDebug = CC[DEBUG_CONTRACTID].getService(CI.nsIDebug2);
    var env = CC[ENVIRONMENT_CONTRACTID].getService(CI.nsIEnvironment);
    gVerbose = !!env.get("MOZ_REFTEST_VERBOSE");

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
#if REFTEST_B2G
    
    return false;
#else
    
    return contentRootElement &&
           contentRootElement.hasAttribute('class') &&
           contentRootElement.getAttribute('class').split(/\s+/)
                             .indexOf("reftest-print") != -1;
#endif
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

function attrOrDefault(element, attr, def) {
    return element.hasAttribute(attr) ? Number(element.getAttribute(attr)) : def;
}

function setupViewport(contentRootElement) {
    if (!contentRootElement) {
        return;
    }

    var vw = attrOrDefault(contentRootElement, "reftest-viewport-w", 0);
    var vh = attrOrDefault(contentRootElement, "reftest-viewport-h", 0);
    if (vw !== 0 || vh !== 0) {
        LogInfo("Setting viewport to <w="+ vw +", h="+ vh +">");
        windowUtils().setCSSViewport(vw, vh);
    }

    var sw = attrOrDefault(contentRootElement, "reftest-scrollport-w", 0);
    var sh = attrOrDefault(contentRootElement, "reftest-scrollport-h", 0);
    if (sw !== 0 || sh !== 0) {
        LogInfo("Setting scrollport to <w=" + sw + ", h=" + sh + ">");
        windowUtils().setScrollPositionClampingScrollPortSize(sw, sh);
    }

    

    
}
 
function setupDisplayport(contentRootElement) {
    if (!contentRootElement) {
        return;
    }

    function setupDisplayportForElement(element, winUtils) {
        var dpw = attrOrDefault(element, "reftest-displayport-w", 0);
        var dph = attrOrDefault(element, "reftest-displayport-h", 0);
        var dpx = attrOrDefault(element, "reftest-displayport-x", 0);
        var dpy = attrOrDefault(element, "reftest-displayport-y", 0);
        if (dpw !== 0 || dph !== 0 || dpx != 0 || dpy != 0) {
            LogInfo("Setting displayport to <x="+ dpx +", y="+ dpy +", w="+ dpw +", h="+ dph +">");
            winUtils.setDisplayPortForElement(dpx, dpy, dpw, dph, element, 1);
        }
    }

    function setupDisplayportForElementSubtree(element, winUtils) {
        setupDisplayportForElement(element, winUtils);
        for (var c = element.firstElementChild; c; c = c.nextElementSibling) {
            setupDisplayportForElementSubtree(c, winUtils);
        }
        if (element.contentDocument) {
            LogInfo("Descending into subdocument");
            setupDisplayportForElementSubtree(element.contentDocument.documentElement,
                                              windowUtilsForWindow(element.contentWindow));
        }
    }

    if (contentRootElement.hasAttribute("reftest-async-scroll")) {
        setupDisplayportForElementSubtree(contentRootElement, windowUtils());
    } else {
        setupDisplayportForElement(contentRootElement, windowUtils());
    }
}


function setupAsyncScrollOffsets(options) {
    var currentDoc = content.document;
    var contentRootElement = currentDoc ? currentDoc.documentElement : null;

    if (!contentRootElement) {
        return false;
    }

    function setupAsyncScrollOffsetsForElement(element, winUtils) {
        var sx = attrOrDefault(element, "reftest-async-scroll-x", 0);
        var sy = attrOrDefault(element, "reftest-async-scroll-y", 0);
        if (sx != 0 || sy != 0) {
            try {
                
                
                winUtils.setAsyncScrollOffset(element, sx, sy);
                return true;
            } catch (e) {
                if (!options.allowFailure) {
                    throw e;
                }
            }
        }
        return false;
    }

    function setupAsyncScrollOffsetsForElementSubtree(element, winUtils) {
        var updatedAny = setupAsyncScrollOffsetsForElement(element, winUtils);
        for (var c = element.firstElementChild; c; c = c.nextElementSibling) {
            if (setupAsyncScrollOffsetsForElementSubtree(c, winUtils)) {
                updatedAny = true;
            }
        }
        if (element.contentDocument) {
            LogInfo("Descending into subdocument (async offsets)");
            if (setupAsyncScrollOffsetsForElementSubtree(element.contentDocument.documentElement,
                                                         windowUtilsForWindow(element.contentWindow))) {
                updatedAny = true;
            }
        }
        return updatedAny;
    }

    var asyncScroll = contentRootElement.hasAttribute("reftest-async-scroll");
    if (asyncScroll) {
        return setupAsyncScrollOffsetsForElementSubtree(contentRootElement, windowUtils());
    }
    return false;
}

function resetDisplayportAndViewport() {
    
    
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

function shouldSnapshotWholePage(contentRootElement) {
    
    return contentRootElement &&
           contentRootElement.hasAttribute('class') &&
           contentRootElement.getAttribute('class').split(/\s+/)
                             .indexOf("reftest-snapshot-all") != -1;
}

function getNoPaintElements(contentRootElement) {
    return contentRootElement.getElementsByClassName('reftest-no-paint');
}

function getOpaqueLayerElements(contentRootElement) {
    return contentRootElement.getElementsByClassName('reftest-opaque-layer');
}

function getAssignedLayerMap(contentRootElement) {
    var layerNameToElementsMap = {};
    var elements = contentRootElement.querySelectorAll('[reftest-assigned-layer]');
    for (var i = 0; i < elements.length; ++i) {
        var element = elements[i];
        var layerName = element.getAttribute('reftest-assigned-layer');
        if (!(layerName in layerNameToElementsMap)) {
            layerNameToElementsMap[layerName] = [];
        }
        layerNameToElementsMap[layerName].push(element);
    }
    return layerNameToElementsMap;
}




const STATE_WAITING_TO_FIRE_INVALIDATE_EVENT = 0;


const STATE_WAITING_FOR_REFTEST_WAIT_REMOVAL = 1;


const STATE_WAITING_FOR_SPELL_CHECKS = 2;


const STATE_WAITING_TO_FINISH = 3;
const STATE_COMPLETED = 4;

function FlushRendering() {
    var anyPendingPaintsGeneratedInDescendants = false;

    function flushWindow(win) {
        var utils = win.QueryInterface(CI.nsIInterfaceRequestor)
                    .getInterface(CI.nsIDOMWindowUtils);
        var afterPaintWasPending = utils.isMozAfterPaintPending;

        if (win.document.documentElement) {
            try {
                
                win.document.documentElement.getBoundingClientRect();
            } catch (e) {
                LogWarning("flushWindow failed: " + e + "\n");
            }
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

function WaitForTestEnd(contentRootElement, inPrintMode, spellCheckedElements) {
    var stopAfterPaintReceived = false;
    var currentDoc = content.document;
    var state = STATE_WAITING_TO_FIRE_INVALIDATE_EVENT;

    function AfterPaintListener(event) {
        LogInfo("AfterPaintListener in " + event.target.document.location.href);
        if (event.target.document != currentDoc) {
            
            
            return;
        }

        SendUpdateCanvasForEvent(event, contentRootElement);
        
        
        
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
                var elements = getNoPaintElements(contentRootElement);
                for (var i = 0; i < elements.length; ++i) {
                  windowUtils().checkAndClearPaintedState(elements[i]);
                }
                var notification = content.document.createEvent("Events");
                notification.initEvent("MozReftestInvalidate", true, false);
                contentRootElement.dispatchEvent(notification);
            }

            if (!inPrintMode && doPrintMode(contentRootElement)) {
                LogInfo("MakeProgress: setting up print mode");
                setupPrintMode();
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

            
            state = STATE_WAITING_FOR_SPELL_CHECKS;
            MakeProgress();
            return;

        case STATE_WAITING_FOR_SPELL_CHECKS:
            LogInfo("MakeProgress: STATE_WAITING_FOR_SPELL_CHECKS");
            if (numPendingSpellChecks) {
                gFailureReason = "timed out waiting for spell checks to end";
                LogInfo("MakeProgress: waiting for spell checks to end");
                return;
            }

            state = STATE_WAITING_TO_FINISH;
            
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
            if (contentRootElement) {
              var elements = getNoPaintElements(contentRootElement);
              for (var i = 0; i < elements.length; ++i) {
                  if (windowUtils().checkAndClearPaintedState(elements[i])) {
                      SendFailedNoPaint();
                  }
              }
              CheckLayerAssertions(contentRootElement);
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

    
    var numPendingSpellChecks = spellCheckedElements.length;
    function decNumPendingSpellChecks() {
        --numPendingSpellChecks;
        MakeProgress();
    }
    for (let editable of spellCheckedElements) {
        try {
            onSpellCheck(editable, decNumPendingSpellChecks);
        } catch (err) {
            
            setTimeout(decNumPendingSpellChecks, 0);
        }
    }

    
    
    
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

    
    
    
    
    
    
    var querySelector =
        '*[class~="spell-checked"],' +
        'textarea:not([spellcheck="false"]),' +
        'input[spellcheck]:-moz-any([spellcheck=""],[spellcheck="true"]),' +
        '*[contenteditable]:-moz-any([contenteditable=""],[contenteditable="true"])';
    var spellCheckedElements = currentDoc.querySelectorAll(querySelector);

    var contentRootElement = currentDoc ? currentDoc.documentElement : null;
    currentDoc = null;
    setupZoom(contentRootElement);
    setupViewport(contentRootElement);
    setupDisplayport(contentRootElement);
    var inPrintMode = false;

    function AfterOnLoadScripts() {
        
        var contentRootElement =
          content.document ? content.document.documentElement : null;

        
        FlushRendering();

        
        
        
        
        var painted = SendInitCanvasWithSnapshot();

        if (shouldWaitForExplicitPaintWaiters() ||
            (!inPrintMode && doPrintMode(contentRootElement)) ||
            
            
            
            !painted) {
            LogInfo("AfterOnLoadScripts belatedly entering WaitForTestEnd");
            
            WaitForTestEnd(contentRootElement, inPrintMode, []);
        } else {
            CheckLayerAssertions(contentRootElement);
            CheckForProcessCrashExpectation(contentRootElement);
            RecordResult();
        }
    }

    if (shouldWaitForReftestWaitRemoval(contentRootElement) ||
        shouldWaitForExplicitPaintWaiters() ||
        spellCheckedElements.length) {
        
        
        gFailureReason = "timed out waiting for test to complete (trying to get into WaitForTestEnd)";
        LogInfo("OnDocumentLoad triggering WaitForTestEnd");
        setTimeout(function () { WaitForTestEnd(contentRootElement, inPrintMode, spellCheckedElements); }, 0);
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

function CheckLayerAssertions(contentRootElement)
{
    if (!contentRootElement) {
        return;
    }

    var opaqueLayerElements = getOpaqueLayerElements(contentRootElement);
    for (var i = 0; i < opaqueLayerElements.length; ++i) {
        var elem = opaqueLayerElements[i];
        try {
            if (!windowUtils().isPartOfOpaqueLayer(elem)) {
                SendFailedOpaqueLayer(elementDescription(elem) + ' is not part of an opaque layer');
            }
        } catch (e) {
            SendFailedOpaqueLayer('got an exception while checking whether ' + elementDescription(elem) + ' is part of an opaque layer');
        }
    }
    var layerNameToElementsMap = getAssignedLayerMap(contentRootElement);
    var oneOfEach = [];
    
    for (var layerName in layerNameToElementsMap) {
        try {
            var elements = layerNameToElementsMap[layerName];
            oneOfEach.push(elements[0]);
            var numberOfLayers = windowUtils().numberOfAssignedPaintedLayers(elements, elements.length);
            if (numberOfLayers !== 1) {
                SendFailedAssignedLayer('these elements are assigned to ' + numberOfLayers +
                                        ' different layers, instead of sharing just one layer: ' +
                                        elements.map(elementDescription).join(', '));
            }
        } catch (e) {
            SendFailedAssignedLayer('got an exception while checking whether these elements share a layer: ' +
                                    elements.map(elementDescription).join(', '));
        }
    }
    
    if (oneOfEach.length > 0) {
        try {
            var numberOfLayers = windowUtils().numberOfAssignedPaintedLayers(oneOfEach, oneOfEach.length);
            if (numberOfLayers !== oneOfEach.length) {
                SendFailedAssignedLayer('these elements are assigned to ' + numberOfLayers +
                                        ' different layers, instead of having none in common (expected ' +
                                        oneOfEach.length + ' different layers): ' +
                                        oneOfEach.map(elementDescription).join(', '));
            }
        } catch (e) {
            SendFailedAssignedLayer('got an exception while checking whether these elements are assigned to different layers: ' +
                                    oneOfEach.map(elementDescription).join(', '));
        }
    }
}

function CheckForProcessCrashExpectation(contentRootElement)
{
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

    
    
    
    var changedAsyncScrollOffsets = setupAsyncScrollOffsets({allowFailure:true}) ;
    if (changedAsyncScrollOffsets && !gBrowserIsRemote) {
        sendAsyncMessage("reftest:UpdateWholeCanvasForInvalidation");
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
    if (gVerbose) {
        sendSyncMessage("reftest:Log", { type: "warning", msg: str });
    } else {
        sendAsyncMessage("reftest:Log", { type: "warning", msg: str });
    }
}

function LogInfo(str)
{
    if (gVerbose) {
        sendSyncMessage("reftest:Log", { type: "info", msg: str });
    } else {
        sendAsyncMessage("reftest:Log", { type: "info", msg: str });
    }
}

const SYNC_DEFAULT = 0x0;
const SYNC_ALLOW_DISABLE = 0x1;
function SynchronizeForSnapshot(flags)
{
    if (gCurrentTestType == TYPE_SCRIPT ||
        gCurrentTestType == TYPE_LOAD) {
        
        return;
    }

    if (flags & SYNC_ALLOW_DISABLE) {
        var docElt = content.document.documentElement;
        if (docElt && docElt.hasAttribute("reftest-no-sync-layers")) {
            LogInfo("Test file chose to skip SynchronizeForSnapshot");
            return;
        }
    }

    windowUtils().updateLayerTree();

    
    
    setupAsyncScrollOffsets({allowFailure:false});
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
    resetDisplayportAndViewport();
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

function SendFailedNoPaint()
{
    sendAsyncMessage("reftest:FailedNoPaint");
}

function SendFailedOpaqueLayer(why)
{
    sendAsyncMessage("reftest:FailedOpaqueLayer", { why: why });
}

function SendFailedAssignedLayer(why)
{
    sendAsyncMessage("reftest:FailedAssignedLayer", { why: why });
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

function elementDescription(element)
{
    return '<' + element.localName +
        [].slice.call(element.attributes).map((attr) =>
            ` ${attr.nodeName}="${attr.value}"`).join('') +
        '>';
}

function SendUpdateCanvasForEvent(event, contentRootElement)
{
    var win = content;
    var scale = markupDocumentViewer().fullZoom;

    var rects = [ ];
    if (shouldSnapshotWholePage(contentRootElement)) {
      
      
      if (!gBrowserIsRemote) {
          sendSyncMessage("reftest:UpdateWholeCanvasForInvalidation");
      } else {
          SynchronizeForSnapshot(SYNC_ALLOW_DISABLE);
          sendAsyncMessage("reftest:UpdateWholeCanvasForInvalidation");
      }
      return;
    }
    
    var rectList = event.clientRects;
    LogInfo("SendUpdateCanvasForEvent with " + rectList.length + " rects");
    for (var i = 0; i < rectList.length; ++i) {
        var r = rectList[i];
        
        var left = Math.floor(roundTo(r.left*scale, 0.001));
        var top = Math.floor(roundTo(r.top*scale, 0.001));
        var right = Math.ceil(roundTo(r.right*scale, 0.001));
        var bottom = Math.ceil(roundTo(r.bottom*scale, 0.001));
        LogInfo("Rect: " + left + " " + top + " " + right + " " + bottom);

        rects.push({ left: left, top: top, right: right, bottom: bottom });
    }

    
    
    if (!gBrowserIsRemote) {
        sendSyncMessage("reftest:UpdateCanvasForInvalidation", { rects: rects });
    } else {
        SynchronizeForSnapshot(SYNC_ALLOW_DISABLE);
        sendAsyncMessage("reftest:UpdateCanvasForInvalidation", { rects: rects });
    }
}
#if REFTEST_B2G
OnInitialLoad();
#else
addEventListener("load", OnInitialLoad, true);
#endif
