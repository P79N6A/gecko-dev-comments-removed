


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

const TEST_DATA = [
  {command: "D02E" + 
            "8103011300" + 
            "82028183" + 
            "86099111223344556677F8" + 
            "8B180100099110325476F840F40C54657374204D657373616765", 
   expect: {commandQualifier: 0x00}},
  {command: "D03D" + 
            "8103011300" + 
            "82028183" + 
            "850D53686F7274204D657373616765" + 
            "86099111223344556677F8" + 
            "8B180100099110325476F840F00D53F45B4E0735CBF379F85C06", 
   expect: {commandQualifier: 0x00,
            text: "Short Message"}},
  {command: "D081FD" + 
            "8103011301" + 
            "82028183" + 
            "853854686520616464726573732064617461206F62" + 
            "6A65637420686F6C64732074686520525011446573" +
            "74696E6174696F6E1141646472657373" +
            "86099111223344556677F8" + 
            "8B81AC0100099110325476F840F4A054776F20747970" + 
            "65732061726520646566696E65643A202D2041207368" +
            "6F7274206D65737361676520746F2062652073656E74" +
            "20746F20746865206E6574776F726B20696E20616E20" +
            "534D532D5355424D4954206D6573736167652C206F72" +
            "20616E20534D532D434F4D4D414E44206D6573736167" +
            "652C2077686572652074686520757365722064617461" +
            "2063616E20626520706173736564207472616E7370",
   expect: {commandQualifier: 0x01,
            text: "The address data object holds the RP_Destination_Address"}},
  {command: "D081C3" + 
            "8103011301" + 
            "82028183" + 
            "86099111223344556677F8" + 
            "8B81AC0100099110325476F840F4A054776F20747970" + 
            "65732061726520646566696E65643A202D2041207368" +
            "6F7274206D65737361676520746F2062652073656E74" +
            "20746F20746865206E6574776F726B20696E20616E20" +
            "534D532D5355424D4954206D6573736167652C206F72" +
            "20616E20534D532D434F4D4D414E44206D6573736167" +
            "652C2077686572652074686520757365722064617461" +
            "2063616E20626520706173736564207472616E7370",
   expect: {commandQualifier: 0x01}},
  {command: "D081FD" + 
            "8103011300" + 
            "82028183" + 
            "8581E654776F207479706573206172652064656669" + 
            "6E65643A202D20412073686F7274206D6573736167" +
            "6520746F2062652073656E7420746F20746865206E" +
            "6574776F726B20696E20616E20534D532D5355424D" +
            "4954206D6573736167652C206F7220616E20534D53" +
            "2D434F4D4D414E44206D6573736167652C20776865" +
            "726520746865207573657220646174612063616E20" +
            "626520706173736564207472616E73706172656E74" +
            "6C793B202D20412073686F7274206D657373616765" +
            "20746F2062652073656E7420746F20746865206E65" +
            "74776F726B20696E20616E20534D532D5355424D49" +
            "5420" +
            "8B09010002911040F00120", 
   expect: {commandQualifier: 0x00,
            text: "Two types are defined: - A short message to be sent to " +
                  "the network in an SMS-SUBMIT message, or an SMS-COMMAND " +
                  "message, where the user data can be passed transparently; " +
                  "- A short message to be sent to the network in an " +
                  "SMS-SUBMIT "}},
  {command: "D030" + 
            "8103011300" + 
            "82028183" + 
            "8500" + 
            "86099111223344556677F8" + 
            "8B180100099110325476F840F40C54657374204D657373616765", 
   expect: {commandQualifier: 0x00,
            text: ""}},
  {command: "D055" + 
            "8103011300" + 
            "82028183" + 
            "851980041704140420041004120421042204120423041904220415" + 
            "86099111223344556677F8" + 
            "8B240100099110325476F8400818041704140420041004120421042204120423041904220415", 
   expect: {commandQualifier: 0x00,
            text: "ЗДРАВСТВУЙТЕ"}},
  {command: "D04B" + 
            "8103011300" + 
            "82028183" + 
            "850F810C089794A09092A1A292A399A295" + 
            "86099111223344556677F8" + 
            "8B240100099110325476F8400818041704140420041004120421042204120423041904220415", 
   expect: {commandQualifier: 0x00,
            text: "ЗДРАВСТВУЙТЕ"}},
  {command: "D03B" + 
            "8103011300" + 
            "82028183" + 
            "85074E4F2049434F4E" + 
            "86099111223344556677F8" + 
            "8B180100099110325476F840F40C54657374204D657373616765" + 
            "9E020002", 
   expect: {commandQualifier: 0x00,
            
            
            text: "NO ICON"}},
  {command: "D032" + 
            "8103011300" + 
            "82028183" + 
            "86099111223344556677F8" + 
            "8B180100099110325476F840F40C54657374204D657373616765" + 
            "9E020001", 
   expect: {commandQualifier: 0x00,
            iconSelfExplanatory: true,
            icons: [BASIC_ICON]}},
  {command: "D03B" + 
            "8103011300" + 
            "82028183" + 
            "850753656E6420534D" + 
            "86099111223344556677F8" + 
            "8B180100099110325476F840F40C54657374204D657373616765" + 
            "1E020101", 
   expect: {commandQualifier: 0x00,
            text: "Send SM",
            iconSelfExplanatory: false,
            icons: [BASIC_ICON]}},
  {command: "D02C" + 
            "8103011300" + 
            "82028183" + 
            "851054657874204174747269627574652031" + 
            "8B09010002911040F00120" + 
            "D004001000B4", 
   expect: {commandQualifier: 0x00,
            text: "Text Attribute 1"}},
  {command: "D035" + 
            "8103011300" + 
            "82028183" + 
            "8509800038003030EB0030" + 
            "86099111223344556677F8" + 
            "8B140100099110325476F84008080038003030EB0031",
   expect: {commandQualifier: 0x00, 
            text: "80ル0"}},
  {command: "D033" + 
            "8103011300" + 
            "82028183" + 
            "85078104613831EB31" + 
            "86099111223344556677F8" + 
            "8B140100099110325476F84008080038003030EB0032",
   expect: {commandQualifier: 0x00, 
            text: "81ル1"}},
  {command: "D034" + 
            "8103011300" + 
            "82028183" + 
            "8508820430A03832CB32" + 
            "86099111223344556677F8" + 
            "8B140100099110325476F84008080038003030EB0033",
   expect: {commandQualifier: 0x00, 
            text: "82ル2"}},
];

function testSendSMS(aCommand, aExpect) {
  is(aCommand.commandNumber, 0x01, "commandNumber");
  is(aCommand.typeOfCommand, MozIccManager.STK_CMD_SEND_SMS, "typeOfCommand");
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
      log("send_sms_cmd: " + data.command);

      let promises = [];
      
      promises.push(waitForTargetEvent(icc, "stkcommand")
        .then((aEvent) => testSendSMS(aEvent.command, data.expect)));
      
      promises.push(waitForSystemMessage("icc-stkcommand")
        .then((aMessage) => {
          is(aMessage.iccId, icc.iccInfo.iccid, "iccId");
          testSendSMS(aMessage.command, data.expect);
        }));
      
      promises.push(sendEmulatorStkPdu(data.command));

      return Promise.all(promises);
    });
  }
  return promise;
});
