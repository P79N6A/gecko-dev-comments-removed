




































const CC = Components.classes;
const CI = Components.interfaces;
const CR = Components.results;

const XHTML_NS = "http://www.w3.org/1999/xhtml";

const NS_LOCAL_FILE_CONTRACTID = "@mozilla.org/file/local;1";
const IO_SERVICE_CONTRACTID = "@mozilla.org/network/io-service;1";
const NS_LOCALFILEINPUTSTREAM_CONTRACTID =
          "@mozilla.org/network/file-input-stream;1";

const LOAD_FAILURE_TIMEOUT = 10000; 

var gBrowser;
var gCanvas;
var gURLs;
var gState;
var gPart1Key;
var gFailureTimeout;

const EXPECTED_PASS = 0;
const EXPECTED_FAIL = 1;
const EXPECTED_RANDOM = 2;

function OnRefTestLoad()
{
    gBrowser = document.getElementById("browser");

    gBrowser.addEventListener("load", OnDocumentLoad, true);

    gCanvas = document.createElementNS(XHTML_NS, "canvas");
    var windowElem = document.documentElement;
    gCanvas.setAttribute("width", windowElem.getAttribute("width"));
    gCanvas.setAttribute("height", windowElem.getAttribute("height"));

    try {
        ReadTopManifest(window.arguments[0]);
        StartCurrentTest();
    } catch (ex) {
        gBrowser.loadURI('data:text/plain,' + ex);
    }
}

function OnRefTestUnload()
{
    gBrowser.removeEventListener("load", OnDocumentLoad, true);
}

function ReadTopManifest(aFileURL)
{
    gURLs = new Array();
    var ios = CC[IO_SERVICE_CONTRACTID].getService(CI.nsIIOService);
    var url = ios.newURI(aFileURL, null, null);
    if (!url || !url.schemeIs("file"))
        throw "Expected a file URL for the manifest.";
    ReadManifest(url);
}

function ReadManifest(aURL)
{
    var ios = CC[IO_SERVICE_CONTRACTID].getService(CI.nsIIOService);
    var listURL = aURL.QueryInterface(CI.nsIFileURL);

    var fis = CC[NS_LOCALFILEINPUTSTREAM_CONTRACTID].
                  createInstance(CI.nsIFileInputStream);
    fis.init(listURL.file, -1, -1, false);
    var lis = fis.QueryInterface(CI.nsILineInputStream);

    var sandbox = new Components.utils.Sandbox(aURL.spec);
    for (var prop in gAutoconfVars)
        sandbox[prop] = gAutoconfVars[prop];

    var line = {value:null};
    var lineNo = 0;
    do {
        var more = lis.readLine(line);
        ++lineNo;
        var str = line.value;
        str = /^[^#]*/.exec(str)[0]; 
        if (!str)
            continue; 
        
        str = str.replace(/^\s*/, '').replace(/\s*$/, '');
        if (!str || str == "")
            continue;
        var items = str.split(/\s+/); 

        var expected_status = EXPECTED_PASS;
        while (items[0].match(/^(fails|random)/)) {
            var item = items.shift();
            var stat;
            var cond;
            var m = item.match(/^(fails|random)-if(\(.*\))$/);
            if (m) {
                stat = m[1];
                
                cond = Components.utils.evalInSandbox(m[2], sandbox);
            } else if (item.match(/^(fails|random)$/)) {
                stat = item;
                cond = true;
            } else {
                throw "Error in manifest file " + aURL.spec + " line " + lineNo;
            }

            if (cond) {
                if (stat == "fails") {
                    expected_status = EXPECTED_FAIL;
                } else if (stat == "random") {
                    expected_status = EXPECTED_RANDOM;
                }
            }
        }

        if (items[0] == "include") {
            if (items.length != 2)
                throw "Error in manifest file " + aURL.spec + " line " + lineNo;
            ReadManifest(ios.newURI(items[1], null, listURL));
        } else if (items[0] == "==" || items[0] == "!=") {
            if (items.length != 3)
                throw "Error in manifest file " + aURL.spec + " line " + lineNo;
            gURLs.push( { equal: (items[0] == "=="),
                          expected: expected_status,
                          url1: ios.newURI(items[1], null, listURL),
                          url2: ios.newURI(items[2], null, listURL)} );
        } else {
            throw "Error in manifest file " + aURL.spec + " line " + lineNo;
        }
    } while (more);
}

function StartCurrentTest()
{
    if (gURLs.length == 0)
        DoneTests();
    else
        StartCurrentURI(1);
}

function StartCurrentURI(aState)
{
    gFailureTimeout = setTimeout(LoadFailed, LOAD_FAILURE_TIMEOUT);

    gState = aState;
    gBrowser.loadURI(gURLs[0]["url" + aState].spec);
}

function DoneTests()
{
    goQuitApplication();
}

function IFrameToKey()
{
    var ctx = gCanvas.getContext("2d");
    


    ctx.drawWindow(gBrowser.contentWindow, 0, 0,
                   gCanvas.width, gCanvas.height, "rgb(255,255,255)");
    return gCanvas.toDataURL();
}

function OnDocumentLoad(event)
{
    if (event.target != gBrowser.contentDocument)
        
        return;

    var contentRootElement = gBrowser.contentDocument.documentElement;

    function shouldWait() {
        
        return contentRootElement.hasAttribute('class') &&
               contentRootElement.getAttribute('class').split(/\s+/)
                                 .indexOf("reftest-wait") != -1;
    }

    function doPrintMode() {
        
        return contentRootElement.hasAttribute('class') &&
               contentRootElement.getAttribute('class').split(/\s+/)
                                 .indexOf("reftest-print") != -1;
    }

    if (shouldWait()) {
        
        
        
        contentRootElement.addEventListener(
            "DOMAttrModified",
            function(event) {
                if (!shouldWait()) {
                    contentRootElement.removeEventListener(
                        "DOMAttrModified",
                        arguments.callee,
                        false);
                    setTimeout(DocumentLoaded, 0);
                }
            }, false);
    } else {
        if (doPrintMode()) {
            var PSSVC = Components.classes["@mozilla.org/gfx/printsettings-service;1"]
                    .getService(Components.interfaces.nsIPrintSettingsService);
            var ps = PSSVC.newPrintSettings;
            ps.paperWidth = 5;
            ps.paperHeight = 3;
            ps.headerStrLeft = "";
            ps.headerStrCenter = "";
            ps.headerStrRight = "";
            ps.footerStrLeft = "";
            ps.footerStrCenter = "";
            ps.footerStrRight = "";
            gBrowser.docShell.contentViewer.setPageMode(true, ps);
        }

        
        
        
        
        setTimeout(setTimeout, 0, DocumentLoaded, 0);
    }
}

function DocumentLoaded()
{
    clearTimeout(gFailureTimeout);
    var key = IFrameToKey();
    switch (gState) {
        case 1:
            
            
            gPart1Key = key;

            StartCurrentURI(2);
            break;
        case 2:
            
            
            
            
            
            var equal = (key == gPart1Key);
            
            var test_passed = (equal == gURLs[0].equal);
            
            var expected = gURLs[0].expected;
            
            var outputs = {};
            const randomMsg = " (RESULT EXPECTED TO BE RANDOM)";
            outputs[EXPECTED_PASS] = {true: "PASS",
                                      false: "UNEXPECTED FAIL"};
            outputs[EXPECTED_FAIL] = {true: "UNEXPECTED PASS",
                                      false: "KNOWN FAIL"};
            outputs[EXPECTED_RANDOM] = {true: "PASS" + randomMsg,
                                        false: "KNOWN FAIL" + randomMsg};
            
            var result = "REFTEST " + outputs[expected][test_passed] + ": ";
            if (!gURLs[0].equal) {
                result += "(!=) ";
            }
            result += gURLs[0].url1.spec; 
            dump(result + "\n");
            if (!test_passed && expected == EXPECTED_PASS) {
                dump("REFTEST   IMAGE 1 (TEST): " + gPart1Key + "\n");
                dump("REFTEST   IMAGE 2 (REFERENCE): " + key + "\n");
            }

            gPart1Key = undefined;
            gURLs.shift();
            StartCurrentTest();
            break;
        default:
            throw "Unexpected state."
    }
}

function LoadFailed()
{
    dump("REFTEST UNEXPECTED FAIL (LOADING): " +
         gURLs[0]["url" + gState].spec + "\n");
    gURLs.shift();
    StartCurrentTest();
}
