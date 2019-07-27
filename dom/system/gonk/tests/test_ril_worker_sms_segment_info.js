


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





add_test(function test_SmsSegmentHelper__countGsm7BitSeptets() {
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

  function do_check_calc(str, expectedCalcLen, lst, sst, strict7BitEncoding, strToWrite) {
    equal(expectedCalcLen,
                gSmsSegmentHelper
                  .countGsm7BitSeptets(str,
                                       PDU_NL_LOCKING_SHIFT_TABLES[lst],
                                       PDU_NL_SINGLE_SHIFT_TABLES[sst],
                                       strict7BitEncoding));

    helper.resetOctetWritten();
    strToWrite = strToWrite || str;
    helper.writeStringAsSeptets(strToWrite, 0, lst, sst);
    equal(Math.ceil(expectedCalcLen * 7 / 8), helper.octetsWritten);
  }

  
  for (let lst = 0; lst < PDU_NL_LOCKING_SHIFT_TABLES.length; lst++) {
    let langTable = PDU_NL_LOCKING_SHIFT_TABLES[lst];

    let str = langTable.substring(0, PDU_NL_EXTENDED_ESCAPE)
              + langTable.substring(PDU_NL_EXTENDED_ESCAPE + 1);

    for (let sst = 0; sst < PDU_NL_SINGLE_SHIFT_TABLES.length; sst++) {
      let langShiftTable = PDU_NL_SINGLE_SHIFT_TABLES[sst];

      
      do_check_calc(ESCAPE + RESCTL, 0, lst, sst);

      
      do_check_calc(str, str.length, lst, sst);

      let [str1, str2] = ["", ""];
      for (let i = 0; i < langShiftTable.length; i++) {
        if ((i == PDU_NL_EXTENDED_ESCAPE) || (i == PDU_NL_RESERVED_CONTROL)) {
          continue;
        }

        let c = langShiftTable[i];
        if (langTable.indexOf(c) >= 0) {
          str1 += c;
        } else {
          str2 += c;
        }
      }

      
      
      do_check_calc(str1, str1.length, lst, sst);

      
      
      do_check_calc(str2, str2.length * 2, lst, sst);
    }
  }

  
  let str = "", strToWrite = "", gsmLen = 0;
  for (let c in GSM_SMS_STRICT_7BIT_CHARMAP) {
    str += c;
    strToWrite += GSM_SMS_STRICT_7BIT_CHARMAP[c];
    if (PDU_NL_LOCKING_SHIFT_TABLES.indexOf(GSM_SMS_STRICT_7BIT_CHARMAP[c])) {
      gsmLen += 1;
    } else {
      gsmLen += 2;
    }
  }
  do_check_calc(str, gsmLen,
                PDU_NL_IDENTIFIER_DEFAULT, PDU_NL_IDENTIFIER_DEFAULT,
                true, strToWrite);

  run_next_test();
});

