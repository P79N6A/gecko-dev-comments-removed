


MARIONETTE_TIMEOUT = 90000;
MARIONETTE_HEAD_JS = "head.js";



startTestCommon(function() {
  let icc = getMozIcc();

  
  

  
  
  
  return Promise.resolve()
    .then(() => icc.sendStkEventDownload({
      eventType: MozIccManager.STK_EVENT_TYPE_USER_ACTIVITY
    }))
    .then(() => verifyWithPeekedStkEnvelope(
      "D6" + 
      "07" + 
      "990104" + 
      "82028281" 
    ))

    .then(() => icc.sendStkEventDownload({
      eventType: MozIccManager.STK_EVENT_TYPE_IDLE_SCREEN_AVAILABLE
    }))
    .then(() => verifyWithPeekedStkEnvelope(
      "D6" + 
      "07" + 
      "990105" + 
      "82020281" 
    ))

    .then(() => icc.sendStkEventDownload({
      eventType: MozIccManager.STK_EVENT_TYPE_LOCATION_STATUS,
      locationStatus: MozIccManager.STK_SERVICE_STATE_NORMAL,
      locationInfo: {
        mcc: "466",
        mnc: "92",
        gsmLocationAreaCode: 10291,
        gsmCellId: 19072823
      }
    }))
    .then(() => verifyWithPeekedStkEnvelope(
      "D6" + 
      "15" + 
      "990103" + 
      "82028281" + 
      "9B0100" + 
      "930964F629283301230737" 
    ))

    .then(() => icc.sendStkEventDownload({
      eventType: MozIccManager.STK_EVENT_TYPE_LOCATION_STATUS,
      locationStatus: MozIccManager.STK_SERVICE_STATE_LIMITED,
      
      locationInfo: {
        mcc: "466",
        mnc: "92",
        gsmLocationAreaCode: 10291,
        gsmCellId: 19072823
      }
    }))
    .then(() => verifyWithPeekedStkEnvelope(
      "D6" + 
      "0A" + 
      "990103" + 
      "82028281" + 
      "9B0101" 
    ))

    .then(() => icc.sendStkEventDownload({
      eventType: MozIccManager.STK_EVENT_TYPE_LOCATION_STATUS,
      locationStatus: MozIccManager.STK_SERVICE_STATE_UNAVAILABLE,
    }))
    .then(() => verifyWithPeekedStkEnvelope(
      "D6" + 
      "0A" + 
      "990103" + 
      "82028281" + 
      "9B0102" 
    ))

    .then(() => icc.sendStkEventDownload({
      eventType: MozIccManager.STK_EVENT_TYPE_MT_CALL,
      number: "+9876543210", 
      isIssuedByRemote: true,
      error: null
    }))
    .then(() => verifyWithPeekedStkEnvelope(
      "D6" + 
      "12" + 
      "990100" + 
      "82028381" + 
      "9C0100" + 
      "8606918967452301" 
    ))

    .then(() => icc.sendStkEventDownload({
      eventType: MozIccManager.STK_EVENT_TYPE_MT_CALL,
      number: "987654321", 
      isIssuedByRemote: true,
      error: null
    }))
    .then(() => verifyWithPeekedStkEnvelope(
      "D6" + 
      "12" + 
      "990100" + 
      "82028381" + 
      "9C0100" + 
      "86068189674523F1" 
    ))

    .then(() => icc.sendStkEventDownload({
      eventType: MozIccManager.STK_EVENT_TYPE_CALL_CONNECTED,
      isIssuedByRemote: true,
    }))
    .then(() => verifyWithPeekedStkEnvelope(
      "D6" + 
      "0A" + 
      "990101" + 
      "82028381" + 
      "9C0100" 
    ))

    .then(() => icc.sendStkEventDownload({
      eventType: MozIccManager.STK_EVENT_TYPE_CALL_CONNECTED,
      isIssuedByRemote: false,
    }))
    .then(() => verifyWithPeekedStkEnvelope(
      "D6" + 
      "0A" + 
      "990101" + 
      "82028281" + 
      "9C0100" 
    ))

    .then(() => icc.sendStkEventDownload({
      eventType: MozIccManager.STK_EVENT_TYPE_CALL_DISCONNECTED,
      isIssuedByRemote: false,
      error: "BusyError"
    }))
    .then(() => verifyWithPeekedStkEnvelope(
      "D6" + 
      "0E" + 
      "990102" + 
      "82028281" + 
      "9C0100" + 
      "9A026091" 
    ))

    .then(() => icc.sendStkEventDownload({
      eventType: MozIccManager.STK_EVENT_TYPE_CALL_DISCONNECTED,
      isIssuedByRemote: true,
    }))
    .then(() => verifyWithPeekedStkEnvelope(
      "D6" + 
      "0A" + 
      "990102" + 
      "82028381" + 
      "9C0100" 
    ))

    .then(() => icc.sendStkEventDownload({
      eventType: MozIccManager.STK_EVENT_TYPE_LANGUAGE_SELECTION,
      language: "zh",
    }))
    .then(() => verifyWithPeekedStkEnvelope(
      "D6" + 
      "0B" + 
      "990107" + 
      "82028281" + 
      "2D027A68" 
    ))

    .then(() => icc.sendStkEventDownload({
      eventType: MozIccManager.STK_EVENT_TYPE_BROWSER_TERMINATION,
      terminationCause: MozIccManager.STK_BROWSER_TERMINATION_CAUSE_USER,
    }))
    .then(() => verifyWithPeekedStkEnvelope(
      "D6" + 
      "0A" + 
      "990108" + 
      "82028281" + 
      "B40100" 
    ))

    .then(() => icc.sendStkEventDownload({
      eventType: MozIccManager.STK_EVENT_TYPE_BROWSER_TERMINATION,
      terminationCause: MozIccManager.STK_BROWSER_TERMINATION_CAUSE_ERROR,
    }))
    .then(() => verifyWithPeekedStkEnvelope(
      "D6" + 
      "0A" + 
      "990108" + 
      "82028281" + 
      "B40101" 
    ));
});