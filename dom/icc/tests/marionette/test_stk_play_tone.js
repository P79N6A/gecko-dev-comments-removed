


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

const TEST_DATA = [
  {command: "D009" + 
            "8103012000" + 
            "82028182", 
   expect: {commandQualifier: 0x00}},
  {command: "D014" + 
            "8103012001" + 
            "82028182" + 
            "8509506C617920546F6E65", 
   expect: {commandQualifier: 0x01,
            text: "Play Tone"}},
  {command: "D00B" + 
            "8103012001" + 
            "82028182" + 
            "8500", 
   expect: {commandQualifier: 0x01,
            text: ""}},
  {command: "D00C" + 
            "8103012000" + 
            "82028182" + 
            "8E0101", 
   expect: {commandQualifier: 0x00,
            tone: 0x01}},
  {command: "D00D" + 
            "8103012000" + 
            "82028182" + 
            "84020205", 
   expect: {commandQualifier: 0x00,
            duration: {timeUnit: MozIccManager.STK_TIME_UNIT_TENTH_SECOND,
                       timeInterval: 0x05}}},
  {command: "D00D" + 
            "8103012000" + 
            "82028182" + 
            "1E020101", 
   expect: {commandQualifier: 0x00,
            iconSelfExplanatory: false,
            icons: [BASIC_ICON]}},
  {command: "D01F" + 
            "8103012001" + 
            "82028182" + 
            "8509506C617920546F6E65" + 
            "8E0101" + 
            "84020202" + 
            "1E020003", 
   expect: {commandQualifier: 0x01,
            text: "Play Tone",
            duration: {timeUnit: MozIccManager.STK_TIME_UNIT_TENTH_SECOND,
                       timeInterval: 0x02},
            iconSelfExplanatory: true,
            icons: [COLOR_ICON]}},
];

function testPlayTone(aCommand, aExpect) {
  is(aCommand.commandNumber, 0x01, "commandNumber");
  is(aCommand.typeOfCommand, MozIccManager.STK_CMD_PLAY_TONE, "typeOfCommand");
  is(aCommand.commandQualifier, aExpect.commandQualifier, "commandQualifier");

  is(aCommand.options.isVibrate, !!(aExpect.commandQualifier & 0x01),
     "options.isVibrate");

  
  if ("text" in aExpect) {
    is(aCommand.options.text, aExpect.text, "options.text");
  }

  
  if ("tone" in aExpect) {
    is(aCommand.options.tone, aExpect.tone, "options.tone");
  }

  
  if ("duration" in aExpect) {
    let duration = aCommand.options.duration;
    is(duration.timeUnit, aExpect.duration.timeUnit,
       "options.duration.timeUnit");
    is(duration.timeInterval, aExpect.duration.timeInterval,
       "options.duration.timeInterval");
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
      log("play_tone_cmd: " + data.command);

      let promises = [];
      
      promises.push(waitForTargetEvent(icc, "stkcommand")
        .then((aEvent) => testPlayTone(aEvent.command, data.expect)));
      
      promises.push(waitForSystemMessage("icc-stkcommand")
        .then((aMessage) => {
          is(aMessage.iccId, icc.iccInfo.iccid, "iccId");
          testPlayTone(aMessage.command, data.expect);
        }));
      
      promises.push(sendEmulatorStkPdu(data.command));

      return Promise.all(promises);
    });
  }
  return promise;
});
