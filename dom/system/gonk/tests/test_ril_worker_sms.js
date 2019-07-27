


subscriptLoader.loadSubScript("resource://gre/modules/ril_consts.js", this);

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyGetter(this, "gSmsSegmentHelper", function() {
  let ns = {};
  Cu.import("resource://gre/modules/SmsSegmentHelper.jsm", ns);
  return ns.SmsSegmentHelper;
});

const ESCAPE = "\uffff";
const RESCTL = "\ufffe";

function run_test() {
  run_next_test();
}





function hexToNibble(nibble) {
  nibble &= 0x0f;
  if (nibble < 10) {
    nibble += 48; 
  } else {
    nibble += 55; 
  }
  return nibble;
}

function pduToParcelData(pdu) {
  let dataLength = 4 + pdu.length * 4 + 4;
  let data = new Uint8Array(dataLength);
  let offset = 0;

  
  data[offset++] = pdu.length & 0xFF;
  data[offset++] = (pdu.length >> 8) & 0xFF;
  data[offset++] = (pdu.length >> 16) & 0xFF;
  data[offset++] = (pdu.length >> 24) & 0xFF;

  
  for (let i = 0; i < pdu.length; i++) {
    let hi = (pdu[i] >>> 4) & 0x0F;
    let lo = pdu[i] & 0x0F;

    data[offset++] = hexToNibble(hi);
    data[offset++] = 0;
    data[offset++] = hexToNibble(lo);
    data[offset++] = 0;
  }

  
  data[offset++] = 0;
  data[offset++] = 0;
  data[offset++] = 0;
  data[offset++] = 0;

  return data;
}

function compose7bitPdu(lst, sst, data, septets) {
  if ((lst == 0) && (sst == 0)) {
    return [0x00,                              
            PDU_MTI_SMS_DELIVER,               
            1, 0x00, 0,                        
            0x00,                              
            PDU_DCS_MSG_CODING_7BITS_ALPHABET, 
            0, 0, 0, 0, 0, 0, 0,               
            septets]                           
           .concat(data);
  }

  return [0x00,                                            
          PDU_MTI_SMS_DELIVER | PDU_UDHI,                  
          1, 0x00, 0,                                      
          0x00,                                            
          PDU_DCS_MSG_CODING_7BITS_ALPHABET,               
          0, 0, 0, 0, 0, 0, 0,                             
          8 + septets,                                     
          6,                                               
          PDU_IEI_NATIONAL_LANGUAGE_LOCKING_SHIFT, 1, lst, 
          PDU_IEI_NATIONAL_LANGUAGE_SINGLE_SHIFT, 1, sst]  
         .concat(data);
}

function composeUcs2Pdu(rawBytes) {
  return [0x00,                               
          PDU_MTI_SMS_DELIVER,                
          1, 0x00, 0,                         
          0x00,                               
          PDU_DCS_MSG_CODING_16BITS_ALPHABET, 
          0, 0, 0, 0, 0, 0, 0,                
          rawBytes.length]                    
         .concat(rawBytes);
}

function newSmsParcel(pdu) {
  return newIncomingParcel(-1,
                           RESPONSE_TYPE_UNSOLICITED,
                           UNSOLICITED_RESPONSE_NEW_SMS,
                           pduToParcelData(pdu));
}

function removeSpecialChar(str, needle) {
  for (let i = 0; i < needle.length; i++) {
    let pos;
    while ((pos = str.indexOf(needle[i])) >= 0) {
      str = str.substring(0, pos) + str.substring(pos + 1);
    }
  }
  return str;
}

function newWriteHexOctetAsUint8Worker() {
  let worker = newWorker({
    postRILMessage: function(data) {
      
    },
    postMessage: function(message) {
      
    }
  });

  let context = worker.ContextPool._contexts[0];
  context.GsmPDUHelper.writeHexOctet = function(value) {
    context.Buf.writeUint8(value);
  };

  return worker;
}

function add_test_receiving_sms(expected, pdu) {
  add_test(function test_receiving_sms() {
    let worker = newWorker({
      postRILMessage: function(data) {
        
      },
      postMessage: function(message) {
        do_print("fullBody: " + message.fullBody);
        equal(expected, message.fullBody)
      }
    });

    do_print("expect: " + expected);
    do_print("pdu: " + pdu);
    worker.onRILMessage(0, newSmsParcel(pdu));

    run_next_test();
  });
}

let test_receiving_7bit_alphabets__worker;
function test_receiving_7bit_alphabets(lst, sst) {
  if (!test_receiving_7bit_alphabets__worker) {
    test_receiving_7bit_alphabets__worker = newWriteHexOctetAsUint8Worker();
  }
  let worker = test_receiving_7bit_alphabets__worker;
  let context = worker.ContextPool._contexts[0];
  let helper = context.GsmPDUHelper;
  let buf = context.Buf;

  function get7bitRawBytes(expected) {
    buf.outgoingIndex = 0;
    helper.writeStringAsSeptets(expected, 0, lst, sst);

    let subArray = buf.outgoingBytes.subarray(0, buf.outgoingIndex);
    return Array.slice(subArray);
  }

  let langTable = PDU_NL_LOCKING_SHIFT_TABLES[lst];
  let langShiftTable = PDU_NL_SINGLE_SHIFT_TABLES[sst];

  let text = removeSpecialChar(langTable + langShiftTable, ESCAPE + RESCTL);
  for (let i = 0; i < text.length;) {
    let len = Math.min(70, text.length - i);
    let expected = text.substring(i, i + len);
    let septets =
      gSmsSegmentHelper.countGsm7BitSeptets(expected, langTable, langShiftTable);
    let rawBytes = get7bitRawBytes(expected);
    let pdu = compose7bitPdu(lst, sst, rawBytes, septets);
    add_test_receiving_sms(expected, pdu);

    i += len;
  }
}

function test_receiving_ucs2_alphabets(text) {
  let worker = test_receiving_7bit_alphabets__worker;
  let context = worker.ContextPool._contexts[0];
  let buf = context.Buf;

  function getUCS2RawBytes(expected) {
    buf.outgoingIndex = 0;
    context.GsmPDUHelper.writeUCS2String(expected);

    let subArray = buf.outgoingBytes.subarray(0, buf.outgoingIndex);
    return Array.slice(subArray);
  }

  for (let i = 0; i < text.length;) {
    let len = Math.min(70, text.length - i);
    let expected = text.substring(i, i + len);
    let rawBytes = getUCS2RawBytes(expected);
    let pdu = composeUcs2Pdu(rawBytes);
    add_test_receiving_sms(expected, pdu);

    i += len;
  }
}

let ucs2str = "";
for (let lst = 0; lst < PDU_NL_LOCKING_SHIFT_TABLES.length; lst++) {
  ucs2str += PDU_NL_LOCKING_SHIFT_TABLES[lst];
  for (let sst = 0; sst < PDU_NL_SINGLE_SHIFT_TABLES.length; sst++) {
    test_receiving_7bit_alphabets(lst, sst);

    if (lst == 0) {
      ucs2str += PDU_NL_SINGLE_SHIFT_TABLES[sst];
    }
  }
}
test_receiving_ucs2_alphabets(ucs2str);


add_test(function test_sendSMS_UCS2_without_langIndex_langShiftIndex_defined() {
  let worker = newWriteHexOctetAsUint8Worker();
  let context = worker.ContextPool._contexts[0];

  context.Buf.sendParcel = function() {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    equal(this.outgoingIndex, 57);

    run_next_test();
  };

  context.RIL.sendSMS({
    number: "1",
    segmentMaxSeq: 2,
    fullBody: "Hello World!",
    dcs: PDU_DCS_MSG_CODING_16BITS_ALPHABET,
    segmentRef16Bit: false,
    userDataHeaderLength: 5,
    requestStatusReport: true,
    segments: [
      {
        body: "Hello ",
        encodedBodyLength: 12,
      }, {
        body: "World!",
        encodedBodyLength: 12,
      }
    ],
  });
});
