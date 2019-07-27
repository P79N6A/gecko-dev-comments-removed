


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

const TEST_DATA = [
  
  {command: "D02E" + 
            "8103014001" + 
            "82028182" + 
            "05074F70656E204944" + 
            "9E020007" + 
            "86099111223344556677F8" + 
            "350702030403041F02" + 
            "39020578", 
   expect: {typeOfCommand: MozIccManager.STK_CMD_OPEN_CHANNEL,
            commandQualifier: 0x01,
            text: "Open ID",
            iconSelfExplanatory: true,
            icons: [COLOR_ICON, COLOR_TRANSPARENCY_ICON]}},
  {command: "D023" + 
            "8103014001" + 
            "82028182" + 
            "0500" + 
            "86099111223344556677F8" + 
            "350702030403041F02" + 
            "39020578", 
   expect: {typeOfCommand: MozIccManager.STK_CMD_OPEN_CHANNEL,
            commandQualifier: 0x01,
            text: ""}},
  {command: "D02C" + 
            "8103014001" + 
            "82028182" + 
            "05094F70656E2049442031" + 
            "86099111223344556677F8" + 
            "350702030403041F02" + 
            "39020578", 
   expect: {typeOfCommand: MozIccManager.STK_CMD_OPEN_CHANNEL,
            commandQualifier: 0x01,
            text: "Open ID 1"}},
  
  {command: "D00D" + 
            "8103014100" + 
            "82028182" + 
            "9E020007", 
   expect: {typeOfCommand: MozIccManager.STK_CMD_CLOSE_CHANNEL,
            commandQualifier: 0x00,
            iconSelfExplanatory: true,
            icons: [COLOR_ICON, COLOR_TRANSPARENCY_ICON]}},
  {command: "D015" + 
            "8103014100" + 
            "82028121" + 
            "850A436C6F73652049442031", 
   expect: {typeOfCommand: MozIccManager.STK_CMD_CLOSE_CHANNEL,
            commandQualifier: 0x00,
            text: "Close ID 1"}},
  
  {command: "D00C" + 
            "8103014200" + 
            "82028121" + 
            "B701C8", 
   expect: {typeOfCommand: MozIccManager.STK_CMD_RECEIVE_DATA,
            commandQualifier: 0x00}},
  {command: "D01C" + 
            "8103014200" + 
            "82028121" + 
            "850E5265636569766520446174612031" + 
            "B701C8", 
   expect: {typeOfCommand: MozIccManager.STK_CMD_RECEIVE_DATA,
            commandQualifier: 0x00,
            text: "Receive Data 1"}},
  
  {command: "D017" + 
            "8103014301" + 
            "82028121" + 
            "9E020007" + 
            "B6080001020304050607", 
   expect: {typeOfCommand: MozIccManager.STK_CMD_SEND_DATA,
            commandQualifier: 0x01,
            iconSelfExplanatory: true,
            icons: [COLOR_ICON, COLOR_TRANSPARENCY_ICON]}},
  {command: "D020" + 
            "8103014301" + 
            "82028121" + 
            "850B53656E6420446174612031" + 
            "B6080001020304050607", 
   expect: {typeOfCommand: MozIccManager.STK_CMD_SEND_DATA,
            commandQualifier: 0x01,
            text: "Send Data 1"}},
];

function testBipCommand(aCommand, aExpect) {
  is(aCommand.commandNumber, 0x01, "commandNumber");
  is(aCommand.typeOfCommand, aExpect.typeOfCommand, "typeOfCommand");
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
      log("bip_cmd: " + data.command);

      let promises = [];
      
      promises.push(waitForTargetEvent(icc, "stkcommand")
        .then((aEvent) => testBipCommand(aEvent.command, data.expect)));
      
      promises.push(sendEmulatorStkPdu(data.command));

      return Promise.all(promises);
    });
  }
  return promise;
});
