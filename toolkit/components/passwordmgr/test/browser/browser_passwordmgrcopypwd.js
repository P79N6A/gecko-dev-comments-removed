




































const Cc = Components.classes;
const Ci = Components.interfaces;

function test() {
    waitForExplicitFinish();

    let pwmgr = Cc["@mozilla.org/login-manager;1"].
                getService(Ci.nsILoginManager);
    pwmgr.removeAllLogins();

    
    let urls = [
        "http://example.com/",
        "http://mozilla.org/",
        "http://spreadfirefox.com/",
        "https://developer.mozilla.org/",
        "http://hg.mozilla.org/"
    ];
    let nsLoginInfo = new Components.Constructor("@mozilla.org/login-manager/loginInfo;1",
                                                 Ci.nsILoginInfo, "init");
    let logins = [
        new nsLoginInfo(urls[0], urls[0], null, "o", "hai", "u1", "p1"),
        new nsLoginInfo(urls[1], urls[1], null, "ehsan", "coded", "u2", "p2"),
        new nsLoginInfo(urls[2], urls[2], null, "this", "awesome", "u3", "p3"),
        new nsLoginInfo(urls[3], urls[3], null, "array of", "logins", "u4", "p4"),
        new nsLoginInfo(urls[4], urls[4], null, "then", "i wrote the test", "u5", "p5")
    ];
    logins.forEach(function (login) pwmgr.addLogin(login));

    
    const PWMGR_DLG = "chrome://passwordmgr/content/passwordManager.xul";
    let pwmgrdlg = window.openDialog(PWMGR_DLG, "Toolkit:PasswordManager", "");
    SimpleTest.waitForFocus(doTest, pwmgrdlg);

    
    function doTest() {
        let clip = Cc["@mozilla.org/widget/clipboard;1"].getService(Ci.nsIClipboard);
        let data = "";
        let polls = 0;

        function step1() {
            Cc["@mozilla.org/widget/clipboardhelper;1"].
            getService(Ci.nsIClipboardHelper).copyString("manatee");

            waitForClipboard("manatee", step2);
        }

        function step2() {
            let doc = pwmgrdlg.document;
            doc.getElementById("signonsTree").currentIndex = 2;
            doc.getElementById("context-copypassword").doCommand();

            polls = 0;
            waitForClipboard("coded", cleanUp);
        }

        function waitForClipboard(expectedValue, callback) {
            if (++polls > 50) {
                ok(false, "Timed out while polling clipboard");
                cleanUp();
                return;
            }

            let data = null;
            let trans = Cc["@mozilla.org/widget/transferable;1"].
                        createInstance(Ci.nsITransferable);
            trans.addDataFlavor("text/unicode");
            clip.getData(trans, clip.kGlobalClipboard);

            try {
                let str = {};
                trans.getTransferData("text/unicode", str, {});
                data = str.value.QueryInterface(Ci.nsISupportsString).data;
            } catch (e) {}

            if (data == expectedValue) {
                is(data, expectedValue, "Clipboard should match expected value");
                callback();
            }
            else
                setTimeout(waitForClipboard, 100, expectedValue, callback);
        }

        function cleanUp() {
            Services.ww.registerNotification(function (aSubject, aTopic, aData) {
                Services.ww.unregisterNotification(arguments.callee);
                pwmgr.removeAllLogins();
                finish();
            });
            pwmgrdlg.close();
        }

        step1();
    }
}
