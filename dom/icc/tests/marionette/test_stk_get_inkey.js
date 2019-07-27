


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

const TEST_DATA = [
  {command: "D014" + 
            "8103012200" + 
            "82028182" + 
            "8D09004537BD2C07896022", 
   expect: {commandQualifier: 0x00,
            text: "Enter \"0\""}},
  {command: "D00B" + 
            "8103012200" + 
            "82028182" + 
            "8D00", 
   expect: {commandQualifier: 0x00,
            text: null}},
  {command: "D00C" + 
            "8103012200" + 
            "82028182" + 
            "8D0100", 
   expect: {commandQualifier: 0x00,
            text: ""}},
  {command: "D081AD" + 
            "8103012201" + 
            "82028182" + 
            "8D81A104456E746572202278222E205468697320636F6D" + 
            "6D616E6420696E7374727563747320746865204D452074" +
            "6F20646973706C617920746578742C20616E6420746F20" +
            "65787065637420746865207573657220746F20656E7465" +
            "7220612073696E676C65206368617261637465722E2041" +
            "6E7920726573706F6E736520656E746572656420627920" +
            "7468652075736572207368616C6C206265207061737365" +
            "642074",
   expect: {commandQualifier: 0x01,
            text: "Enter \"x\". This command instructs the ME to display " +
                  "text, and to expect the user to enter a single character. " +
                  "Any response entered by the user shall be passed t"}},
  {command: "D016" + 
            "8103012200" + 
            "82028182" + 
            "8D0B043C54494D452D4F55543E", 
   expect: {commandQualifier: 0x00,
            text: "<TIME-OUT>"}},
  {command: "D08199" + 
            "8103012200" + 
            "82028182" + 
            "8D818D0804170414042004100412042104220412042304" + 
            "1904220415041704140420041004120421042204120423" +
            "0419042204150417041404200410041204210422041204" +
            "2304190422041504170414042004100412042104220412" +
            "0423041904220415041704140420041004120421042204" +
            "1204230419042204150417041404200410041204210422" +
            "041204230419",
   expect: {commandQualifier: 0x00,
            text: "ЗДРАВСТВУЙТЕЗДРАВСТВУЙТЕЗДРАВСТВУЙТЕЗДРАВСТВУЙТЕЗДР" +
                  "АВСТВУЙТЕЗДРАВСТВУЙ"}},
  {command: "D011" + 
            "8103012203" + 
            "82028182" + 
            "8D0604456E746572", 
   expect: {commandQualifier: 0x03,
            text: "Enter"}},
  {command: "D015" + 
            "8103012204" + 
            "82028182" + 
            "8D0A04456E74657220594553", 
   expect: {commandQualifier: 0x04,
            text: "Enter YES"}},
  {command: "D019" + 
            "8103012200" + 
            "82028182" + 
            "8D0A043C4E4F2D49434F4E3E" + 
            "1E020002", 
   expect: {commandQualifier: 0x00,
            
            
            text: "<NO-ICON>"}},
  {command: "D016" + 
            "8103012280" + 
            "82028182" + 
            "8D07043C49434F4E3E" + 
            "1E020101", 
   expect: {commandQualifier: 0x80,
            text: "<ICON>",
            iconSelfExplanatory: false,
            icons : [BASIC_ICON]}},
  {command: "D019" + 
            "8103012200" + 
            "82028182" + 
            "8D0A04456E74657220222B22" + 
            "8402010A", 
   expect: {commandQualifier: 0x00,
            text: "Enter \"+\"",
            duration: {timeUnit: MozIccManager.STK_TIME_UNIT_SECOND,
                       timeInterval: 0x0A}}},
];

function testGetInKey(aCommand, aExpect) {
  is(aCommand.commandNumber, 0x01, "commandNumber");
  is(aCommand.typeOfCommand, MozIccManager.STK_CMD_GET_INKEY, "typeOfCommand");
  is(aCommand.commandQualifier, aExpect.commandQualifier, "commandQualifier");

  is(aCommand.options.isAlphabet, !!(aExpect.commandQualifier & 0x01),
     "options.isAlphabet");
  is(aCommand.options.isUCS2, !!(aExpect.commandQualifier & 0x02),
     "options.isUCS2");
  is(aCommand.options.isYesNoRequested, !!(aExpect.commandQualifier & 0x04),
     "options.isYesNoRequested");
  is(aCommand.options.isHelpAvailable, !!(aExpect.commandQualifier & 0x80),
     "options.isHelpAvailable");
  is(aCommand.options.text, aExpect.text, "options.text");
  is(aCommand.options.minLength, 1, "options.minLength");
  is(aCommand.options.maxLength, 1, "options.maxLength");

  
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
      log("get_inkey_cmd: " + data.command);

      let promises = [];
      
      promises.push(waitForTargetEvent(icc, "stkcommand")
        .then((aEvent) => testGetInKey(aEvent.command, data.expect)));
      
      promises.push(sendEmulatorStkPdu(data.command));

      return Promise.all(promises);
    });
  }
  return promise;
});
