


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

const TEST_DATA = [
  {command: "D010" + 
            "8103010101" + 
            "82028182" + 
            "9205013F002FE2", 
   expect: {commandQualifier: 0x01}},
  {command: "D009" + 
            "8103010104" + 
            "82028182", 
   expect: {commandQualifier: 0x04}}
];

function testRefresh(aCommand, aExpect) {
  is(aCommand.commandNumber, 0x01, "commandNumber");
  is(aCommand.typeOfCommand, MozIccManager.STK_CMD_REFRESH, "typeOfCommand");
  is(aCommand.commandQualifier, aExpect.commandQualifier, "commandQualifier");
}


startTestCommon(function() {
  let icc = getMozIcc();
  let promise = Promise.resolve();
  for (let i = 0; i < TEST_DATA.length; i++) {
    let data = TEST_DATA[i];
    promise = promise.then(() => {
      log("refresh_cmd: " + data.command);

      let promises = [];
      
      promises.push(waitForTargetEvent(icc, "stkcommand")
        .then((aEvent) => testRefresh(aEvent.command, data.expect)));
      
      promises.push(waitForSystemMessage("icc-stkcommand")
        .then((aMessage) => {
          is(aMessage.iccId, icc.iccInfo.iccid, "iccId");
          testRefresh(aMessage.command, data.expect);
        }));
      
      promises.push(sendEmulatorStkPdu(data.command));

      return Promise.all(promises);
    });
  }
  return promise;
});
