


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head_chrome.js";

function getAndroidVersion() {
  return runEmulatorShellCmdSafe(["getprop", "ro.build.version.sdk"])
    .then(aResults => aResults[0]);
}


startTestBase(function() {
  return getAndroidVersion().
    then((aVersion) => {
      if (aVersion < "19") {
        
        
        log("Skip test: AndroidVersion: " + aVersion);
        return;
      }

      return getCellInfoList()
        .then((aResults) => {
          
          is(aResults.length, 1, "Check number of cell Info");

          let cell = aResults[0];
          is(cell.type, Ci.nsICellInfo.CELL_INFO_TYPE_GSM, "Check cell.type");
          is(cell.registered, true, "Check cell.registered");

          ok(cell instanceof Ci.nsIGsmCellInfo,
             "cell.constructor is " + cell.constructor);

          
          
          
        });
    });
});
