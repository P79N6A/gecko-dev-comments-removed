


function test() {
  waitForExplicitFinish();
  
  SpecialPowers.pushPrefEnv({"set": [["network.cookie.cookieBehavior", 0]]}, initTest);
}

function initTest() {
    const searchTerm = "example";
    const dummyTerm = "elpmaxe";

    var cm =  Components.classes["@mozilla.org/cookiemanager;1"]
                        .getService(Components.interfaces.nsICookieManager);

    
    cm.removeAll();

    
    var vals = [[searchTerm+".com", dummyTerm, dummyTerm],          
                [searchTerm+".org", dummyTerm, dummyTerm],          
                [dummyTerm+".com", searchTerm, dummyTerm],          
                [dummyTerm+".edu", searchTerm+dummyTerm, dummyTerm],
                [dummyTerm+".net", dummyTerm, searchTerm],          
                [dummyTerm+".org", dummyTerm, searchTerm+dummyTerm],
                [dummyTerm+".int", dummyTerm, dummyTerm]];          

    
    const matches = 6;

    var ios = Components.classes["@mozilla.org/network/io-service;1"]
                        .getService(Components.interfaces.nsIIOService);
    var cookieSvc = Components.classes["@mozilla.org/cookieService;1"]
                              .getService(Components.interfaces.nsICookieService);
    var v;
    
    for (v in vals) {
        let [host, name, value] = vals[v];    
        var cookieUri = ios.newURI("http://"+host, null, null);
        cookieSvc.setCookieString(cookieUri, null, name+"="+value+";", null);
    }

    
    var cmd = window.openDialog("chrome://browser/content/preferences/cookies.xul",
                                "Browser:Cookies", "", {});
    
    
    cmd.addEventListener("load", function() {executeSoon(function() {runTest(cmd, searchTerm, vals.length, matches);});}, false);
}

function isDisabled(win, expectation) {
    var disabled = win.document.getElementById("removeAllCookies").disabled;
    is(disabled, expectation, "Remove all cookies button has correct state: "+(expectation?"disabled":"enabled"));
}

function runTest(win, searchTerm, cookies, matches) {
    var cm =  Components.classes["@mozilla.org/cookiemanager;1"]
                        .getService(Components.interfaces.nsICookieManager);


    
    var cnt = 0,
        enumerator = cm.enumerator;
    while (enumerator.hasMoreElements()) {
        cnt++;
        enumerator.getNext();
    }
    is(cnt, cookies, "Number of cookies match injected cookies");

    
    isDisabled(win, false);

    
    win.gCookiesWindow.setFilter(searchTerm);
    is(win.gCookiesWindow._view.rowCount, matches, "Correct number of cookies shown after filter is applied");

    
    isDisabled(win, false);


    
    var tree = win.document.getElementById("cookiesList");
    var deleteButton = win.document.getElementById("removeCookie");
    var x = {}, y = {}, width = {}, height = {};
    tree.treeBoxObject.getCoordsForCellItem(0, tree.columns[0], "cell", x, y, width, height);
    EventUtils.synthesizeMouse(tree.body, x.value + width.value / 2, y.value + height.value / 2, {}, win);
    EventUtils.synthesizeMouseAtCenter(deleteButton, {}, win);

    
    is(win.gCookiesWindow._view.rowCount, matches-1, "Deleted selected cookie");

    
    EventUtils.synthesizeMouse(tree.body, x.value + width.value / 2, y.value + height.value / 2, {}, win);
    deleteButton = win.document.getElementById("removeCookies"); 
    var eventObj = {};
    if (navigator.platform.indexOf("Mac") >= 0)
        eventObj.metaKey = true;
    else
        eventObj.ctrlKey = true;
    tree.treeBoxObject.getCoordsForCellItem(1, tree.columns[0], "cell", x, y, width, height);
    EventUtils.synthesizeMouse(tree.body, x.value + width.value / 2, y.value + height.value / 2, eventObj, win);
    EventUtils.synthesizeMouseAtCenter(deleteButton, {}, win);

    
    is(win.gCookiesWindow._view.rowCount, matches-3, "Deleted selected two adjacent cookies");

    
    isDisabled(win, false);

    
    var deleteAllButton = win.document.getElementById("removeAllCookies");
    EventUtils.synthesizeMouseAtCenter(deleteAllButton, {}, win);
    is(win.gCookiesWindow._view.rowCount, 0, "Deleted all matching cookies");

    
    isDisabled(win, true);

    
    win.gCookiesWindow.setFilter("");
    is(win.gCookiesWindow._view.rowCount, cookies-matches, "Unmatched cookies remain");

    
    isDisabled(win, false);

    
    EventUtils.synthesizeMouseAtCenter(deleteAllButton, {}, win);
    is(win.gCookiesWindow._view.rowCount, 0, "Deleted all cookies");

    
    var cnt = 0,
        enumerator = cm.enumerator;
    while (enumerator.hasMoreElements()) {
        cnt++;
        enumerator.getNext();
    }
    is(cnt, 0, "Zero cookies remain");

    
    isDisabled(win, true);

    
    win.close();
    finish();
}
