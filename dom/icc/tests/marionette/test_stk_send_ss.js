


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

const TEST_DATA = [
  {command: "D029" + 
            "8103011100" + 
            "82028183" + 
            "850C43616C6C20466F7277617264" + 
            "891091AA120A214365870921436587A901FB", 
   expect: {commandQualifier: 0x00,
            text: "Call Forward"}},
  {command: "D01B" + 
            "8103011100" + 
            "82028183" + 
            "891091AA120A214365870921436587A901FB", 
   expect: {commandQualifier: 0x00}},
  {command: "D081FD" + 
            "8103011100" + 
            "82028183" + 
            "8581EB4576656E2069662074686520466978656420" + 
            "4469616C6C696E67204E756D626572207365727669" +
            "636520697320656E61626C65642C20746865207375" +
            "70706C656D656E7461727920736572766963652063" +
            "6F6E74726F6C20737472696E6720696E636C756465" +
            "6420696E207468652053454E442053532070726F61" +
            "637469766520636F6D6D616E64207368616C6C206E" +
            "6F7420626520636865636B656420616761696E7374" +
            "2074686F7365206F66207468652046444E206C6973" +
            "742E2055706F6E20726563656976696E6720746869" +
            "7320636F6D6D616E642C20746865204D4520736861" +
            "6C6C2064656369" +
            "8904FFBA13FB", 
   expect: {commandQualifier: 0x00,
            text: "Even if the Fixed Dialling Number service is enabled, " +
                  "the supplementary service control string included in the " +
                  "SEND SS proactive command shall not be checked against " +
                  "those of the FDN list. Upon receiving this command, the " +
                  "ME shall deci"}},
  {command: "D01D" + 
            "8103011100" + 
            "82028183" + 
            "8500" + 
            "891091AA120A214365870921436587A901FB", 
   expect: {commandQualifier: 0x00,
            text: ""}},
  {command: "D02B" + 
            "8103011100" + 
            "82028183" + 
            "850A42617369632049636F6E" + 
            "891091AA120A214365870921436587A901FB" + 
            "9E020001", 
   expect: {commandQualifier: 0x00,
            text: "Basic Icon",
            iconSelfExplanatory: true,
            icons: [BASIC_ICON]}},
  {command: "D01F" + 
            "8103011100" + 
            "82028183" + 
            "891091AA120A214365870921436587A901FB" + 
            "9E020103", 
   expect: {commandQualifier: 0x00,
            iconSelfExplanatory: false,
            icons: [COLOR_ICON]}},
  {command: "D036" + 
            "8103011100" + 
            "82028183" + 
            "851980041704140420041004120421042204120423041904220415" + 
            "891091AA120A214365870921436587A901FB", 
   expect: {commandQualifier: 0x00,
            text: "ЗДРАВСТВУЙТЕ"}},
  {command: "D033" + 
            "8103011100" + 
            "82028183" + 
            "851054657874204174747269627574652031" + 
            "891091AA120A214365870921436587A901FB" + 
            "D004001000B4", 
   expect: {commandQualifier: 0x00,
            text: "Text Attribute 1"}},
  {command: "D022" + 
            "8103011100" + 
            "82028183" + 
            "8505804F60597D" + 
            "891091AA120A214365870921436587A901FB", 
   expect: {commandQualifier: 0x00,
            text: "你好"}},
];

function testSendSS(aCommand, aExpect) {
  is(aCommand.commandNumber, 0x01, "commandNumber");
  is(aCommand.typeOfCommand, MozIccManager.STK_CMD_SEND_SS, "typeOfCommand");
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
      log("send_ss_cmd: " + data.command);

      let promises = [];
      
      promises.push(waitForTargetEvent(icc, "stkcommand")
        .then((aEvent) => testSendSS(aEvent.command, data.expect)));
      
      promises.push(waitForSystemMessage("icc-stkcommand")
        .then((aMessage) => {
          is(aMessage.iccId, icc.iccInfo.iccid, "iccId");
          testSendSS(aMessage.command, data.expect);
        }));
      
      promises.push(sendEmulatorStkPdu(data.command));

      return Promise.all(promises);
    });
  }
  return promise;
});
