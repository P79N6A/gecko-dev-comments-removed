





const CC = Components.classes;
const CI = Components.interfaces;

const NS_GFXINFO_CONTRACTID = "@mozilla.org/gfx/info;1";

var gContainingWindow = null;

var gBrowser;

function OnDocumentLoad(evt) {
dump(evt.target.location + "\n");
    if (evt.target.location == "about:blank")
        return;
    gContainingWindow.close();
}

this.OnRecordingLoad = function OnRecordingLoad(win) {
    if (win === undefined || win == null) {
        win = window;
    }
    if (gContainingWindow == null && win != null) {
        gContainingWindow = win;
    }

    gBrowser = gContainingWindow.document.getElementById("browser");

    var gfxInfo = (NS_GFXINFO_CONTRACTID in CC) && CC[NS_GFXINFO_CONTRACTID].getService(CI.nsIGfxInfo);
    var info = gfxInfo.getInfo();
    dump(info.AzureContentBackend + "\n");
    if (info.AzureContentBackend == "none") {
        alert("Page recordings may only be made with Azure content enabled.");
        gContainingWindow.close();
        return;
    }

    gContainingWindow.document.addEventListener("load", OnDocumentLoad, true);

    var args = window.arguments[0].wrappedJSObject;

    gBrowser.loadURI(args.uri);
};
