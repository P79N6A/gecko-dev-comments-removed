


MARIONETTE_CONTEXT = "chrome";

let XPCOMUtils = Cu.import("resource://gre/modules/XPCOMUtils.jsm").XPCOMUtils;
let Promise = Cu.import("resource://gre/modules/Promise.jsm").Promise;

let mobileConnectionService =
  Cc["@mozilla.org/mobileconnection/gonkmobileconnectionservice;1"]
  .getService(Ci.nsIMobileConnectionService);
ok(mobileConnectionService,
   "mobileConnectionService.constructor is " + mobileConnectionService.constructor);

let _pendingEmulatorShellCmdCount = 0;
















function runEmulatorShellCmdSafe(aCommands) {
  return new Promise(function(aResolve, aReject) {
    ++_pendingEmulatorShellCmdCount;
    runEmulatorShell(aCommands, function(aResult) {
      --_pendingEmulatorShellCmdCount;

      ok(true, "Emulator shell response: " + JSON.stringify(aResult));
      aResolve(aResult);
    });
  });
}









function getMobileConnection(aClientId = 0) {
  let mobileConnection = mobileConnectionService.getItemByServiceId(0);
  ok(mobileConnection,
     "mobileConnection.constructor is " + mobileConnection.constructor);
  return mobileConnection;
}














function getNeighboringCellIds(aClientId = 0) {
  let mobileConnection = getMobileConnection(aClientId);
  return new Promise(function(aResolve, aReject) {
    ok(true, "getNeighboringCellIds");
    mobileConnection.getNeighboringCellIds({
      QueryInterface: XPCOMUtils.generateQI([Ci.nsINeighboringCellIdsCallback]),
      notifyGetNeighboringCellIds: function(aCount, aResults) {
        aResolve(aResults);
      },
      notifyGetNeighboringCellIdsFailed: function(aErrorMsg) {
        aReject(aErrorMsg);
      },
    });
  });
}














function getCellInfoList(aClientId = 0) {
  let mobileConnection = getMobileConnection(aClientId);
  return new Promise(function(aResolve, aReject) {
    ok(true, "getCellInfoList");
    mobileConnection.getCellInfoList({
      QueryInterface: XPCOMUtils.generateQI([Ci.nsICellInfoListCallback]),
      notifyGetCellInfoList: function(aCount, aResults) {
        aResolve(aResults);
      },
      notifyGetCellInfoListFailed: function(aErrorMsg) {
        aReject(aErrorMsg);
      },
    });
  });
}




function cleanUp() {
  
  ok(true, ":: CLEANING UP ::");

  waitFor(finish, function() {
    return _pendingEmulatorShellCmdCount === 0;
  });
}









function startTestBase(aTestCaseMain) {
  return Promise.resolve()
    .then(aTestCaseMain)
    .catch((aException) => {
      ok(false, "promise rejects during test: " + aException);
    })
    .then(cleanUp);
}
