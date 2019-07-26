





const CC = Components.classes;
const CI = Components.interfaces;
const CR = Components.results;

const XHTML_NS = "http://www.w3.org/1999/xhtml";
const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
const NS_GFXINFO_CONTRACTID = "@mozilla.org/gfx/info;1";

var gContainingWindow = null;

var gBrowser;

function OnDocumentLoad() {
    gContainingWindow.close();
}

this.OnRecordingLoad = function OnRecordingLoad(win) {
    var prefs = Components.classes["@mozilla.org/preferences-service;1"].
                getService(Components.interfaces.nsIPrefBranch);

    if (win === undefined || win == null) {
        win = window;
    }
    if (gContainingWindow == null && win != null) {
        gContainingWindow = win;
    }

    gBrowser = gContainingWindow.document.getElementById("browser");

    var gfxInfo = (NS_GFXINFO_CONTRACTID in CC) && CC[NS_GFXINFO_CONTRACTID].getService(CI.nsIGfxInfo);
    var info = gfxInfo.getInfo();
    dump(info.AzureContentBackend);
    if (info.AzureContentBackend == "none") {
        alert("Page recordings may only be made with Azure content enabled.");
        gContainingWindow.close();
        return;
    }

    gContainingWindow.document.addEventListener("load", OnDocumentLoad, true);

    var args = window.arguments[0].wrappedJSObject;

    gBrowser.loadURI(args.uri);
}

function OnRecordingUnload() {
}
