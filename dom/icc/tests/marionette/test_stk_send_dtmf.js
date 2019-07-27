


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

const TEST_DATA = [
  {command: "D01B" + 
            "8103011400" + 
            "82028183" + 
            "850953656E642044544D46" + 
            "AC052143658709", 
   expect: {commandQualifier: 0x00,
            text: "Send DTMF"}},
  {command: "D010" + 
            "8103011400" + 
            "82028183" + 
            "AC052143658709", 
   expect: {commandQualifier: 0x00}},
  {command: "D013" + 
            "8103011400" + 
            "82028183" + 
            "8500" + 
            "AC06C1CCCCCCCC2C", 
   expect: {commandQualifier: 0x00,
            text: ""}},
  {command: "D01D" + 
            "8103011400" + 
            "82028183" + 
            "850A42617369632049636F6E" + 
            "AC02C1F2" + 
            "9E020001", 
   expect: {commandQualifier: 0x00,
            text: "Basic Icon",
            iconSelfExplanatory: true,
            icons: [BASIC_ICON]}},
  {command: "D011" + 
            "8103011400" + 
            "82028183" + 
            "AC02C1F2" + 
            "9E020005", 
   expect: {commandQualifier: 0x00,
            iconSelfExplanatory: true,
            icons: [COLOR_TRANSPARENCY_ICON]}},
  {command: "D028" + 
            "8103011400" + 
            "82028183" + 
            "851980041704140420041004120421042204120423041904220415" + 
            "AC02C1F2", 
   expect: {commandQualifier: 0x00,
            text: "ЗДРАВСТВУЙТЕ"}},
  {command: "D023" + 
            "8103011400" + 
            "82028183" + 
            "850B53656E642044544D462031" + 
            "AC052143658709" + 
            "D004000B00B4", 
   expect: {commandQualifier: 0x00,
            text: "Send DTMF 1"}},
  {command: "D014" + 
            "8103011400" + 
            "82028183" + 
            "8505804F60597D" + 
            "AC02C1F2", 
   expect: {commandQualifier: 0x00,
            text: "你好"}},
];

function testSendDTMF(aCommand, aExpect) {
  is(aCommand.commandNumber, 0x01, "commandNumber");
  is(aCommand.typeOfCommand, MozIccManager.STK_CMD_SEND_DTMF, "typeOfCommand");
  is(aCommand.commandQualifier, aExpect.commandQualifier, "commandQualifier");

  
  if ("text" in aExpect) {
    is(aCommand.options.text, aExpect.text, "options.text");
  }

  
  if ("icons" in aExpect) {
    isIcons(aCommand.options.icons, aExpect.icons);
    is(aCommand.options.iconSelfExplanatory, aExpect.iconSelfExplanatory,
       "options.iconSelfExplanatory");
  }
}


startTestCommon(function() {
  let icc = getMozIcc();
  let promise = Promise.resolve();
  for (let i = 0; i < TEST_DATA.length; i++) {
    let data = TEST_DATA[i];
    promise = promise.then(() => {
      log("send_dtmf_cmd: " + data.command);

      let promises = [];
      
      promises.push(waitForTargetEvent(icc, "stkcommand")
        .then((aEvent) => testSendDTMF(aEvent.command, data.expect)));
      
      promises.push(waitForSystemMessage("icc-stkcommand")
        .then((aMessage) => {
          is(aMessage.iccId, icc.iccInfo.iccid, "iccId");
          testSendDTMF(aMessage.command, data.expect);
        }));
      
      promises.push(sendEmulatorStkPdu(data.command));

      return Promise.all(promises);
    });
  }
  return promise;
});
