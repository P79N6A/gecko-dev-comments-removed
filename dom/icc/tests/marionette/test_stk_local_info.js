


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

const TEST_DATA = [
  
  {command: "d009810301260082028182",
   expect: {commandNumber: 0x01,
            commandQualifier: MozIccManager.STK_LOCAL_INFO_LOCATION_INFO,
            localInfoType: MozIccManager.STK_LOCAL_INFO_LOCATION_INFO}},
  
  {command: "d009810301260182028182",
   expect: {commandNumber: 0x01,
            commandQualifier: MozIccManager.STK_LOCAL_INFO_IMEI,
            localInfoType: MozIccManager.STK_LOCAL_INFO_IMEI}},
  
  {command: "d009810301260382028182",
   expect: {commandNumber: 0x01,
            commandQualifier: MozIccManager.STK_LOCAL_INFO_DATE_TIME_ZONE,
            localInfoType: MozIccManager.STK_LOCAL_INFO_DATE_TIME_ZONE}},
  
  {command: "d009810301260482028182",
   expect: {commandNumber: 0x01,
            commandQualifier: MozIccManager.STK_LOCAL_INFO_LANGUAGE,
            localInfoType: MozIccManager.STK_LOCAL_INFO_LANGUAGE}},
];

function testLocalInfo(aCommand, aExpect) {
  is(aCommand.typeOfCommand, MozIccManager.STK_CMD_PROVIDE_LOCAL_INFO, "typeOfCommand");
  is(aCommand.commandQualifier, aExpect.commandQualifier, "commandQualifier");
  is(aCommand.options.localInfoType, aExpect.localInfoType, "options.localInfoType");
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
      
      promises.push(sendEmulatorStkPdu(data.command));

      return Promise.all(promises);
    });
  }
  return promise;
});
