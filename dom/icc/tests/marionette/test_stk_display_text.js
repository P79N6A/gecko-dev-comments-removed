


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

const TEST_DATA = [
  {command: "d01a8103012180820281028d0f04546f6f6c6b697420546573742031",
   expect: {commandQualifier: 0x80,
            text: "Toolkit Test 1",
            userClear: true}},
  {command: "d01a8103012181820281028d0f04546f6f6c6b697420546573742032",
   expect: {commandQualifier: 0x81,
            text: "Toolkit Test 2",
            isHighPriority: true,
            userClear: true}},
  {command: "d0198103012180820281028d0e00d4f79bbd4ed341d4f29c0e9a01",
   expect: {commandQualifier: 0x80,
            text: "Toolkit Test 3",
            userClear: true}},
  {command: "d01a8103012100820281028d0f04546f6f6c6b697420546573742034",
   expect: {commandQualifier: 0x00,
            text: "Toolkit Test 4"}},
  {command: "d081ad8103012180820281028d81a1045468697320636f6d6d616e6420696e7374727563747320746865204d4520746f20646973706c617920612074657874206d6573736167652e20497420616c6c6f7773207468652053494d20746f20646566696e6520746865207072696f72697479206f662074686174206d6573736167652c20616e6420746865207465787420737472696e6720666f726d61742e2054776f207479706573206f66207072696f",
   expect: {commandQualifier: 0x80,
            text: "This command instructs the ME to display a text message. It allows the SIM to define the priority of that message, and the text string format. Two types of prio",
            userClear: true}},
  {command: "d01a8103012180820281028d0f043c474f2d4241434b57415244533e",
   expect: {commandQualifier: 0x80,
            text: "<GO-BACKWARDS>",
            userClear: true}},
  {command: "d0248103012180820281028d1908041704140420041004120421042204120423041904220415",
   expect: {commandQualifier: 0x80,
            text: "ЗДРАВСТВУЙТЕ",
            userClear: true}},
  {command: "d0108103012180820281028d05084f60597d",
   expect: {commandQualifier: 0x80,
            text: "你好",
            userClear: true}},
  {command: "d0128103012180820281028d07080038003030eb",
   expect: {commandQualifier: 0x80,
            text: "80ル",
            userClear: true}},
  {command: "d0288103012180820281020d1d00d3309bfc06c95c301aa8e80259c3ec34b9ac07c9602f58ed159bb940",
   expect: {commandQualifier: 0x80,
            text: "Saldo 2.04 E. Validez 20/05/13. ",
            userClear: true}},
  {command: "d0198103012180820281028D0A043130205365636F6E648402010A",
   expect: {commandQualifier: 0x80,
            text: "10 Second",
            userClear: true,
            duration: {timeUnit: MozIccManager.STK_TIME_UNIT_SECOND,
                       timeInterval: 0x0A}}},
  {command: "d01a8103012180820281028d0b0442617369632049636f6e9e020001",
   expect: {commandQualifier: 0x80,
            text: "Basic Icon",
            userClear: true,
            iconSelfExplanatory: true,
            icons: [BASIC_ICON]}},
  {command: "D026810301210082028102" +
            "8D" +
            "1B" +
            "00" + 
            "D4F79BBD4ED341D4F29C0E3A4A9F55A8" +
            "0E8687C158A09B304905",
   expect: {commandQualifier: 0x00,
            text: "Toolkit Test GROUP:0x00, 7BIT"}},
  {command: "D029810301210082028102" +
            "8D" +
            "1E" +
            "04" + 
            "546F6F6C6B697420546573742047524F" +
            "55503A307830302C2038424954",
   expect: {commandQualifier: 0x00,
            text: "Toolkit Test GROUP:0x00, 8BIT"}},
  {command: "D046810301210082028102" +
            "8D" +
            "3B" +
            "08" + 
            "0054006F006F006C006B006900740020" +
            "0054006500730074002000470052004F" +
            "00550050003A0030007800300030002C" +
            "00200055004300530032",
   expect: {commandQualifier: 0x00,
            text: "Toolkit Test GROUP:0x00, UCS2"}},
  {command: "D026810301210082028102" +
            "8D" +
            "1B" +
            "12" + 
            "D4F79BBD4ED341D4F29C0E3A4A9F55A8" +
            "0E868FC158A09B304905",
   expect: {commandQualifier: 0x00,
            text: "Toolkit Test GROUP:0x10, 7BIT"}},
  {command: "D029810301210082028102" +
            "8D" +
            "1E" +
            "16" + 
            "546F6F6C6B697420546573742047524F" +
            "55503A307831302C2038424954",
   expect: {commandQualifier: 0x00,
            text: "Toolkit Test GROUP:0x10, 8BIT"}},
  {command: "D046810301210082028102" +
            "8D" +
            "3B" +
            "1A" + 
            "0054006F006F006C006B006900740020" +
            "0054006500730074002000470052004F" +
            "00550050003A0030007800310030002C" +
            "00200055004300530032",
   expect: {commandQualifier: 0x00,
            text: "Toolkit Test GROUP:0x10, UCS2"}},
  {command: "D026810301210082028102" +
            "8D" +
            "1B" +
            "F2" + 
            "D4F79BBD4ED341D4F29C0E3A4A9F55A8" +
            "0E8637C258A09B304905",
   expect: {commandQualifier: 0x00,
            text: "Toolkit Test GROUP:0xF0, 7BIT"}},
  {command: "D029810301210082028102" +
            "8D" +
            "1E" +
            "F6" + 
            "546F6F6C6B697420546573742047524F" +
            "55503A307846302C2038424954",
   expect: {commandQualifier: 0x00,
            text: "Toolkit Test GROUP:0xF0, 8BIT"}},
  
  
  {command: "D0" +
            "81" + 
            "FC" + 
            "810301210082028102" +
            "8D" +
            "81" + 
            "F0" + 
            "00" + 
            "C332A85D9ECFC3E732685E068DDF6DF8" +
            "7B5E0691CB20D96D061A87E5E131BD2C" +
            "2FCF416537A8FD269741E3771B2E2FCF" +
            "E76517685806B5CBF379F85C0695E774" +
            "50D86C4E8FD165D0BC2E07C1D9F579BA" +
            "5C97CF41E5B13CEC9E83CA7490BB0C22" +
            "BFD374103C3C0795E9F232882E7FBBE3" +
            "F5B20B24BBCD40E5391DC42E83DCEFB6" +
            "585E06B5C3F874BBDE0691CBA071581E" +
            "1ED3CBF2F21C14369BD3637458CC2EBB" +
            "40C3329D5E0699DFEE313DFD76BBC3EC" +
            "34BD0C0A83CAF432280C87CBDF757BB9" +
            "0C8287E5207619346D1E73A0783D0D9A" +
            "9FCA733A885C96BFEBEC32280C9A6689" +
            "CE621654768382D529551A64268B2E",
   expect: {commandQualifier: 0x00,
            text: "Ce message se compose de 273 caracteres en mode " +
                  "compresse. Ce message est affiche sur plusieurs " +
                  "ecrans et ne doit pas etre tronque. 273 est le " +
                  "nombre maximum de caracteres affichable. Cette " +
                  "fonctionnalite a ete approuvee par le SMG9 qui s'est " +
                  "deroule a SYDNEY en AUSTRALIE."}},
];

function testDisplayText(aCommand, aExpect) {
  is(aCommand.typeOfCommand, MozIccManager.STK_CMD_DISPLAY_TEXT,
     "typeOfCommand");
  is(aCommand.commandQualifier, aExpect.commandQualifier, "commandQualifier");
  is(aCommand.options.text, aExpect.text, "options.text");
  is(aCommand.options.userClear, aExpect.userClear, "options.userClear");
  is(aCommand.options.isHighPriority, aExpect.isHighPriority,
     "options.isHighPriority");

  if (aExpect.duration) {
    let duration = aCommand.options.duration;
    is(duration.timeUnit, aExpect.duration.timeUnit,
       "options.duration.timeUnit");
    is(duration.timeInterval, aExpect.duration.timeInterval,
       "options.duration.timeInterval");
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
      log("display_text_cmd: " + data.command);

      let promises = [];
      
      promises.push(waitForTargetEvent(icc, "stkcommand")
        .then((aEvent) => testDisplayText(aEvent.command, data.expect)));
      
      promises.push(sendEmulatorStkPdu(data.command));

      return Promise.all(promises);
    });
  }
  return promise;
});
