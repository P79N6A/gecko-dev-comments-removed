


const {Cc: Cc, Ci: Ci, Cr: Cr, Cu: Cu} = SpecialPowers;

const PREF_KEY_RIL_DEBUGGING_ENABLED = "ril.debugging.enabled";


const DEFAULT_PIN = "0000";

const DEFAULT_PUK = "12345678";

const WHT = 0xFFFFFFFF;
const BLK = 0x000000FF;
const RED = 0xFF0000FF;
const GRN = 0x00FF00FF;
const BLU = 0x0000FFFF;
const TSP = 0;


const BASIC_ICON = {
  width: 8,
  height: 8,
  codingScheme: "basic",
  pixels: [WHT, WHT, WHT, WHT, WHT, WHT, WHT, WHT,
           BLK, BLK, BLK, BLK, BLK, BLK, WHT, WHT,
           WHT, BLK, WHT, BLK, BLK, WHT, BLK, WHT,
           WHT, BLK, BLK, WHT, WHT, BLK, BLK, WHT,
           WHT, BLK, BLK, WHT, WHT, BLK, BLK, WHT,
           WHT, BLK, WHT, BLK, BLK, WHT, BLK, WHT,
           WHT, WHT, BLK, BLK, BLK, BLK, WHT, WHT,
           WHT, WHT, WHT, WHT, WHT, WHT, WHT, WHT]
};

const COLOR_ICON = {
  width: 8,
  height: 8,
  codingScheme: "color",
  pixels: [BLU, BLU, BLU, BLU, BLU, BLU, BLU, BLU,
           BLU, RED, RED, RED, RED, RED, RED, BLU,
           BLU, RED, GRN, GRN, GRN, RED, RED, BLU,
           BLU, RED, RED, GRN, GRN, RED, RED, BLU,
           BLU, RED, RED, GRN, GRN, RED, RED, BLU,
           BLU, RED, RED, GRN, GRN, GRN, RED, BLU,
           BLU, RED, RED, RED, RED, RED, RED, BLU,
           BLU, BLU, BLU, BLU, BLU, BLU, BLU, BLU]
};

const COLOR_TRANSPARENCY_ICON = {
  width: 8,
  height: 8,
  codingScheme: "color-transparency",
  pixels: [TSP, TSP, TSP, TSP, TSP, TSP, TSP, TSP,
           TSP, RED, RED, RED, RED, RED, RED, TSP,
           TSP, RED, GRN, GRN, GRN, RED, RED, TSP,
           TSP, RED, RED, GRN, GRN, RED, RED, TSP,
           TSP, RED, RED, GRN, GRN, RED, RED, TSP,
           TSP, RED, RED, GRN, GRN, GRN, RED, TSP,
           TSP, RED, RED, RED, RED, RED, RED, TSP,
           TSP, TSP, TSP, TSP, TSP, TSP, TSP, TSP]
};




function isIcons(aIcons, aExpectedIcons) {
  is(aIcons.length, aExpectedIcons.length, "icons.length");
  for (let i = 0; i < aIcons.length; i++) {
    let icon = aIcons[i];
    let expectedIcon = aExpectedIcons[i];

    is(icon.width, expectedIcon.width, "icon.width");
    is(icon.height, expectedIcon.height, "icon.height");
    is(icon.codingScheme, expectedIcon.codingScheme, "icon.codingScheme");

    is(icon.pixels.length, expectedIcon.pixels.length);
    for (let j = 0; j < icon.pixels.length; j++) {
      is(icon.pixels[j], expectedIcon.pixels[j], "icon.pixels[" + j + "]");
    }
  }
}




function isStkText(aStkText, aExpectedStkText) {
  is(aStkText.text, aExpectedStkText.text, "stkText.text");
  if (aExpectedStkText.icons) {
    is(aStkText.iconSelfExplanatory, aExpectedStkText.iconSelfExplanatory,
       "stkText.iconSelfExplanatory");
    isIcons(aStkText.icons, aExpectedStkText.icons);
  }
}

let _pendingEmulatorCmdCount = 0;


















function runEmulatorCmdSafe(aCommand) {
  return new Promise(function(aResolve, aReject) {
    ++_pendingEmulatorCmdCount;
    runEmulatorCmd(aCommand, function(aResult) {
      --_pendingEmulatorCmdCount;

      ok(true, "Emulator response: " + JSON.stringify(aResult));
      if (Array.isArray(aResult) &&
          aResult[aResult.length - 1] === "OK") {
        aResolve(aResult);
      } else {
        aReject(aResult);
      }
    });
  });
}











function sendEmulatorStkPdu(aPdu) {
  let cmd = "stk pdu " + aPdu;
  return runEmulatorCmdSafe(cmd);
}

let workingFrame;
let iccManager;

















function ensureIccManager(aAdditionalPermissions) {
  return new Promise(function(aResolve, aReject) {
    aAdditionalPermissions = aAdditionalPermissions || [];

    if (aAdditionalPermissions.indexOf("mobileconnection") < 0) {
      aAdditionalPermissions.push("mobileconnection");
    }
    let permissions = [];
    for (let perm of aAdditionalPermissions) {
      permissions.push({ "type": perm, "allow": 1, "context": document });
    }

    SpecialPowers.pushPermissions(permissions, function() {
      ok(true, "permissions pushed: " + JSON.stringify(permissions));

      
      
      workingFrame = document.createElement("iframe");
      workingFrame.addEventListener("load", function load() {
        workingFrame.removeEventListener("load", load);

        iccManager = workingFrame.contentWindow.navigator.mozIccManager;

        if (iccManager) {
          ok(true, "navigator.mozIccManager is instance of " + iccManager.constructor);
        } else {
          ok(true, "navigator.mozIccManager is undefined");
        }

        if (iccManager instanceof MozIccManager) {
          aResolve(iccManager);
        } else {
          aReject();
        }
      });

      document.body.appendChild(workingFrame);
    });
  });
}









function getMozIcc(aIccId) {
  aIccId = aIccId || iccManager.iccIds[0];

  if (!aIccId) {
    ok(true, "iccManager.iccIds[0] is undefined");
    return null;
  }

  return iccManager.getIccById(aIccId);
}









function getMozMobileConnectionByServiceId(aServiceId) {
  aServiceId = aServiceId || 0;
  return workingFrame.contentWindow.navigator.mozMobileConnections[aServiceId];
}















function setRadioEnabled(aEnabled, aServiceId) {
  return getMozMobileConnectionByServiceId(aServiceId).setRadioEnabled(aEnabled)
    .then(() => {
      ok(true, "setRadioEnabled " + aEnabled + " on " + aServiceId + " success.");
    }, (aError) => {
      ok(false, "setRadioEnabled " + aEnabled + " on " + aServiceId + " " +
         aError.name);
    });
}

















function waitForTargetEvent(aEventTarget, aEventName, aMatchFun) {
  return new Promise(function(aResolve, aReject) {
    aEventTarget.addEventListener(aEventName, function onevent(aEvent) {
      if (!aMatchFun || aMatchFun(aEvent)) {
        aEventTarget.removeEventListener(aEventName, onevent);
        ok(true, "Event '" + aEventName + "' got.");
        aResolve(aEvent);
      }
    });
  });
}

















function waitForSystemMessage(aMessageName, aMatchFun) {
  let target = workingFrame.contentWindow.navigator;

  return new Promise(function(aResolve, aReject) {
    target.mozSetMessageHandler(aMessageName, function(aMessage) {
      if (!aMatchFun || aMatchFun(aMessage)) {
        target.mozSetMessageHandler(aMessageName, null);
        ok(true, "System message '" + aMessageName + "' got.");
        aResolve(aMessage);
      }
    });
  });
}
















function setRadioEnabledAndWait(aEnabled, aServiceId) {
  let mobileConn = getMozMobileConnectionByServiceId(aServiceId);
  let promises = [];

  promises.push(waitForTargetEvent(mobileConn, "radiostatechange", function() {
    
    
    return mobileConn.radioState === (aEnabled ? "enabled" : "disabled");
  }));
  promises.push(setRadioEnabled(aEnabled, aServiceId));

  return Promise.all(promises);
}













function restartRadioAndWait(aCardState) {
  return setRadioEnabledAndWait(false).then(() => {
    let promises = [];

    promises.push(waitForTargetEvent(iccManager, "iccdetected")
      .then((aEvent) => {
        let icc = getMozIcc(aEvent.iccId);
        if (icc.cardState !== aCardState) {
          return waitForTargetEvent(icc, "cardstatechange", function() {
            return icc.cardState === aCardState;
          });
        }
      }));
    promises.push(setRadioEnabledAndWait(true));

    return Promise.all(promises);
  });
}
















function setPinLockEnabled(aIcc, aEnabled) {
  let options = {
    lockType: "pin",
    enabled: aEnabled,
    pin: DEFAULT_PIN
  };

  return aIcc.setCardLock(options);
}




function cleanUp() {
  
  ok(true, ":: CLEANING UP ::");

  waitFor(finish, function() {
    return _pendingEmulatorCmdCount === 0;
  });
}









function startTestBase(aTestCaseMain) {
  
  let debugPref = SpecialPowers.getBoolPref(PREF_KEY_RIL_DEBUGGING_ENABLED);
  SpecialPowers.setBoolPref(PREF_KEY_RIL_DEBUGGING_ENABLED, true);

  Promise.resolve()
    .then(aTestCaseMain)
    .catch((aError) => {
      ok(false, "promise rejects during test: " + aError);
    })
    .then(() => {
      
      SpecialPowers.setBoolPref(PREF_KEY_RIL_DEBUGGING_ENABLED, debugPref);
      cleanUp();
    });
}













function startTestCommon(aTestCaseMain, aAdditionalPermissions) {
  startTestBase(function() {
    return ensureIccManager(aAdditionalPermissions)
      .then(aTestCaseMain);
  });
}
