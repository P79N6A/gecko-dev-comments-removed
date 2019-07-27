


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

const TEST_DATA = [
  {command: "d029810301110082028183850c43616c6c20466f7277617264891091aa120a214365870921436587a901fb",
   expect: {commandQualifier: 0x00,
            text: "Call Forward"}},
  {command: "d01b810301110082028183891091aa120a214365870921436587a901fb",
   expect: {name: "send_ss_cmd_1_without_alpha_identifier",
            commandQualifier: 0x00}},
  {command: "d02d810301110082028183850c43616c6c20466f7277617264891491aa120a21436587092143658709214365a711fb",
   expect: {name: "send_ss_cmd_2_with_alpha_identifier",
            commandQualifier: 0x00,
            text: "Call Forward"}},
  {command: "d01f810301110082028183891491aa120a21436587092143658709214365a711fb",
   expect: {commandQualifier: 0x00}},
  {command: "d081fd8103011100820281838581eb4576656e20696620746865204669786564204469616c6c696e67204e756d626572207365727669636520697320656e61626c65642c2074686520737570706c656d656e74617279207365727669636520636f6e74726f6c20737472696e6720696e636c7564656420696e207468652053454e442053532070726f61637469766520636f6d6d616e64207368616c6c206e6f7420626520636865636b656420616761696e73742074686f7365206f66207468652046444e206c6973742e2055706f6e20726563656976696e67207468697320636f6d6d616e642c20746865204d45207368616c6c20646563698904ffba13fb",
   expect: {commandQualifier: 0x00,
            text: "Even if the Fixed Dialling Number service is enabled, the supplementary service control string included in the SEND SS proactive command shall not be checked against those of the FDN list. Upon receiving this command, the ME shall deci"}},
  {command: "d00f8103011100820281838904ffba13fb",
   expect: {commandQualifier: 0x00}},
  {command: "d01d8103011100820281838500891091aa120a214365870921436587a901fb",
   expect: {commandQualifier: 0x00,
            text: ""}},
  {command: "d02b810301110082028183850a42617369632049636f6e891091aa120a214365870921436587a901fb9e020001",
   expect: {commandQualifier: 0x00,
            text: "Basic Icon",
            iconSelfExplanatory: true,
            icons: [BASIC_ICON]}},
  {command: "d01f810301110082028183891091aa120a214365870921436587a901fb9e020001",
   expect: {commandQualifier: 0x00,
            iconSelfExplanatory: true,
            icons: [BASIC_ICON]}},
  {command: "d02c810301110082028183850b436f6c6f75722049636f6e891091aa120a214365870921436587a901fb9e020003",
   expect: {commandQualifier: 0x00,
            text: "Colour Icon",
            iconSelfExplanatory: true,
            icons: [COLOR_ICON]}},
  {command: "d01f810301110082028183891091aa120a214365870921436587a901fb9e020003",
   expect: {commandQualifier: 0x00,
            iconSelfExplanatory: true,
            icons: [COLOR_ICON]}},
  {command: "d02b810301110082028183850a42617369632049636f6e891091aa120a214365870921436587a901fb9e020101",
   expect: {commandQualifier: 0x00,
            text: "Basic Icon",
            iconSelfExplanatory: false,
            icons: [BASIC_ICON]}},
  {command: "d01f810301110082028183891091aa120a214365870921436587a901fb9e020101",
   expect: {commandQualifier: 0x00,
            iconSelfExplanatory: false,
            icons: [BASIC_ICON]}},
  {command: "d036810301110082028183851980041704140420041004120421042204120423041904220415891091aa120a214365870921436587a901fb",
   expect: {commandQualifier: 0x00,
            text: "ЗДРАВСТВУЙТЕ"}},
  {command: "d033810301110082028183851054657874204174747269627574652031891091aa120a214365870921436587a901fbd004001000b4",
   expect: {commandQualifier: 0x00,
            text: "Text Attribute 1"}},
  {command: "d021810301110082028183891091aa120a214365870921436587a901fbd004001000b4",
   expect: {commandQualifier: 0x00}},
  {command: "d02d810301110082028183851054657874204174747269627574652032891091aa120a214365870921436587a901fb",
   expect: {commandQualifier: 0x00,
            text: "Text Attribute 2"}},
  {command: "d033810301110082028183851054657874204174747269627574652031891091aa120a214365870921436587a901fbd004001001b4",
   expect: {commandQualifier: 0x00,
            text: "Text Attribute 1"}},
  {command: "d021810301110082028183891091aa120a214365870921436587a901fbd004001001b4",
   expect: {commandQualifier: 0x00}},
  {command: "d033810301110082028183851054657874204174747269627574652031891091aa120a214365870921436587a901fbd004001002b4",
   expect: {commandQualifier: 0x00,
            text: "Text Attribute 1"}},
  {command: "d021810301110082028183891091aa120a214365870921436587a901fbd004001002b4",
   expect: {commandQualifier: 0x00}},
  {command: "d033810301110082028183851054657874204174747269627574652031891091aa120a214365870921436587a901fbd004001004b4",
   expect: {commandQualifier: 0x00,
            text: "Text Attribute 1"}},
  {command: "d021810301110082028183891091aa120a214365870921436587a901fbd004001004b4",
   expect: {commandQualifier: 0x00}},
  {command: "d033810301110082028183851054657874204174747269627574652032891091aa120a214365870921436587a901fbd004001000b4",
   expect: {commandQualifier: 0x00,
            text: "Text Attribute 2"}},
  {command: "d02d810301110082028183851054657874204174747269627574652033891091aa120a214365870921436587a901fb",
   expect: {commandQualifier: 0x00,
            text: "Text Attribute 3"}},
  {command: "d033810301110082028183851054657874204174747269627574652031891091aa120a214365870921436587a901fbd004001008b4",
   expect: {commandQualifier: 0x00,
            text: "Text Attribute 1"}},
  {command: "d021810301110082028183891091aa120a214365870921436587a901fbd004001008b4",
   expect: {commandQualifier: 0x00}},
  {command: "d033810301110082028183851054657874204174747269627574652031891091aa120a214365870921436587a901fbd004001010b4",
   expect: {commandQualifier: 0x00,
            text: "Text Attribute 1"}},
  {command: "d021810301110082028183891091aa120a214365870921436587a901fbd004001010b4",
   expect: {commandQualifier: 0x00}},
  {command: "d033810301110082028183851054657874204174747269627574652031891091aa120a214365870921436587a901fbd004001020b4",
   expect: {commandQualifier: 0x00,
            text: "Text Attribute 1"}},
  {command: "d021810301110082028183891091aa120a214365870921436587a901fbd004001020b4",
   expect: {commandQualifier: 0x00}},
  {command: "d033810301110082028183851054657874204174747269627574652031891091aa120a214365870921436587a901fbd004001040b4",
   expect: {commandQualifier: 0x00,
            text: "Text Attribute 1"}},
  {command: "d021810301110082028183891091aa120a214365870921436587a901fbd004001040b4",
   expect: {commandQualifier: 0x00}},
  {command: "d033810301110082028183851054657874204174747269627574652031891091aa120a214365870921436587a901fbd004001080b4",
   expect: {commandQualifier: 0x00,
            text: "Text Attribute 1"}},
  {command: "d021810301110082028183891091aa120a214365870921436587a901fbd004001080b4",
   expect: {commandQualifier: 0x00}},
  {command: "d0228103011100820281838505804f60597d891091aa120a214365870921436587a901fb",
   expect: {commandQualifier: 0x00,
            text: "你好"}},
  {command: "d02081030111008202818385038030eb891091aa120a214365870921436587a901fb",
   expect: {commandQualifier: 0x00,
            text: "ル"}},
];

function testSendSS(aCommand, aExpect) {
  is(aCommand.typeOfCommand, MozIccManager.STK_CMD_SEND_SS, "typeOfCommand");
  is(aCommand.commandQualifier, aExpect.commandQualifier, "commandQualifier");

  if (aExpect.text) {
    is(aCommand.options.text, aExpect.text, "options.text");
  }

  if (aExpect.icons) {
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
      
      promises.push(sendEmulatorStkPdu(data.command));

      return Promise.all(promises);
    });
  }
  return promise;
});
