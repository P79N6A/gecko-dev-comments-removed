


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

const TEST_DATA = [
  {command: "D018" + 
            "8103011500" + 
            "82028182" + 
            "3100" + 
            "050B44656661756C742055524C", 
   expect: {commandQualifier: 0x00,
            url: "",
            mode: MozIccManager.STK_BROWSER_MODE_LAUNCH_IF_NOT_ALREADY_LAUNCHED,
            confirmMessage: { text: "Default URL" }}},
  {command: "D01F" + 
            "8103011500" + 
            "82028182" + 
            "3112687474703A2F2F7878782E7979792E7A7A7A" + 
            "0500", 
   expect: {commandQualifier: 0x00,
            url: "http://xxx.yyy.zzz",
            mode: MozIccManager.STK_BROWSER_MODE_LAUNCH_IF_NOT_ALREADY_LAUNCHED,
            confirmMessage: { text: "" }}},
  {command: "D023" + 
            "8103011500" + 
            "82028182" + 
            "300100" + 
            "3100" + 
            "320103" + 
            "0D10046162632E6465662E6768692E6A6B6C", 
   expect: {commandQualifier: 0x00,
            
            url: "",
            mode: MozIccManager.STK_BROWSER_MODE_LAUNCH_IF_NOT_ALREADY_LAUNCHED}},
  {command: "D018" + 
            "8103011502" + 
            "82028182" + 
            "3100" + 
            "050B44656661756C742055524C", 
   expect: {commandQualifier: 0x02,
            url: "",
            mode: MozIccManager.STK_BROWSER_MODE_USING_EXISTING_BROWSER,
            confirmMessage: { text: "Default URL" }}},
  {command: "D018" + 
            "8103011503" + 
            "82028182" + 
            "3100" + 
            "050B44656661756C742055524C", 
   expect: {commandQualifier: 0x03,
            url: "",
            mode: MozIccManager.STK_BROWSER_MODE_USING_NEW_BROWSER,
            confirmMessage: { text: "Default URL"}}},
  {command: "D026" + 
            "8103011502" + 
            "82028182" + 
            "3100" + 
            "051980041704140420041004120421042204120423" + 
            "041904220415",
   expect: {commandQualifier: 0x02,
            url: "",
            mode: MozIccManager.STK_BROWSER_MODE_USING_EXISTING_BROWSER,
            confirmMessage: { text: "ЗДРАВСТВУЙТЕ" }}},
  {command: "D021" + 
            "8103011502" + 
            "82028182" + 
            "3100" + 
            "05104E6F742073656C66206578706C616E2E" + 
            "1E020101", 
   expect: {commandQualifier: 0x02,
            url: "",
            mode: MozIccManager.STK_BROWSER_MODE_USING_EXISTING_BROWSER,
            confirmMessage: { text: "Not self explan.",
                              iconSelfExplanatory: false,
                              icons : [BASIC_ICON] }
            }},
  {command: "D012" + 
            "8103011502" + 
            "82028182" + 
            "3100" + 
            "0505804F60597D", 
   expect: {commandQualifier: 0x02,
            url: "",
            mode: MozIccManager.STK_BROWSER_MODE_USING_EXISTING_BROWSER,
            confirmMessage: { text: "你好" }}},
  {command: "D00F" + 
            "8103011500" + 
            "82028182" + 
            "3100" + 
            "1E020001", 
   expect: {commandQualifier: 0x00,
            url: "",
            mode: MozIccManager.STK_BROWSER_MODE_LAUNCH_IF_NOT_ALREADY_LAUNCHED,
            confirmMessage: { iconSelfExplanatory: true,
                              icons: [BASIC_ICON] }}},
  {command: "D00F" + 
            "8103011500" + 
            "82028182" + 
            "3100" + 
            "1E020003", 
   expect: {commandQualifier: 0x00,
            url: "",
            mode: MozIccManager.STK_BROWSER_MODE_LAUNCH_IF_NOT_ALREADY_LAUNCHED,
            confirmMessage: { iconSelfExplanatory: true,
                              icons: [COLOR_ICON] }}},
];

function testLaunchBrowser(aCommand, aExpect) {
  is(aCommand.commandNumber, 0x01, "commandNumber");
  is(aCommand.typeOfCommand, MozIccManager.STK_CMD_LAUNCH_BROWSER,
     "typeOfCommand");
  is(aCommand.commandQualifier, aExpect.commandQualifier, "commandQualifier");

  is(aCommand.options.url, aExpect.url, "options.url");
  is(aCommand.options.mode, aExpect.mode, "options.mode");

  
  if ("confirmMessage" in aExpect) {
    isStkText(aCommand.options.confirmMessage, aExpect.confirmMessage,
              "options.confirmMessage");
  }
}


startTestCommon(function() {
  let icc = getMozIcc();
  let promise = Promise.resolve();
  for (let i = 0; i < TEST_DATA.length; i++) {
    let data = TEST_DATA[i];
    promise = promise.then(() => {
      log("launch_browser_cmd: " + data.command);

      let promises = [];
      
      promises.push(waitForTargetEvent(icc, "stkcommand")
        .then((aEvent) => testLaunchBrowser(aEvent.command, data.expect)));
      
      promises.push(sendEmulatorStkPdu(data.command));

      return Promise.all(promises);
    });
  }
  return promise;
});
