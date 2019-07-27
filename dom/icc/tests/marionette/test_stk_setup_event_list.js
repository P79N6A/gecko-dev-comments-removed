


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

const TEST_DATA = [
  {command: "D00C" + 
            "8103010500" + 
            "82028182" + 
            "990104", 
   expect: {commandQualifier: 0x00,
            eventList: [4]}},
  {command: "D00D" + 
            "8103010500" + 
            "82028182" + 
            "99020507", 
   expect: {commandQualifier: 0x00,
            eventList: [5, 7]}},
  {command: "D00C" + 
            "8103010500" + 
            "82028182" + 
            "990107", 
   expect: {commandQualifier: 0x00,
            eventList: [7]}},
  {command: "D00C" + 
            "8103010500" + 
            "82028182" + 
            "990107", 
   expect: {commandQualifier: 0x00,
            eventList: [7]}},
  {command: "D00B" + 
            "8103010500" + 
            "82028182" + 
            "9900", 
   expect: {commandQualifier: 0x00,
            eventList: null}},
  {command: "D00C" + 
            "8103010500" + 
            "82028182" + 
            "990107", 
   expect: {commandQualifier: 0x00,
            eventList: [7]}}
];

function testSetupEventList(aCommand, aExpect) {
  is(aCommand.commandNumber, 0x01, "commandNumber");
  is(aCommand.typeOfCommand, MozIccManager.STK_CMD_SET_UP_EVENT_LIST,
     "typeOfCommand");
  is(aCommand.commandQualifier, aExpect.commandQualifier, "commandQualifier");

  for (let index in aExpect.eventList) {
    is(aCommand.options.eventList[index], aExpect.eventList[index],
       "options.eventList[" + index + "]");
  }
}


startTestCommon(function() {
  let icc = getMozIcc();
  let promise = Promise.resolve();
  for (let i = 0; i < TEST_DATA.length; i++) {
    let data = TEST_DATA[i];
    promise = promise.then(() => {
      log("setup_event_list_cmd: " + data.command);

      let promises = [];
      
      promises.push(waitForTargetEvent(icc, "stkcommand")
        .then((aEvent) => testSetupEventList(aEvent.command, data.expect)));
      
      promises.push(sendEmulatorStkPdu(data.command));

      return Promise.all(promises);
    });
  }
  return promise;
});
