


MARIONETTE_TIMEOUT = 120000;
MARIONETTE_HEAD_JS = "head.js";



startTestCommon(function() {
  let icc = getMozIcc();

  
  

  
  
  
  return Promise.resolve()
    .then(() => icc.sendStkResponse({
      commandNumber: 0x01,
      typeOfCommand: MozIccManager.STK_CMD_LAUNCH_BROWSER,
      commandQualifier: 0x00,
      options: {
        url: "",
        mode: MozIccManager.STK_BROWSER_MODE_LAUNCH_IF_NOT_ALREADY_LAUNCHED
      }
    }, {
      resultCode: MozIccManager.STK_RESULT_OK
    }))
    .then(() => verifyWithPeekedStkResponse(
      "8103011500" + 
      "02028281" + 
      "830100"  
    ))

    .then(() => icc.sendStkResponse({
      commandNumber: 0x02,
      typeOfCommand: MozIccManager.STK_CMD_DISPLAY_TEXT,
      commandQualifier: 0x01,
      options: {
        text: "Toolkit Test 1",
        responseNeeded: false
      }
    }, {
      resultCode:
        MozIccManager.STK_RESULT_TERMINAL_CRNTLY_UNABLE_TO_PROCESS,
      additionalInformation:
        MozIccManager.STK_ADDITIONAL_INFO_ME_PROBLEM_SCREEN_IS_BUSY
    }))
    .then(() => verifyWithPeekedStkResponse(
      "8103022101" + 
      "02028281" + 
      "83022001"  
    ))

    .then(() => icc.sendStkResponse({
      commandNumber: 0x03,
      typeOfCommand: MozIccManager.STK_CMD_SELECT_ITEM,
      commandQualifier: 0x00,
      options: {
        title: "Toolkit Select",
        items: [{identifier: 1, text: "Item 1"},
                {identifier: 2, text: "Item 2"},
                {identifier: 3, text: "Item 3"},
                {identifier: 4, text: "Item 4"}]
      }
    }, {
      resultCode: MozIccManager.STK_RESULT_HELP_INFO_REQUIRED,
      itemIdentifier: 5
    }))
    .then(() => verifyWithPeekedStkResponse(
      "8103032400" + 
      "02028281" + 
      "830113" + 
      "900105" 
    ))

    .then(() => icc.sendStkResponse({
      commandNumber: 0x04,
      typeOfCommand: MozIccManager.STK_CMD_GET_INKEY,
      commandQualifier: 0x04,
      options: {
        text: "Enter Y/N"
      }
    }, {
      resultCode: MozIccManager.STK_RESULT_OK,
      isYesNo: true
    }))
    .then(() => verifyWithPeekedStkResponse(
      "8103042204" + 
      "02028281" + 
      "830100" + 
      "8D020401" 
    ))

    .then(() => icc.sendStkResponse({
      commandNumber: 0x05,
      typeOfCommand: MozIccManager.STK_CMD_GET_INKEY,
      commandQualifier: 0x04,
      options: {
        text: "Enter Y/N",
        duration: {
          timeUnit: MozIccManager.STK_TIME_UNIT_SECOND,
          timeInterval: 10
        }
      }
    }, {
      resultCode: MozIccManager.STK_RESULT_NO_RESPONSE_FROM_USER,
    }))
    .then(() => verifyWithPeekedStkResponse(
      "8103052204" + 
      "02028281" + 
      "830112" + 
      "0402010A" 
    ))

    .then(() => icc.sendStkResponse({
      commandNumber: 0x06,
      typeOfCommand: MozIccManager.STK_CMD_GET_INPUT,
      commandQualifier: 0x01,
      options: {
        text: "Enter Yes"
      }
    }, {
      resultCode: MozIccManager.STK_RESULT_OK,
      input: "Yes"
    }))
    .then(() => verifyWithPeekedStkResponse(
      "8103062301" + 
      "02028281" + 
      "830100" + 
      "8D0404596573" 
    ))

    .then(() => icc.sendStkResponse({
      commandNumber: 0x07,
      typeOfCommand: MozIccManager.STK_CMD_TIMER_MANAGEMENT,
      commandQualifier: MozIccManager.STK_TIMER_START,
      options: {
        timerAction: MozIccManager.STK_TIMER_START,
        timerId: 0x01,
        timerValue: (0x01 * 60 * 60) + (0x02 * 60) + 0x03 
      }
    }, {
      resultCode: MozIccManager.STK_RESULT_OK,
      timer: {
        timerAction: MozIccManager.STK_TIMER_START,
        timerId: 0x01,
        timerValue: (0x01 * 60 * 60) + (0x02 * 60) + 0x03 
      },
    }))
    .then(() => verifyWithPeekedStkResponse(
      "8103072700" + 
      "02028281" + 
      "830100" + 
      "240101" + 
      "2503102030" 
    ))

    .then(() => icc.sendStkResponse({
      commandNumber: 0x08,
      typeOfCommand: MozIccManager.STK_CMD_TIMER_MANAGEMENT,
      commandQualifier: MozIccManager.STK_TIMER_DEACTIVATE,
      options: {
        timerAction: MozIccManager.STK_TIMER_DEACTIVATE,
        timerId: 0x02,
        timerValue: (0x01 * 60 * 60) + (0x02 * 60) + 0x03 
      }
    }, {
      resultCode: MozIccManager.STK_RESULT_OK,
      timer: {
        timerId: 0x02
      },
    }))
    .then(() => verifyWithPeekedStkResponse(
      "8103082701" + 
      "02028281" + 
      "830100" + 
      "240102" 
    ))

    .then(() => icc.sendStkResponse({
      commandNumber: 0x09,
      typeOfCommand: MozIccManager.STK_CMD_TIMER_MANAGEMENT,
      commandQualifier: MozIccManager.STK_TIMER_GET_CURRENT_VALUE,
      options: {
        timerAction: MozIccManager.STK_TIMER_GET_CURRENT_VALUE,
        timerId: 0x03,
      }
    }, {
      resultCode: MozIccManager.STK_RESULT_ACTION_CONTRADICTION_TIMER_STATE,
      timer: {
        timerId: 0x03
      },
    }))
    .then(() => verifyWithPeekedStkResponse(
      "8103092702" + 
      "02028281" + 
      "830124" + 
      "240103" 
    ))

    .then(() => icc.sendStkResponse({
      commandNumber: 0x0A,
      typeOfCommand: MozIccManager.STK_CMD_PROVIDE_LOCAL_INFO,
      commandQualifier: MozIccManager.STK_LOCAL_INFO_LOCATION_INFO,
      options: {
        localInfoType: MozIccManager.STK_LOCAL_INFO_LOCATION_INFO
      }
    }, {
      resultCode: MozIccManager.STK_RESULT_OK,
      localInfo: {
        locationInfo: {
          mcc: "466",
          mnc: "92",
          gsmLocationAreaCode: 10291,
          gsmCellId: 19072823
        }
      },
    }))
    .then(() => verifyWithPeekedStkResponse(
      "81030A2600" + 
      "02028281" + 
      "830100" + 
      "930964F629283301230737" 
    ))

    .then(() => icc.sendStkResponse({
      commandNumber: 0x0B,
      typeOfCommand: MozIccManager.STK_CMD_PROVIDE_LOCAL_INFO,
      commandQualifier: MozIccManager.STK_LOCAL_INFO_IMEI,
      options: {
        localInfoType: MozIccManager.STK_LOCAL_INFO_IMEI
      }
    }, {
      resultCode: MozIccManager.STK_RESULT_OK,
      localInfo: {
        imei: "123456789012345"
      },
    }))
    .then(() => verifyWithPeekedStkResponse(
      "81030B2601" + 
      "02028281" + 
      "830100" + 
      "14081234567890123450" 
    ))

    .then(() => icc.sendStkResponse({
      commandNumber: 0x0C,
      typeOfCommand: MozIccManager.STK_CMD_PROVIDE_LOCAL_INFO,
      commandQualifier: MozIccManager.STK_LOCAL_INFO_DATE_TIME_ZONE,
      options: {
        localInfoType: MozIccManager.STK_LOCAL_INFO_DATE_TIME_ZONE
      }
    }, {
      resultCode: MozIccManager.STK_RESULT_OK,
      localInfo: {
        
        date: new Date(Date.UTC(2012, 2, 4, 5, 6, 7))
      },
    }))
    .then(() => verifyWithPeekedStkResponse(
      "81030C2603" + 
      "02028281" + 
      "830100" + 
      "260721304050607000" 
    ))

    .then(() => icc.sendStkResponse({
      commandNumber: 0x0D,
      typeOfCommand: MozIccManager.STK_CMD_PROVIDE_LOCAL_INFO,
      commandQualifier: MozIccManager.STK_LOCAL_INFO_LANGUAGE,
      options: {
        localInfoType: MozIccManager.STK_LOCAL_INFO_LANGUAGE
      }
    }, {
      resultCode: MozIccManager.STK_RESULT_OK,
      localInfo: {
        language: "zh"
      },
    }))
    .then(() => verifyWithPeekedStkResponse(
      "81030D2604" + 
      "02028281" + 
      "830100" + 
      "2D027A68" 
    ));
});