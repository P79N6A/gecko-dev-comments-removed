


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

const TEST_DATA = [
  {command: "D00D" + 
            "8103010300" + 
            "82028182" + 
            "8402001A", 
   expect: {commandQualifier: 0x00,
            timeUnit: MozIccManager.STK_TIME_UNIT_MINUTE,
            timeInterval: 0x1A}},
  {command: "D00D" + 
            "8103010300" + 
            "82028182" + 
            "8402010A", 
   expect: {commandQualifier: 0x00,
            timeUnit: MozIccManager.STK_TIME_UNIT_SECOND,
            timeInterval: 0x0A}},
  {command: "D00D" + 
            "8103010300" + 
            "82028182" + 
            "84020205", 
   expect: {commandQualifier: 0x00,
            timeUnit: MozIccManager.STK_TIME_UNIT_TENTH_SECOND,
            timeInterval: 0x05}},
];

function testPollOff(aCommand, aExpect) {
  is(aCommand.commandNumber, 0x01, "commandNumber");
  is(aCommand.typeOfCommand, MozIccManager.STK_CMD_POLL_INTERVAL,
     "typeOfCommand");
  is(aCommand.commandQualifier, aExpect.commandQualifier, "commandQualifier");
  is(aCommand.options.timeUnit, aExpect.timeUnit,
     "options.timeUnit");
  is(aCommand.options.timeInterval, aExpect.timeInterval,
     "options.timeInterval");
}


startTestCommon(function() {
  let icc = getMozIcc();
  let promise = Promise.resolve();
  for (let i = 0; i < TEST_DATA.length; i++) {
    let data = TEST_DATA[i];
    promise = promise.then(() => {
      log("poll_interval_cmd: " + data.command);

      let promises = [];
      
      promises.push(waitForTargetEvent(icc, "stkcommand")
        .then((aEvent) => testPollOff(aEvent.command, data.expect)));
      
      promises.push(waitForSystemMessage("icc-stkcommand")
        .then((aMessage) => {
          is(aMessage.iccId, icc.iccInfo.iccid, "iccId");
          testPollOff(aMessage.command, data.expect);
        }));
      
      promises.push(sendEmulatorStkPdu(data.command));

      return Promise.all(promises);
    });
  }
  return promise;
});
