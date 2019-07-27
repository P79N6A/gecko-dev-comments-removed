


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

const TEST_DATA = [
  {command: "d0188103011500820281823100050b44656661756c742055524c",
   expect: {commandQualifier: 0x00,
            url: "",
            confirmMessage: { text: "Default URL" }}},
  {command: "d01f8103011500820281823112687474703a2f2f7878782e7979792e7a7a7a0500",
   expect: {commandQualifier: 0x00,
            url: "http://xxx.yyy.zzz",
            confirmMessage: { text: "" }}},
  {command: "d00e8103011500820281823001003100",
   expect: {commandQualifier: 0x00,
            url: ""}},
  {command: "d0208103011500820281823100320103" +
            "0d10046162632e6465662e6768692e6a6b6c", 
   expect: {commandQualifier: 0x00,
            url: ""}},
  {command: "d0188103011502820281823100050b44656661756c742055524c",
   expect: {commandQualifier: 0x02,
            url: "",
            confirmMessage: { text: "Default URL" }}},
  {command: "d0188103011503820281823100050b44656661756c742055524c",
   expect: {commandQualifier: 0x03,
            url: "",
            confirmMessage: { text: "Default URL"}}},
  {command: "d00b8103011500820281823100",
   expect: {commandQualifier: 0x00,
            url: ""}},
  {command: "d0268103011502820281823100051980041704140420041004120421042204120423041904220415",
   expect: {commandQualifier: 0x02,
            url: "",
            confirmMessage: { text: "ЗДРАВСТВУЙТЕ" }}},
  {command: "d021810301150282028182310005104e6f742073656c66206578706c616e2e1e020101",
   expect: {commandQualifier: 0x02,
            url: "",
            confirmMessage: { text: "Not self explan.",
                              iconSelfExplanatory: false,
                              icons : [BASIC_ICON] }
            }},
  {command: "d01d8103011502820281823100050c53656c66206578706c616e2e1e020001",
   expect: {commandQualifier: 0x02,
            url: "",
            confirmMessage: { text: "Self explan.",
                              iconSelfExplanatory: true,
                              icons : [BASIC_ICON] }
            }},
  {command: "d0208103011500820281823100050d44656661756c742055524c2031d004000d00b4",
   expect: {commandQualifier: 0x00,
            url: "",
            confirmMessage: { text: "Default URL 1" }}},
  {command: "d01a8103011500820281823100050d44656661756c742055524c2032",
   expect: {commandQualifier: 0x00,
            url: "",
            confirmMessage: { text: "Default URL 2" }}},
  {command: "d0208103011500820281823100050d44656661756c742055524c2031d004000d01b4",
   expect: {commandQualifier: 0x00,
            url: "",
            confirmMessage: { text: "Default URL 1" }}},
  {command: "d01a8103011500820281823100050d44656661756c742055524c2032",
   expect: {commandQualifier: 0x00,
            url: "",
            confirmMessage: { text: "Default URL 2" }}},
  {command: "d0208103011500820281823100050d44656661756c742055524c2031d004000d02b4",
   expect: {commandQualifier: 0x00,
            url: "",
            confirmMessage: { text: "Default URL 1" }}},
  {command: "d01a8103011500820281823100050d44656661756c742055524c2032",
   expect: {commandQualifier: 0x00,
            url: "",
            confirmMessage: { text: "Default URL 2" }}},
  {command: "d0208103011500820281823100050d44656661756c742055524c2031d004000d04b4",
   expect: {commandQualifier: 0x00,
            url: "",
            confirmMessage: { text: "Default URL 1" }}},
  {command: "d0208103011500820281823100050d44656661756c742055524c2032d004000d00b4",
   expect: {commandQualifier: 0x00,
            url: "",
            confirmMessage: { text: "Default URL 2" }}},
  {command: "d01a8103011500820281823100050d44656661756c742055524c2033",
   expect: {commandQualifier: 0x00,
            url: "",
            confirmMessage: { text: "Default URL 3" }}},
  {command: "d0208103011500820281823100050d44656661756c742055524c2031d004000d08b4",
   expect: {commandQualifier: 0x00,
            url: "",
            confirmMessage: { text: "Default URL 1" }}},
  {command: "d0208103011500820281823100050d44656661756c742055524c2032d004000d00b4",
   expect: {commandQualifier: 0x00,
            url: "",
            confirmMessage: { text: "Default URL 2" }}},
  {command: "d01a8103011500820281823100050d44656661756c742055524c2033",
   expect: {commandQualifier: 0x00,
            url: "",
            confirmMessage: { text: "Default URL 3" }}},
  {command: "d0208103011500820281823100050d44656661756c742055524c2031d004000d10b4",
   expect: {commandQualifier: 0x00,
            url: "",
            confirmMessage: { text: "Default URL 1" }}},
  {command: "d0208103011500820281823100050d44656661756c742055524c2032d004000d00b4",
   expect: {commandQualifier: 0x00,
            url: "",
            confirmMessage: { text: "Default URL 2" }}},
  {command: "d01a8103011500820281823100050d44656661756c742055524c2033",
   expect: {commandQualifier: 0x00,
            url: "",
            confirmMessage: { text: "Default URL 3" }}},
  {command: "d0208103011500820281823100050d44656661756c742055524c2031d004000d20b4",
   expect: {commandQualifier: 0x00,
            url: "",
            confirmMessage: { text: "Default URL 1" }}},
  {command: "d0208103011500820281823100050d44656661756c742055524c2032d004000d00b4",
   expect: {commandQualifier: 0x00,
            url: "",
            confirmMessage: { text: "Default URL 2" }}},
  {command: "d01a8103011500820281823100050d44656661756c742055524c2033",
   expect: {commandQualifier: 0x00,
            url: "",
            confirmMessage: { text: "Default URL 3" }}},
  {command: "d0208103011500820281823100050d44656661756c742055524c2031d004000d40b4",
   expect: {commandQualifier: 0x00,
            url: "",
            confirmMessage: { text: "Default URL 1" }}},
  {command: "d0208103011500820281823100050d44656661756c742055524c2032d004000d00b4",
   expect: {commandQualifier: 0x00,
            url: "",
            confirmMessage: { text: "Default URL 2" }}},
  {command: "d01a8103011500820281823100050d44656661756c742055524c2033",
   expect: {commandQualifier: 0x00,
            url: "",
            confirmMessage: { text: "Default URL 3" }}},
  {command: "d0208103011500820281823100050d44656661756c742055524c2031d004000d80b4",
   expect: {commandQualifier: 0x00,
            url: "",
            confirmMessage: { text: "Default URL 1" }}},
  {command: "d0208103011500820281823100050d44656661756c742055524c2032d004000d00b4",
   expect: {commandQualifier: 0x00,
            url: "",
            confirmMessage: { text: "Default URL 2" }}},
  {command: "d01a8103011500820281823100050d44656661756c742055524c2033",
   expect: {commandQualifier: 0x00,
            url: "",
            confirmMessage: { text: "Default URL 3" }}},
  {command: "d0208103011500820281823100050d44656661756c742055524c2031d004000d00b4",
   expect: {commandQualifier: 0x00,
            url: "",
            confirmMessage: { text: "Default URL 1" }}},
  {command: "d01a8103011500820281823100050d44656661756c742055524c2032",
   expect: {commandQualifier: 0x00,
            url: "",
            confirmMessage: { text: "Default URL 2" }}},
  {command: "d01281030115028202818231000505804f60597d",
   expect: {commandQualifier: 0x02,
            url: "",
            confirmMessage: { text: "你好" }}},
  {command: "d010810301150282028182310005038030eb",
   expect: {commandQualifier: 0x02,
            url: "",
            confirmMessage: { text: "ル" }}},
  {command: "d01281030115008202818230010031001e020001",
   expect: {commandQualifier: 0x00,
            url: "",
            confirmMessage: { iconSelfExplanatory: true,
                              icons: [BASIC_ICON] }}},
  {command: "d01281030115008202818230010031001e020003",
   expect: {commandQualifier: 0x00,
            url: "",
            confirmMessage: { iconSelfExplanatory: true,
                              icons: [COLOR_ICON] }}},
];

function testLaunchBrowser(aCommand, aExpect) {
  is(aCommand.typeOfCommand, MozIccManager.STK_CMD_LAUNCH_BROWSER,
     "typeOfCommand");
  is(aCommand.commandQualifier, aExpect.commandQualifier, "commandQualifier");
  is(aCommand.options.url, aExpect.url, "options.url");

  if (aExpect.confirmMessage) {
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
