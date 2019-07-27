


subscriptLoader.loadSubScript("resource://gre/modules/ril_consts.js", this);

function run_test() {
  run_next_test();
}




add_test(function test_GsmPDUHelper_readDataCodingScheme() {
  let worker = newWorker({
    postRILMessage: function(data) {
      
    },
    postMessage: function(message) {
      
    }
  });

  let context = worker.ContextPool._contexts[0];
  let helper = context.GsmPDUHelper;
  function test_dcs(dcs, encoding, messageClass, mwi) {
    helper.readHexOctet = function() {
      return dcs;
    }

    let msg = {};
    helper.readDataCodingScheme(msg);

    equal(msg.dcs, dcs);
    equal(msg.encoding, encoding);
    equal(msg.messageClass, messageClass);
    equal(msg.mwi == null, mwi == null);
    if (mwi != null) {
      equal(msg.mwi.active, mwi.active);
      equal(msg.mwi.discard, mwi.discard);
      equal(msg.mwi.msgCount, mwi.msgCount);
    }
  }

  
  
  test_dcs(0x00, PDU_DCS_MSG_CODING_7BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_NORMAL]);
  test_dcs(0x04, PDU_DCS_MSG_CODING_8BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_NORMAL]);
  test_dcs(0x08, PDU_DCS_MSG_CODING_16BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_NORMAL]);
  test_dcs(0x0C, PDU_DCS_MSG_CODING_7BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_NORMAL]);
  
  
  test_dcs(0x01, PDU_DCS_MSG_CODING_7BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_NORMAL]);
  test_dcs(0x02, PDU_DCS_MSG_CODING_7BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_NORMAL]);
  test_dcs(0x03, PDU_DCS_MSG_CODING_7BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_NORMAL]);
  
  test_dcs(0x10, PDU_DCS_MSG_CODING_7BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_0]);
  test_dcs(0x11, PDU_DCS_MSG_CODING_7BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_1]);
  test_dcs(0x12, PDU_DCS_MSG_CODING_7BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_2]);
  test_dcs(0x13, PDU_DCS_MSG_CODING_7BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_3]);

  
  test_dcs(0x50, PDU_DCS_MSG_CODING_7BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_0]);

  
  test_dcs(0x8F, PDU_DCS_MSG_CODING_7BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_NORMAL]);
  test_dcs(0x9F, PDU_DCS_MSG_CODING_7BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_NORMAL]);
  test_dcs(0xAF, PDU_DCS_MSG_CODING_7BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_NORMAL]);
  test_dcs(0xBF, PDU_DCS_MSG_CODING_7BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_NORMAL]);

  
  
  test_dcs(0xC0, PDU_DCS_MSG_CODING_7BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_NORMAL],
           {active: false, discard: true, msgCount: 0});
  test_dcs(0xC8, PDU_DCS_MSG_CODING_7BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_NORMAL],
           {active: true, discard: true, msgCount: -1});
  
  test_dcs(0xCC, PDU_DCS_MSG_CODING_7BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_NORMAL],
           {active: true, discard: true, msgCount: -1});

  
  
  test_dcs(0xD0, PDU_DCS_MSG_CODING_7BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_NORMAL],
           {active: false, discard: false, msgCount: 0});
  test_dcs(0xD8, PDU_DCS_MSG_CODING_7BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_NORMAL],
           {active: true, discard: false, msgCount: -1});
  
  test_dcs(0xDC, PDU_DCS_MSG_CODING_7BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_NORMAL],
           {active: true, discard: false, msgCount: -1});

  
  
  test_dcs(0xE0, PDU_DCS_MSG_CODING_16BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_NORMAL],
           {active: false, discard: false, msgCount: 0});
  test_dcs(0xE8, PDU_DCS_MSG_CODING_16BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_NORMAL],
           {active: true, discard: false, msgCount: -1});
  
  test_dcs(0xEC, PDU_DCS_MSG_CODING_16BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_NORMAL],
           {active: true, discard: false, msgCount: -1});

  
  test_dcs(0xF0, PDU_DCS_MSG_CODING_7BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_0]);
  test_dcs(0xF1, PDU_DCS_MSG_CODING_7BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_1]);
  test_dcs(0xF2, PDU_DCS_MSG_CODING_7BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_2]);
  test_dcs(0xF3, PDU_DCS_MSG_CODING_7BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_3]);
  test_dcs(0xF4, PDU_DCS_MSG_CODING_8BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_0]);
  test_dcs(0xF5, PDU_DCS_MSG_CODING_8BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_1]);
  test_dcs(0xF6, PDU_DCS_MSG_CODING_8BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_2]);
  test_dcs(0xF7, PDU_DCS_MSG_CODING_8BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_3]);
  
  
  test_dcs(0xF8, PDU_DCS_MSG_CODING_7BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_0]);

  run_next_test();
});




add_test(function test_GsmPDUHelper_writeStringAsSeptets() {
  let worker = newWorker({
    postRILMessage: function(data) {
      
    },
    postMessage: function(message) {
      
    }
  });

  let context = worker.ContextPool._contexts[0];
  let helper = context.GsmPDUHelper;
  helper.resetOctetWritten = function() {
    helper.octetsWritten = 0;
  };
  helper.writeHexOctet = function() {
    helper.octetsWritten++;
  };

  let base = "AAAAAAAA"; 
  for (let len = 0; len < 8; len++) {
    let str = base.substring(0, len);

    for (let paddingBits = 0; paddingBits < 8; paddingBits++) {
      do_print("Verifying GsmPDUHelper.writeStringAsSeptets("
               + str + ", " + paddingBits + ", <default>, <default>)");
      helper.resetOctetWritten();
      helper.writeStringAsSeptets(str, paddingBits, PDU_NL_IDENTIFIER_DEFAULT,
                                  PDU_NL_IDENTIFIER_DEFAULT);
      equal(Math.ceil(((len * 7) + paddingBits) / 8),
                  helper.octetsWritten);
    }
  }

  run_next_test();
});




add_test(function test_GsmPDUHelper_readAddress() {
  let worker = newWorker({
    postRILMessage: function(data) {
      
    },
    postMessage: function(message) {
      
    }
  });

  let context = worker.ContextPool._contexts[0];
  let helper = context.GsmPDUHelper;
  function test_address(addrHex, addrString) {
    let uint16Array = [];
    let ix = 0;
    for (let i = 0; i < addrHex.length; ++i) {
      uint16Array[i] = addrHex[i].charCodeAt();
    }

    context.Buf.readUint16 = function(){
      if(ix >= uint16Array.length) {
        do_throw("out of range in uint16Array");
      }
      return uint16Array[ix++];
    }
    let length = helper.readHexOctet();
    let parsedAddr = helper.readAddress(length);
    equal(parsedAddr, addrString);
  }

  
  test_address("04D01100", "_@");
  test_address("04D01000", "\u0394@");

  
  test_address("0B914151245584F6", "+14154255486");
  test_address("0E914151245584B633", "+14154255486#33");

  
  test_address("0BA14151245584F6", "14154255486");

  run_next_test();
});
