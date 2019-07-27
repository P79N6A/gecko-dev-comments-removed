


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

const TEST_DATA = [
  {command: "D050" + 
            "8103011200" + 
            "82028183" + 
            "850A372D6269742055535344" + 
            "8A39F041E19058341E9149E592D9743EA151E9945AB55E" + 
            "B1596D2B2C1E93CBE6333AAD5EB3DBEE373C2E9FD3EBF6" +
            "3B3EAF6FC564335ACD76C3E560",
   expect: {commandQualifier: 0x00,
            text: "7-bit USSD"}},
  {command: "D044" + 
            "8103011200" + 
            "82028183" + 
            "8A39F041E19058341E9149E592D9743EA151E9945AB55E" + 
            "B1596D2B2C1E93CBE6333AAD5EB3DBEE373C2E9FD3EBF6" +
            "3B3EAF6FC564335ACD76C3E560",
   expect: {commandQualifier: 0x00}},
  {command: "D058" + 
            "8103011200" + 
            "82028183" + 
            "850A382D6269742055535344" + 
            "8A41444142434445464748494A4B4C4D4E4F5051525354" + 
            "55565758595A2D6162636465666768696A6B6C6D6E6F70" +
            "7172737475767778797A2D31323334353637383930",
   expect: {commandQualifier: 0x00,
            text: "8-bit USSD"}},
  {command: "D02F" + 
            "8103011200" + 
            "82028183" + 
            "8509554353322055535344" + 
            "8A1948041704140420041004120421042204120423041904220415", 
   expect: {commandQualifier: 0x00,
            text: "UCS2 USSD"}},
  {command: "D081FD" + 
            "8103011200" + 
            "82028183" + 
            "8581B66F6E636520612052454C4541534520434F4D" + 
            "504C455445206D65737361676520636F6E7461696E" +
            "696E672074686520555353442052657475726E2052" +
            "6573756C74206D657373616765206E6F7420636F6E" +
            "7461696E696E6720616E206572726F722068617320" +
            "6265656E2072656365697665642066726F6D207468" +
            "65206E6574776F726B2C20746865204D4520736861" +
            "6C6C20696E666F726D207468652053494D20746861" +
            "742074686520636F6D6D616E6420686173" +
            "8A39F041E19058341E9149E592D9743EA151E9945AB55E" + 
            "B1596D2B2C1E93CBE6333AAD5EB3DBEE373C2E9FD3EBF6" +
            "3B3EAF6FC564335ACD76C3E560",
   expect: {commandQualifier: 0x00,
            text: "once a RELEASE COMPLETE message containing the USSD " +
                  "Return Result message not containing an error has been " +
                  "received from the network, the ME shall inform the SIM " +
                  "that the command has"}},
  {command: "D046" + 
            "8103011200" + 
            "82028183" + 
            "8500" + 
            "8A39F041E19058341E9149E592D9743EA151E9945AB55E" + 
            "B1596D2B2C1E93CBE6333AAD5EB3DBEE373C2E9FD3EBF6" +
            "3B3EAF6FC564335ACD76C3E560",
   expect: {commandQualifier: 0x00,
            text: ""}},
  {command: "D054" + 
            "8103011200" + 
            "82028183" + 
            "850A42617369632049636F6E" + 
            "8A39F041E19058341E9149E592D9743EA151E9945AB55E" + 
            "B1596D2B2C1E93CBE6333AAD5EB3DBEE373C2E9FD3EBF6" +
            "3B3EAF6FC564335ACD76C3E560" +
            "9E020001", 
   expect: {commandQualifier: 0x00,
            text: "Basic Icon",
            iconSelfExplanatory: true,
            icons: [BASIC_ICON]}},
  {command: "D048" + 
            "8103011200" + 
            "82028183" + 
            "8A39F041E19058341E9149E592D9743EA151E9945AB55E" + 
            "B1596D2B2C1E93CBE6333AAD5EB3DBEE373C2E9FD3EBF6" +
            "3B3EAF6FC564335ACD76C3E560" +
            "9E020103", 
   expect: {commandQualifier: 0x00,
            iconSelfExplanatory: false,
            icons: [COLOR_ICON]}},
  {command: "D05F" + 
            "8103011200" + 
            "82028183" + 
            "851980041704140420041004120421042204120423041904220415" + 
            "8A39F041E19058341E9149E592D9743EA151E9945AB55E" + 
            "B1596D2B2C1E93CBE6333AAD5EB3DBEE373C2E9FD3EBF6" +
            "3B3EAF6FC564335ACD76C3E560",
   expect: {commandQualifier: 0x00,
            text: "ЗДРАВСТВУЙТЕ"}},
  {command: "D05C" + 
            "8103011200" + 
            "82028183" + 
            "851054657874204174747269627574652031" + 
            "8A39F041E19058341E9149E592D9743EA151E9945AB55E" + 
            "B1596D2B2C1E93CBE6333AAD5EB3DBEE373C2E9FD3EBF6" +
            "3B3EAF6FC564335ACD76C3E560" +
            "D004001000B4", 
   expect: {commandQualifier: 0x00,
            text: "Text Attribute 1"}},
  {command: "D04A" + 
            "8103011200" + 
            "82028183" + 
            "8A39F041E19058341E9149E592D9743EA151E9945AB55E" + 
            "B1596D2B2C1E93CBE6333AAD5EB3DBEE373C2E9FD3EBF6" +
            "3B3EAF6FC564335ACD76C3E560" +
            "D004001000B4", 
   expect: {commandQualifier: 0x00}},
  {command: "D04B" + 
            "8103011200" + 
            "82028183" + 
            "8505804F60597D" + 
            "8A39F041E19058341E9149E592D9743EA151E9945AB55E" + 
            "B1596D2B2C1E93CBE6333AAD5EB3DBEE373C2E9FD3EBF6" +
            "3B3EAF6FC564335ACD76C3E560",
   expect: {commandQualifier: 0x00,
            text: "你好"}},
];

function testSendUSSD(aCommand, aExpect) {
  is(aCommand.commandNumber, 0x01, "commandNumber");
  is(aCommand.typeOfCommand, MozIccManager.STK_CMD_SEND_USSD, "typeOfCommand");
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
      log("send_ussd_cmd: " + data.command);

      let promises = [];
      
      promises.push(waitForTargetEvent(icc, "stkcommand")
        .then((aEvent) => testSendUSSD(aEvent.command, data.expect)));
      
      promises.push(waitForSystemMessage("icc-stkcommand")
        .then((aMessage) => {
          is(aMessage.iccId, icc.iccInfo.iccid, "iccId");
          testSendUSSD(aMessage.command, data.expect);
        }));
      
      promises.push(sendEmulatorStkPdu(data.command));

      return Promise.all(promises);
    });
  }
  return promise;
});
