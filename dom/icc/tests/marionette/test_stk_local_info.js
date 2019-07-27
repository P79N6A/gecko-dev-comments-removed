


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

const TEST_DATA = [
  
  {command: "D009" + 
            "8103012600" + 
            "82028182", 
   expect: {commandQualifier: MozIccManager.STK_LOCAL_INFO_LOCATION_INFO,
            localInfoType: MozIccManager.STK_LOCAL_INFO_LOCATION_INFO}},
  
  {command: "D009" + 
            "8103012601" + 
            "82028182", 
   expect: {commandQualifier: MozIccManager.STK_LOCAL_INFO_IMEI,
            localInfoType: MozIccManager.STK_LOCAL_INFO_IMEI}},
  
  {command: "D009" + 
            "8103012603" + 
            "82028182", 
   expect: {commandQualifier: MozIccManager.STK_LOCAL_INFO_DATE_TIME_ZONE,
            localInfoType: MozIccManager.STK_LOCAL_INFO_DATE_TIME_ZONE}},
  
  {command: "D009" + 
            "8103012604" + 
            "82028182", 
   expect: {commandQualifier: MozIccManager.STK_LOCAL_INFO_LANGUAGE,
            localInfoType: MozIccManager.STK_LOCAL_INFO_LANGUAGE}},
];

function testLocalInfo(aCommand, aExpect) {
  is(aCommand.commandNumber, 0x01, "commandNumber");
  is(aCommand.typeOfCommand, MozIccManager.STK_CMD_PROVIDE_LOCAL_INFO,
     "typeOfCommand");
  is(aCommand.commandQualifier, aExpect.commandQualifier, "commandQualifier");
  is(aCommand.options.localInfoType, aExpect.localInfoType,
     "options.localInfoType");
}


startTestCommon(function() {
  let icc = getMozIcc();
  let promise = Promise.resolve();
  for (let i = 0; i < TEST_DATA.length; i++) {
    let data = TEST_DATA[i];
    promise = promise.then(() => {
      log("local_info_cmd: " + data.command);

      let promises = [];
      
      promises.push(waitForTargetEvent(icc, "stkcommand")
        .then((aEvent) => testLocalInfo(aEvent.command, data.expect)));
      
      promises.push(waitForSystemMessage("icc-stkcommand")
        .then((aMessage) => {
          is(aMessage.iccId, icc.iccInfo.iccid, "iccId");
          testLocalInfo(aMessage.command, data.expect);
        }));
      
      promises.push(sendEmulatorStkPdu(data.command));

      return Promise.all(promises);
    });
  }
  return promise;
});
