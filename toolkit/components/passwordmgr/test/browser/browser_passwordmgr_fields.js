



function test() {
  waitForExplicitFinish();

  let pwmgr = Cc["@mozilla.org/login-manager;1"].
                getService(Ci.nsILoginManager);
  pwmgr.removeAllLogins();

  
  let nsLoginInfo = new Components.Constructor("@mozilla.org/login-manager/loginInfo;1",
                                                 Ci.nsILoginInfo, "init");
  let login = new nsLoginInfo("http://example.com/", "http://example.com/", null,
                              "user", "password", "u1", "p1");
  pwmgr.addLogin(login);

  
  const PWMGR_DLG = "chrome://passwordmgr/content/passwordManager.xul";
  let pwmgrdlg = window.openDialog(PWMGR_DLG, "Toolkit:PasswordManager", "");
  SimpleTest.waitForFocus(doTest, pwmgrdlg);

  function doTest() {
    let doc = pwmgrdlg.document;

    let signonsTree = doc.querySelector("#signonsTree");
    is(signonsTree.view.rowCount, 1, "One entry in the passwords list");

    is(signonsTree.view.getCellText(0, signonsTree.columns.getNamedColumn("siteCol")),
       "http://example.com/",
       "Correct website saved");

    is(signonsTree.view.getCellText(0, signonsTree.columns.getNamedColumn("userCol")),
       "user",
       "Correct user saved");

    let timeCreatedCol = doc.getElementById("timeCreatedCol");
    is(timeCreatedCol.getAttribute("hidden"), "true",
       "Time created column is not displayed");


    let timeLastUsedCol = doc.getElementById("timeLastUsedCol");
    is(timeLastUsedCol.getAttribute("hidden"), "",
       "Last Used column is displayed");

    let timePasswordChangedCol = doc.getElementById("timePasswordChangedCol");
    is(timePasswordChangedCol.getAttribute("hidden"), "",
       "Last Changed column is displayed");

    
    Services.ww.registerNotification(function (aSubject, aTopic, aData) {
      if (aSubject.location == pwmgrdlg.location && aTopic == "domwindowclosed") {
        
        Services.ww.unregisterNotification(arguments.callee);

        pwmgr.removeAllLogins();

        finish();
      }
    });

    pwmgrdlg.close();
  }
}
