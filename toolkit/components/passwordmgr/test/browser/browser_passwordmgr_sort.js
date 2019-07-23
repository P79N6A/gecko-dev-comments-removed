





































const Cc = Components.classes;
const Ci = Components.interfaces;

function test() {
    waitForExplicitFinish();

    let pwmgr = Cc["@mozilla.org/login-manager;1"].
                getService(Ci.nsILoginManager);
    pwmgr.removeAllLogins();

    
    let urls = [
        "http://example.com/",
        "http://example.org/",
        "http://mozilla.com/",
        "http://mozilla.org/",
        "http://spreadfirefox.com/",
        "http://planet.mozilla.org/",
        "https://developer.mozilla.org/",
        "http://hg.mozilla.org/",
        "http://mxr.mozilla.org/",
        "http://feeds.mozilla.org/",
    ];
    let users = [
        "user",
        "username",
        "ehsan",
        "ehsan",
        "john",
        "what?",
        "really?",
        "you sure?",
        "my user name",
        "my username",
    ];
    let pwds = [
        "password",
        "password",
        "mypass",
        "mypass",
        "smith",
        "very secret",
        "super secret",
        "absolutely",
        "mozilla",
        "mozilla.com",
    ];
    let nsLoginInfo = new Components.Constructor("@mozilla.org/login-manager/loginInfo;1",
                                                 Ci.nsILoginInfo, "init");
    for (let i = 0; i < 10; i++)
        pwmgr.addLogin(new nsLoginInfo(urls[i], urls[i], null, users[i], pwds[i],
                                       "u"+(i+1), "p"+(i+1)));

    
    let ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
             getService(Ci.nsIWindowWatcher);
    let obs = {
        observe: function(aSubject, aTopic, aData) {
            
            ww.unregisterNotification(this);

            let win = aSubject.QueryInterface(Ci.nsIDOMEventTarget);
            win.addEventListener("load", function() {
                win.removeEventListener("load", arguments.callee, true);
                setTimeout(doTest, 0);
            }, true);
        }
    };
    ww.registerNotification(obs);

    
    const PWMGR_DLG = "chrome://passwordmgr/content/passwordManager.xul";
    let pwmgrdlg = window.openDialog(PWMGR_DLG, "Toolkit:PasswordManager", "");

    
    function doTest() {
        let doc = pwmgrdlg.document;
        let win = doc.defaultView;
        let sTree = doc.getElementById("signonsTree");
        let filter = doc.getElementById("filter");
        let siteCol = doc.getElementById("siteCol");
        let userCol = doc.getElementById("userCol");
        let passwordCol = doc.getElementById("passwordCol");

        let toggleCalls = 0;
        function toggleShowPasswords(func) {
            let toggleButton = doc.getElementById("togglePasswords");
            let showMode = (toggleCalls++ % 2) == 0;

            
            if (showMode) {
                let obs = {
                    observe: function(aSubject, aTopic, aData) {
                        if (aTopic == "domwindowclosed")
                            ww.unregisterNotification(this);
                        else if (aTopic == "domwindowopened") {
                            let win = aSubject.QueryInterface(Ci.nsIDOMEventTarget);
                            win.addEventListener("load", function() {
                                win.removeEventListener("load", arguments.callee, true);
                                setTimeout(function() {
                                    EventUtils.synthesizeKey("VK_RETURN", {}, win)
                                }, 0);
                            }, true);
                        }
                    }
                };
                ww.registerNotification(obs);
            }

            let obsSvc = Cc["@mozilla.org/observer-service;1"].
                         getService(Ci.nsIObserverService);
            obsSvc.addObserver({
                observe: function(aSubject, aTopic, aData) {
                    if (aTopic == "passwordmgr-password-toggle-complete") {
                        obsSvc.removeObserver(this, "passwordmgr-password-toggle-complete", false);
                        func();
                    }
                }
            }, "passwordmgr-password-toggle-complete", false);

            EventUtils.synthesizeMouse(toggleButton, 1, 1, {}, win);
        }

        function clickCol(col) {
            EventUtils.synthesizeMouse(col, 20, 1, {}, win);
            setTimeout(runNextTest, 0);
        }

        function setFilter(string) {
            filter.value = string;
            filter.doCommand();
            setTimeout(runNextTest, 0);
        }

        function checkSortMarkers(activeCol) {
            let isOk = true;
            let col = null;
            let hasAttr = false;
            let treecols = activeCol.parentNode;
            for (let i = 0; i < treecols.childNodes.length; i++) {
                col = treecols.childNodes[i];
                if (col.nodeName != "treecol")
                    continue;
                hasAttr = col.hasAttribute("sortDirection");
                isOk &= col == activeCol ? hasAttr : !hasAttr;
            }
            ok(isOk, "Only " + activeCol.id + " has a sort marker");
        }

        function checkSortDirection(col, ascending) {
            checkSortMarkers(col);
            let direction = ascending ? "ascending" : "descending";
            is(col.getAttribute("sortDirection"), direction,
               col.id + ": sort direction is " + direction);
        }

        function checkColumnEntries(aCol, expectedValues) {
            let actualValues = getColumnEntries(aCol);
            is(actualValues.length, expectedValues.length, "Checking length of expected column");
            for (let i = 0; i < expectedValues.length; i++)
                is(actualValues[i], expectedValues[i], "Checking column entry #"+i);
        }

        function getColumnEntries(aCol) {
            let entries = [];
            let column = sTree.columns[aCol];
            let numRows = sTree.view.rowCount;
            for (let i = 0; i < numRows; i++)
                entries.push(sTree.view.getCellText(i, column));
            return entries;
        }

        let testCounter = 0;
        let expectedValues;
        function runNextTest() {
            switch (testCounter++) {
            case 0:
                expectedValues = urls.slice().sort();
                checkColumnEntries(0, expectedValues);
                checkSortDirection(siteCol, true);
                
                clickCol(siteCol);
                break;
            case 1:
                expectedValues.reverse();
                checkColumnEntries(0, expectedValues);
                checkSortDirection(siteCol, false);
                
                clickCol(userCol);
                break;
            case 2:
                expectedValues = users.slice().sort();
                checkColumnEntries(1, expectedValues);
                checkSortDirection(userCol, true);
                
                clickCol(passwordCol);
                break;
            case 3:
                expectedValues = pwds.slice().sort();
                checkColumnEntries(2, expectedValues);
                checkSortDirection(passwordCol, true);
                
                setFilter("moz");
                break;
            case 4:
                expectedValues = [ "absolutely", "mozilla", "mozilla.com",
                                   "mypass", "mypass", "super secret",
                                   "very secret" ];
                checkColumnEntries(2, expectedValues);
                checkSortDirection(passwordCol, true);
                
                setFilter("");
                break;
            case 5:
                expectedValues = pwds.slice().sort();
                checkColumnEntries(2, expectedValues);
                checkSortDirection(passwordCol, true);
                
                pwmgrdlg.close();
                pwmgr.removeAllLogins();
                finish();
            }
        }

        
        toggleShowPasswords(runNextTest);
    }
}
