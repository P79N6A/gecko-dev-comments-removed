


subscriptLoader.loadSubScript("resource://gre/modules/ril_consts.js", this);

function run_test() {
  run_next_test();
}




add_test(function test_reading_ad_and_parsing_mcc_mnc() {
  let worker = newUint8Worker();
  let context = worker.ContextPool._contexts[0];
  let record = context.SimRecordHelper;
  let helper = context.GsmPDUHelper;
  let ril    = context.RIL;
  let buf    = context.Buf;
  let io     = context.ICCIOHelper;

  function do_test(mncLengthInEf, imsi, expectedMcc, expectedMnc) {
    ril.iccInfoPrivate.imsi = imsi;

    io.loadTransparentEF = function fakeLoadTransparentEF(options) {
      let ad = [0x00, 0x00, 0x00];
      if (typeof mncLengthInEf === 'number') {
        ad.push(mncLengthInEf);
      }

      
      buf.writeInt32(ad.length * 2);

      
      for (let i = 0; i < ad.length; i++) {
        helper.writeHexOctet(ad[i]);
      }

      
      buf.writeStringDelimiter(ad.length * 2);

      if (options.callback) {
        options.callback(options);
      }
    };

    record.readAD();

    do_check_eq(ril.iccInfo.mcc, expectedMcc);
    do_check_eq(ril.iccInfo.mnc, expectedMnc);
  }

  do_test(undefined, "466923202422409", "466", "92" );
  do_test(0x00,      "466923202422409", "466", "92" );
  do_test(0x01,      "466923202422409", "466", "92" );
  do_test(0x02,      "466923202422409", "466", "92" );
  do_test(0x03,      "466923202422409", "466", "923");
  do_test(0x04,      "466923202422409", "466", "92" );
  do_test(0xff,      "466923202422409", "466", "92" );

  do_test(undefined, "310260542718417", "310", "260");
  do_test(0x00,      "310260542718417", "310", "260");
  do_test(0x01,      "310260542718417", "310", "260");
  do_test(0x02,      "310260542718417", "310", "26" );
  do_test(0x03,      "310260542718417", "310", "260");
  do_test(0x04,      "310260542718417", "310", "260");
  do_test(0xff,      "310260542718417", "310", "260");

  run_next_test();
});

add_test(function test_reading_optional_efs() {
  let worker = newUint8Worker();
  let context = worker.ContextPool._contexts[0];
  let record = context.SimRecordHelper;
  let gsmPdu = context.GsmPDUHelper;
  let ril    = context.RIL;
  let buf    = context.Buf;
  let io     = context.ICCIOHelper;

  function buildSST(supportedEf) {
    let sst = [];
    let len = supportedEf.length;
    for (let i = 0; i < len; i++) {
      let index, bitmask, iccService;
      if (ril.appType === CARD_APPTYPE_SIM) {
        iccService = GECKO_ICC_SERVICES.sim[supportedEf[i]];
        iccService -= 1;
        index = Math.floor(iccService / 4);
        bitmask = 2 << ((iccService % 4) << 1);
      } else if (ril.appType === CARD_APPTYPE_USIM){
        iccService = GECKO_ICC_SERVICES.usim[supportedEf[i]];
        iccService -= 1;
        index = Math.floor(iccService / 8);
        bitmask = 1 << ((iccService % 8) << 0);
      }

      if (sst) {
        sst[index] |= bitmask;
      }
    }
    return sst;
  }

  ril.updateCellBroadcastConfig = function fakeUpdateCellBroadcastConfig() {
    
  };

  function do_test(sst, supportedEf) {
    
    let testEf = supportedEf.slice(0);

    record.readMSISDN = function fakeReadMSISDN() {
      testEf.splice(testEf.indexOf("MSISDN"), 1);
    };

    record.readMBDN = function fakeReadMBDN() {
      testEf.splice(testEf.indexOf("MDN"), 1);
    };

    record.readMWIS = function fakeReadMWIS() {
      testEf.splice(testEf.indexOf("MWIS"), 1);
    };

    io.loadTransparentEF = function fakeLoadTransparentEF(options) {
      
      buf.writeInt32(sst.length * 2);

      
      for (let i = 0; i < sst.length; i++) {
         gsmPdu.writeHexOctet(sst[i] || 0);
      }

      
      buf.writeStringDelimiter(sst.length * 2);

      if (options.callback) {
        options.callback(options);
      }

      if (testEf.length !== 0) {
        do_print("Un-handled EF: " + JSON.stringify(testEf));
        do_check_true(false);
      }
    };

    record.readSST();
  }

  
  let supportedEf = ["MSISDN", "MDN", "MWIS"];
  ril.appType = CARD_APPTYPE_SIM;
  do_test(buildSST(supportedEf), supportedEf);
  ril.appType = CARD_APPTYPE_USIM;
  do_test(buildSST(supportedEf), supportedEf);

  run_next_test();
});




add_test(function test_fetch_sim_recodes() {
  let worker = newWorker();
  let context = worker.ContextPool._contexts[0];
  let RIL = context.RIL;
  let iccRecord = context.ICCRecordHelper;
  let simRecord = context.SimRecordHelper;

  function testFetchSimRecordes(expectCalled) {
    let ifCalled = [];

    RIL.getIMSI = function() {
      ifCalled.push("getIMSI");
    };

    simRecord.readAD = function() {
      ifCalled.push("readAD");
    };

    simRecord.readSST = function() {
      ifCalled.push("readSST");
    };

    simRecord.fetchSimRecords();

    for (let i = 0; i < expectCalled.length; i++ ) {
      if (ifCalled[i] != expectCalled[i]) {
        do_print(expectCalled[i] + " is not called.");
        do_check_true(false);
      }
    }
  }

  let expectCalled = ["getIMSI", "readAD", "readSST"];
  testFetchSimRecordes(expectCalled);

  run_next_test();
});




add_test(function test_read_mwis() {
  let worker = newUint8Worker();
  let context = worker.ContextPool._contexts[0];
  let helper = context.GsmPDUHelper;
  let recordHelper = context.SimRecordHelper;
  let buf    = context.Buf;
  let io     = context.ICCIOHelper;
  let mwisData;
  let postedMessage;

  worker.postMessage = function fakePostMessage(message) {
    postedMessage = message;
  };

  io.loadLinearFixedEF = function fakeLoadLinearFixedEF(options) {
    if (mwisData) {
      
      buf.writeInt32(mwisData.length * 2);

      
      for (let i = 0; i < mwisData.length; i++) {
        helper.writeHexOctet(mwisData[i]);
      }

      
      buf.writeStringDelimiter(mwisData.length * 2);

      options.recordSize = mwisData.length;
      if (options.callback) {
        options.callback(options);
      }
    } else {
      do_print("mwisData[] is not set.");
    }
  };

  function buildMwisData(isActive, msgCount) {
    if (msgCount < 0 || msgCount === GECKO_VOICEMAIL_MESSAGE_COUNT_UNKNOWN) {
      msgCount = 0;
    } else if (msgCount > 255) {
      msgCount = 255;
    }

    mwisData =  [ (isActive) ? 0x01 : 0x00,
                  msgCount,
                  0xFF, 0xFF, 0xFF ];
  }

  function do_test(isActive, msgCount) {
    buildMwisData(isActive, msgCount);
    recordHelper.readMWIS();

    do_check_eq("iccmwis", postedMessage.rilMessageType);
    do_check_eq(isActive, postedMessage.mwi.active);
    do_check_eq((isActive) ? msgCount : 0, postedMessage.mwi.msgCount);
  }

  do_test(true, GECKO_VOICEMAIL_MESSAGE_COUNT_UNKNOWN);
  do_test(true, 1);
  do_test(true, 255);

  do_test(false, 0);
  do_test(false, 255); 

  run_next_test();
});




add_test(function test_update_mwis() {
  let worker = newUint8Worker();
  let context = worker.ContextPool._contexts[0];
  let pduHelper = context.GsmPDUHelper;
  let ril = context.RIL;
  ril.appType = CARD_APPTYPE_USIM;
  ril.iccInfoPrivate.mwis = [0x00, 0x00, 0x00, 0x00, 0x00];
  let recordHelper = context.SimRecordHelper;
  let buf = context.Buf;
  let ioHelper = context.ICCIOHelper;
  let recordSize = ril.iccInfoPrivate.mwis.length;
  let recordNum = 1;

  ioHelper.updateLinearFixedEF = function(options) {
    options.pathId = context.ICCFileHelper.getEFPath(options.fileId);
    options.command = ICC_COMMAND_UPDATE_RECORD;
    options.p1 = options.recordNumber;
    options.p2 = READ_RECORD_ABSOLUTE_MODE;
    options.p3 = recordSize;
    ril.iccIO(options);
  };

  function do_test(isActive, count) {
    let mwis = ril.iccInfoPrivate.mwis;
    let isUpdated = false;

    function buildMwisData() {
      let result = mwis.slice(0);
      result[0] = isActive? (mwis[0] | 0x01) : (mwis[0] & 0xFE);
      result[1] = (count === GECKO_VOICEMAIL_MESSAGE_COUNT_UNKNOWN) ? 0 : count;

      return result;
    }

    buf.sendParcel = function() {
      isUpdated = true;

      
      do_check_eq(this.readInt32(), REQUEST_SIM_IO);

      
      this.readInt32();

      
      do_check_eq(this.readInt32(), ICC_COMMAND_UPDATE_RECORD);

      
      do_check_eq(this.readInt32(), ICC_EF_MWIS);

      
      do_check_eq(this.readString(),
                  EF_PATH_MF_SIM + ((ril.appType === CARD_APPTYPE_USIM) ? EF_PATH_ADF_USIM : EF_PATH_DF_GSM));

      
      do_check_eq(this.readInt32(), recordNum);

      
      do_check_eq(this.readInt32(), READ_RECORD_ABSOLUTE_MODE);

      
      do_check_eq(this.readInt32(), recordSize);

      
      let strLen = this.readInt32();
      do_check_eq(recordSize * 2, strLen);
      let expectedMwis = buildMwisData();
      for (let i = 0; i < recordSize; i++) {
        do_check_eq(expectedMwis[i], pduHelper.readHexOctet());
      }
      this.readStringDelimiter(strLen);

      
      do_check_eq(this.readString(), null);

      if (!ril.v5Legacy) {
        
        this.readInt32();
      }
    };

    do_check_false(isUpdated);

    recordHelper.updateMWIS({ active: isActive,
                              msgCount: count });

    do_check_true((ril.iccInfoPrivate.mwis) ? isUpdated : !isUpdated);
  }

  do_test(true, GECKO_VOICEMAIL_MESSAGE_COUNT_UNKNOWN);
  do_test(true, 1);
  do_test(true, 255);

  do_test(false, 0);

  
  ril.appType = CARD_APPTYPE_SIM;
  do_test(false, 0);

  
  
  delete ril.iccInfoPrivate.mwis;
  do_test(false, 0);

  run_next_test();
});







add_test(function test_read_new_sms_on_sim() {
  
  
  function newSmsOnSimWorkerHelper() {
    let _postedMessage;
    let _worker = newWorker({
      postRILMessage: function(data) {
      },
      postMessage: function(message) {
        _postedMessage = message;
      }
    });

    _worker.debug = do_print;

    return {
      get postedMessage() {
        return _postedMessage;
      },
      get worker() {
        return _worker;
      },
      fakeWokerBuffer: function() {
        let context = _worker.ContextPool._contexts[0];
        let index = 0; 
        let buf = [];
        context.Buf.writeUint8 = function(value) {
          buf.push(value);
        };
        context.Buf.readUint8 = function() {
          return buf[index++];
        };
        context.Buf.seekIncoming = function(offset) {
          index += offset;
        };
        context.Buf.getReadAvailable = function() {
          return buf.length - index;
        };
      }
    };
  }

  let workerHelper = newSmsOnSimWorkerHelper();
  let worker = workerHelper.worker;
  let context = worker.ContextPool._contexts[0];

  context.ICCIOHelper.loadLinearFixedEF = function fakeLoadLinearFixedEF(options) {
      
      let SimSmsPduHex = "0306911032547698040A9189674523010000208062917314080CC8F71D14969741F977FD07"
                       
                       
                       
                       + "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
                       + "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
                       + "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
                       + "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF";

      workerHelper.fakeWokerBuffer();

      context.Buf.writeString(SimSmsPduHex);

      options.recordSize = 176; 
      if (options.callback) {
        options.callback(options);
      }
  };

  function newSmsOnSimParcel() {
    let data = new Uint8Array(4 + 4); 
    let offset = 0;

    function writeInt(value) {
      data[offset++] = value & 0xFF;
      data[offset++] = (value >>  8) & 0xFF;
      data[offset++] = (value >> 16) & 0xFF;
      data[offset++] = (value >> 24) & 0xFF;
    }

    writeInt(1); 
    writeInt(1); 

    return newIncomingParcel(-1,
                             RESPONSE_TYPE_UNSOLICITED,
                             UNSOLICITED_RESPONSE_NEW_SMS_ON_SIM,
                             data);
  }

  function do_test() {
    worker.onRILMessage(0, newSmsOnSimParcel());

    let postedMessage = workerHelper.postedMessage;

    do_check_eq("sms-received", postedMessage.rilMessageType);
    do_check_eq("+0123456789", postedMessage.SMSC);
    do_check_eq("+9876543210", postedMessage.sender);
    do_check_eq("How are you?", postedMessage.body);
  }

  do_test();

  run_next_test();
});




add_test(function test_update_display_condition() {
  let worker = newUint8Worker();
  let context = worker.ContextPool._contexts[0];
  let record = context.SimRecordHelper;
  let helper = context.GsmPDUHelper;
  let ril    = context.RIL;
  let buf    = context.Buf;
  let io     = context.ICCIOHelper;

  function do_test_spdi() {
    
    
    io.loadTransparentEF = function fakeLoadTransparentEF(options) {
      
      let spdi = [0xA3, 0x0B, 0x80, 0x09, 0x32, 0x64, 0x31, 0x64, 0x26, 0x9F,
                  0xFF, 0xFF, 0xFF];

      
      buf.writeInt32(spdi.length * 2);

      
      for (let i = 0; i < spdi.length; i++) {
        helper.writeHexOctet(spdi[i]);
      }

      
      buf.writeStringDelimiter(spdi.length * 2);

      if (options.callback) {
        options.callback(options);
      }
    };

    record.readSPDI();

    do_check_eq(ril.iccInfo.isDisplayNetworkNameRequired, true);
    do_check_eq(ril.iccInfo.isDisplaySpnRequired, false);
  }

  function do_test_spn(displayCondition,
                       expectedPlmnNameDisplay,
                       expectedSpnDisplay) {
    io.loadTransparentEF = function fakeLoadTransparentEF(options) {
      
      let spn = [0x41, 0x6E, 0x64, 0x72, 0x6F, 0x69, 0x64];
      if (typeof displayCondition === 'number') {
        spn.unshift(displayCondition);
      }

      
      buf.writeInt32(spn.length * 2);

      
      for (let i = 0; i < spn.length; i++) {
        helper.writeHexOctet(spn[i]);
      }

      
      buf.writeStringDelimiter(spn.length * 2);

      if (options.callback) {
        options.callback(options);
      }
    };

    record.readSPN();

    do_check_eq(ril.iccInfo.isDisplayNetworkNameRequired, expectedPlmnNameDisplay);
    do_check_eq(ril.iccInfo.isDisplaySpnRequired, expectedSpnDisplay);
  }

  
  ril.operator = {};
  
  ril.iccInfo.mcc = 310;
  ril.iccInfo.mnc = 260;

  do_test_spdi();

  
  do_test_spn(0x00, true, true);
  do_test_spn(0x01, true, true);
  do_test_spn(0x02, true, false);
  do_test_spn(0x03, true, false);

  
  ril.operator.mcc = 310;
  ril.operator.mnc = 260;
  do_test_spn(0x00, false, true);
  do_test_spn(0x01, true, true);
  do_test_spn(0x02, false, true);
  do_test_spn(0x03, true, true);

  
  ril.iccInfoPrivate.SPDI = [{"mcc":"234","mnc":"136"},{"mcc":"466","mnc":"92"}];
  ril.operator.mcc = 466;
  ril.operator.mnc = 92;
  do_test_spn(0x00, false, true);
  do_test_spn(0x01, true, true);
  do_test_spn(0x02, false, true);
  do_test_spn(0x03, true, true);
  ril.iccInfoPrivate.SPDI = null; 

  
  ril.operator.mcc = 466;
  ril.operator.mnc = 01;
  do_test_spn(0x00, true, true);
  do_test_spn(0x01, true, true);
  do_test_spn(0x02, true, false);
  do_test_spn(0x03, true, false);

  run_next_test();
});




add_test(function test_reading_img_basic() {
  let worker = newUint8Worker();
  let context = worker.ContextPool._contexts[0];
  let record = context.SimRecordHelper;
  let helper = context.GsmPDUHelper;
  let ril    = context.RIL;
  let buf    = context.Buf;
  let io     = context.ICCIOHelper;

  let test_data = [
    {img: [0x01, 0x05, 0x05, 0x11, 0x4f, 0x00, 0x00, 0x00, 0x00, 0x06],
     iidf: [
       [
        0x05, 0x05,
        
        0x11, 0x33, 0x55, 0xfe]],
     expected: [
       {width: 0x05,
        height: 0x05,
        codingScheme: ICC_IMG_CODING_SCHEME_BASIC,
        body: [0x11, 0x33, 0x55, 0xfe]}]},
    {img: [0x01, 0x05, 0x05, 0x11, 0x4f, 0x00, 0x00, 0x00, 0x00, 0x06,
           
           0xff, 0xff],
     iidf: [
       [
        0x05, 0x05,
        
        0x11, 0x33, 0x55, 0xfe]],
     expected: [
       {width: 0x05,
        height: 0x05,
        codingScheme: ICC_IMG_CODING_SCHEME_BASIC,
        body: [0x11, 0x33, 0x55, 0xfe]}]},
    {img: [0x02, 0x10, 0x01, 0x11, 0x4f, 0x04, 0x00, 0x05, 0x00, 0x04, 0x10,
           0x01, 0x11, 0x4f, 0x05, 0x00, 0x05, 0x00, 0x04],
     iidf: [
       [
        0xff, 0xff, 0xff, 0xff, 0xff,
        
        0x10, 0x01,
        
        0x11, 0x99,
        
        0xff, 0xff, 0xff],
       [
        0xff, 0xff, 0xff, 0xff, 0xff,
        
        0x10, 0x01,
        
        0x99, 0x11]],
     expected: [
       {width: 0x10,
        height: 0x01,
        codingScheme: ICC_IMG_CODING_SCHEME_BASIC,
        body: [0x11, 0x99]},
       {width: 0x10,
        height: 0x01,
        codingScheme: ICC_IMG_CODING_SCHEME_BASIC,
        body: [0x99, 0x11]}]},
    {img: [0x01, 0x28, 0x20, 0x11, 0x4f, 0xac, 0x00, 0x0b, 0x00, 0xa2],
     iidf: [
       [
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        
        0x28, 0x20,
        
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
        0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
        0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20,
        0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b,
        0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36,
        0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41,
        0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c,
        0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
        0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x00, 0x01, 0x02,
        0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d,
        0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
        0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e,
        0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
        0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f]],
     expected: [
       {width: 0x28,
        height: 0x20,
        codingScheme: ICC_IMG_CODING_SCHEME_BASIC,
        body: [0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
               0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13,
               0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d,
               0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
               0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31,
               0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b,
               0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45,
               0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
               0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
               0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x00, 0x01, 0x02, 0x03,
               0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d,
               0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
               0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21,
               0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b,
               0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
               0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f]}]}];

  function do_test(img, iidf, expected) {
    io.loadLinearFixedEF = function fakeLoadLinearFixedEF(options) {
      
      buf.writeInt32(img.length * 2);

      
      for (let i = 0; i < img.length; i++) {
        helper.writeHexOctet(img[i]);
      }

      
      buf.writeStringDelimiter(img.length * 2);

      if (options.callback) {
        options.callback(options);
      }
    };

    let instanceIndex = 0;
    io.loadTransparentEF = function fakeLoadTransparentEF(options) {
      
      buf.writeInt32(iidf[instanceIndex].length * 2);

      
      for (let i = 0; i < iidf[instanceIndex].length; i++) {
        helper.writeHexOctet(iidf[instanceIndex][i]);
      }

      
      buf.writeStringDelimiter(iidf[instanceIndex].length * 2);

      instanceIndex++;

      if (options.callback) {
        options.callback(options);
      }
    };

    let onsuccess = function(icons) {
      do_check_eq(icons.length, expected.length);
      for (let i = 0; i < icons.length; i++) {
        let icon = icons[i];
        let exp = expected[i];
        do_check_eq(icon.width, exp.width);
        do_check_eq(icon.height, exp.height);
        do_check_eq(icon.codingScheme, exp.codingScheme);

        do_check_eq(icon.body.length, exp.body.length);
        for (let j = 0; j < icon.body.length; j++) {
          do_check_eq(icon.body[j], exp.body[j]);
        }
      }
    };
    record.readIMG(0, onsuccess);
  }

  for (let i = 0; i< test_data.length; i++) {
    do_test(test_data[i].img, test_data[i].iidf, test_data[i].expected);
  }
  run_next_test();
});




add_test(function test_reading_img_length_error() {
  let worker = newUint8Worker();
  let context = worker.ContextPool._contexts[0];
  let record = context.SimRecordHelper;
  let helper = context.GsmPDUHelper;
  let ril    = context.RIL;
  let buf    = context.Buf;
  let io     = context.ICCIOHelper;
 
  let test_data = [
    {
     img: [0x01, 0x05, 0x05, 0x11, 0x4f, 0x00, 0x00, 0x04, 0x00, 0x06],
     iidf: [0xff, 0xff, 0xff, 
            0x05, 0x05, 0x11, 0x22, 0x33, 0xfe]},
    {
     img: [0x01, 0x05, 0x05, 0x11, 0x4f, 0x00, 0x00, 0x00, 0x00, 0x06],
     iidf: [0x05, 0x05, 0x11, 0x22, 0x33]}];

  function do_test(img, iidf) {
    io.loadLinearFixedEF = function fakeLoadLinearFixedEF(options) {
      
      buf.writeInt32(img.length * 2);

      
      for (let i = 0; i < img.length; i++) {
        helper.writeHexOctet(img[i]);
      }

      
      buf.writeStringDelimiter(img.length * 2);

      if (options.callback) {
        options.callback(options);
      }
    };

    io.loadTransparentEF = function fakeLoadTransparentEF(options) {
      
      buf.writeInt32(iidf.length * 2);

      
      for (let i = 0; i < iidf.length; i++) {
        helper.writeHexOctet(iidf[i]);
      }

      
      buf.writeStringDelimiter(iidf.length * 2);

      if (options.callback) {
        options.callback(options);
      }
    };

    let onsuccess = function() {
      do_print("onsuccess shouldn't be called.");
      do_check_true(false);
    };

    let onerror = function() {
      do_print("onerror called as expected.");
      do_check_true(true);
    };

    record.readIMG(0, onsuccess, onerror);
  }

  for (let i = 0; i < test_data.length; i++) {
    do_test(test_data[i].img, test_data[i].iidf);
  }
  run_next_test();
});




add_test(function test_reading_img_invalid_fileId() {
  let worker = newUint8Worker();
  let context = worker.ContextPool._contexts[0];
  let record = context.SimRecordHelper;
  let helper = context.GsmPDUHelper;
  let ril    = context.RIL;
  let buf    = context.Buf;
  let io     = context.ICCIOHelper;

  
  let img_test = [0x01, 0x05, 0x05, 0x11, 0x5f, 0x00, 0x00, 0x00, 0x00, 0x06];
  let iidf_test = [0x05, 0x05, 0x11, 0x22, 0x33, 0xfe];

  io.loadLinearFixedEF = function fakeLoadLinearFixedEF(options) {
    
    buf.writeInt32(img_test.length * 2);

    
    for (let i = 0; i < img_test.length; i++) {
      helper.writeHexOctet(img_test[i]);
    }

    
    buf.writeStringDelimiter(img_test.length * 2);

    if (options.callback) {
      options.callback(options);
    }
  };

  io.loadTransparentEF = function fakeLoadTransparentEF(options) {
    
    buf.writeInt32(iidf_test.length * 2);

    
    for (let i = 0; i < iidf_test.length; i++) {
      helper.writeHexOctet(iidf_test[i]);
    }

    
    buf.writeStringDelimiter(iidf_test.length * 2);

    if (options.callback) {
      options.callback(options);
    }
  };

  let onsuccess = function() {
    do_print("onsuccess shouldn't be called.");
    do_check_true(false);
  };

  let onerror = function() {
    do_print("onerror called as expected.");
    do_check_true(true);
  };

  record.readIMG(0, onsuccess, onerror);

  run_next_test();
});




add_test(function test_reading_img_wrong_record_length() {
  let worker = newUint8Worker();
  let context = worker.ContextPool._contexts[0];
  let record = context.SimRecordHelper;
  let helper = context.GsmPDUHelper;
  let ril    = context.RIL;
  let buf    = context.Buf;
  let io     = context.ICCIOHelper;

  let test_data = [
    [0x01, 0x05, 0x05, 0x11, 0x4f, 0x00, 0x00, 0x00],
    [0x02, 0x05, 0x05, 0x11, 0x4f, 0x00, 0x00, 0x00, 0x00, 0x06]];

  function do_test(img) {
    io.loadLinearFixedEF = function fakeLoadLinearFixedEF(options) {
      
      buf.writeInt32(img.length * 2);

      
      for (let i = 0; i < img.length; i++) {
        helper.writeHexOctet(img[i]);
      }

      
      buf.writeStringDelimiter(img.length * 2);

      if (options.callback) {
        options.callback(options);
      }
    };

    let onsuccess = function() {
      do_print("onsuccess shouldn't be called.");
      do_check_true(false);
    };

    let onerror = function() {
      do_print("onerror called as expected.");
      do_check_true(true);
    };

    record.readIMG(0, onsuccess, onerror);
  }

  for (let i = 0; i < test_data.length; i++) {
    do_test(test_data[i]);
  }
  run_next_test();
});




add_test(function test_reading_img_color() {
  let worker = newUint8Worker();
  let context = worker.ContextPool._contexts[0];
  let record = context.SimRecordHelper;
  let helper = context.GsmPDUHelper;
  let ril    = context.RIL;
  let buf    = context.Buf;
  let io     = context.ICCIOHelper;

  let test_data = [
    {img: [0x01, 0x05, 0x05, 0x21, 0x4f, 0x11, 0x00, 0x00, 0x00, 0x13],
     iidf: [
       [
        0x05, 0x05, 0x03, 0x08, 0x00, 0x13,
        
        0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90, 0xa0,
        0xb0, 0xc0, 0xd0,
        
        0x00, 0x01, 0x02,
        0x10, 0x11, 0x12,
        0x20, 0x21, 0x22,
        0x30, 0x31, 0x32,
        0x40, 0x41, 0x42,
        0x50, 0x51, 0x52,
        0x60, 0x61, 0x62,
        0x70, 0x71, 0x72]],
     expected: [
       {width: 0x05,
        height: 0x05,
        codingScheme: ICC_IMG_CODING_SCHEME_COLOR,
        bitsPerImgPoint: 0x03,
        numOfClutEntries: 0x08,
        body: [0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90, 0xa0, 0xb0,
               0xc0, 0xd0],
        clut: [0x00, 0x01, 0x02,
               0x10, 0x11, 0x12,
               0x20, 0x21, 0x22,
               0x30, 0x31, 0x32,
               0x40, 0x41, 0x42,
               0x50, 0x51, 0x52,
               0x60, 0x61, 0x62,
               0x70, 0x71, 0x72]}]},
    {img: [0x02, 0x01, 0x06, 0x21, 0x4f, 0x33, 0x00, 0x02, 0x00, 0x08, 0x01,
           0x06, 0x21, 0x4f, 0x44, 0x00, 0x02, 0x00, 0x08],
     iidf: [
       [
        0xff, 0xff,
        
        0x01, 0x06, 0x02, 0x04, 0x00, 0x0d,
        
        0x40, 0x50,
        
        0xaa, 0xbb, 0xcc,
        
        0x01, 0x03, 0x05,
        0x21, 0x23, 0x25,
        0x41, 0x43, 0x45,
        0x61, 0x63, 0x65],
       [
        0xff, 0xff,
        
        0x01, 0x06, 0x02, 0x04, 0x00, 0x0d,
        
        0x4f, 0x5f,
        
        0xaa, 0xbb, 0xcc,
        
        0x11, 0x13, 0x15,
        0x21, 0x23, 0x25,
        0x41, 0x43, 0x45,
        0x61, 0x63, 0x65]],
      expected: [
        {width: 0x01,
         height: 0x06,
         codingScheme: ICC_IMG_CODING_SCHEME_COLOR,
         bitsPerImgPoint: 0x02,
         numOfClutEntries: 0x04,
         body: [0x40, 0x50],
         clut: [0x01, 0x03, 0x05,
                0x21, 0x23, 0x25,
                0x41, 0x43, 0x45,
                0x61, 0x63, 0x65]},
        {width: 0x01,
         height: 0x06,
         codingScheme: ICC_IMG_CODING_SCHEME_COLOR,
         bitsPerImgPoint: 0x02,
         numOfClutEntries: 0x04,
         body: [0x4f, 0x5f],
         clut: [0x11, 0x13, 0x15,
                0x21, 0x23, 0x25,
                0x41, 0x43, 0x45,
                0x61, 0x63, 0x65]}]}];

  function do_test(img, iidf, expected) {
    io.loadLinearFixedEF = function fakeLoadLinearFixedEF(options) {
      
      buf.writeInt32(img.length * 2);

      
      for (let i = 0; i < img.length; i++) {
        helper.writeHexOctet(img[i]);
      }

      
      buf.writeStringDelimiter(img.length * 2);

      if (options.callback) {
        options.callback(options);
      }
    };

    let instanceIndex = 0;
    io.loadTransparentEF = function fakeLoadTransparentEF(options) {
      
      buf.writeInt32(iidf[instanceIndex].length * 2);

      
      for (let i = 0; i < iidf[instanceIndex].length; i++) {
        helper.writeHexOctet(iidf[instanceIndex][i]);
      }

      
      buf.writeStringDelimiter(iidf[instanceIndex].length * 2);

      instanceIndex++;

      if (options.callback) {
        options.callback(options);
      }
    };

    let onsuccess = function(icons) {
      do_check_eq(icons.length, expected.length);
      for (let i = 0; i < icons.length; i++) {
        let icon = icons[i];
        let exp = expected[i];
        do_check_eq(icon.width, exp.width);
        do_check_eq(icon.height, exp.height);
        do_check_eq(icon.codingScheme, exp.codingScheme);

        do_check_eq(icon.body.length, exp.body.length);
        for (let j = 0; j < icon.body.length; j++) {
          do_check_eq(icon.body[j], exp.body[j]);
        }

        do_check_eq(icon.clut.length, exp.clut.length);
        for (let j = 0; j < icon.clut.length; j++) {
          do_check_eq(icon.clut[j], exp.clut[j]);
        }
      }
    };

    record.readIMG(0, onsuccess);
  }

  for (let i = 0; i< test_data.length; i++) {
    do_test(test_data[i].img, test_data[i].iidf, test_data[i].expected);
  }
  run_next_test();
});





add_test(function test_reading_img_color() {
  let worker = newUint8Worker();
  let context = worker.ContextPool._contexts[0];
  let record = context.SimRecordHelper;
  let helper = context.GsmPDUHelper;
  let ril    = context.RIL;
  let buf    = context.Buf;
  let io     = context.ICCIOHelper;

  let test_data = [
    {img: [0x01, 0x05, 0x05, 0x22, 0x4f, 0x11, 0x00, 0x00, 0x00, 0x13],
     iidf: [
       [
        0x05, 0x05, 0x03, 0x08, 0x00, 0x13,
        
        0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90, 0xa0,
        0xb0, 0xc0, 0xd0,
        
        0x00, 0x01, 0x02,
        0x10, 0x11, 0x12,
        0x20, 0x21, 0x22,
        0x30, 0x31, 0x32,
        0x40, 0x41, 0x42,
        0x50, 0x51, 0x52,
        0x60, 0x61, 0x62,
        0x70, 0x71, 0x72]],
     expected: [
       {width: 0x05,
        height: 0x05,
        codingScheme: ICC_IMG_CODING_SCHEME_COLOR_TRANSPARENCY,
        bitsPerImgPoint: 0x03,
        numOfClutEntries: 0x08,
        body: [0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90,
               0xa0, 0xb0, 0xc0, 0xd0],
        clut: [0x00, 0x01, 0x02,
               0x10, 0x11, 0x12,
               0x20, 0x21, 0x22,
               0x30, 0x31, 0x32,
               0x40, 0x41, 0x42,
               0x50, 0x51, 0x52,
               0x60, 0x61, 0x62,
               0x70, 0x71, 0x72]}]},
    {img: [0x02, 0x01, 0x06, 0x22, 0x4f, 0x33, 0x00, 0x02, 0x00, 0x08, 0x01,
           0x06, 0x22, 0x4f, 0x33, 0x00, 0x02, 0x00, 0x08],
     iidf: [
       [
        0xff, 0xff,
        
        0x01, 0x06, 0x02, 0x04, 0x00, 0x0d,
        
        0x40, 0x50,
        
        0x0a, 0x0b, 0x0c,
        
        0x01, 0x03, 0x05,
        0x21, 0x23, 0x25,
        0x41, 0x43, 0x45,
        0x61, 0x63, 0x65],
       [
        0xff, 0xff,
        
        0x01, 0x06, 0x02, 0x04, 0x00, 0x0d,
        
        0x4f, 0x5f,
        
        0x0a, 0x0b, 0x0c,
        
        0x11, 0x13, 0x15,
        0x21, 0x23, 0x25,
        0x41, 0x43, 0x45,
        0x61, 0x63, 0x65]],
     expected: [
       {width: 0x01,
        height: 0x06,
        codingScheme: ICC_IMG_CODING_SCHEME_COLOR_TRANSPARENCY,
        bitsPerImgPoint: 0x02,
        numOfClutEntries: 0x04,
        body: [0x40, 0x50],
        clut: [0x01, 0x03, 0x05,
               0x21, 0x23, 0x25,
               0x41, 0x43, 0x45,
               0x61, 0x63, 0x65]},
       {width: 0x01,
        height: 0x06,
        codingScheme: ICC_IMG_CODING_SCHEME_COLOR_TRANSPARENCY,
        bitsPerImgPoint: 0x02,
        numOfClutEntries: 0x04,
        body: [0x4f, 0x5f],
        clut: [0x11, 0x13, 0x15,
               0x21, 0x23, 0x25,
               0x41, 0x43, 0x45,
               0x61, 0x63, 0x65]}]}];

  function do_test(img, iidf, expected) {
    io.loadLinearFixedEF = function fakeLoadLinearFixedEF(options) {
      
      buf.writeInt32(img.length * 2);

      
      for (let i = 0; i < img.length; i++) {
        helper.writeHexOctet(img[i]);
      }

      
      buf.writeStringDelimiter(img.length * 2);

      if (options.callback) {
        options.callback(options);
      }
    };

    let instanceIndex = 0;
    io.loadTransparentEF = function fakeLoadTransparentEF(options) {
      
      buf.writeInt32(iidf[instanceIndex].length * 2);

      
      for (let i = 0; i < iidf[instanceIndex].length; i++) {
        helper.writeHexOctet(iidf[instanceIndex][i]);
      }

      
      buf.writeStringDelimiter(iidf[instanceIndex].length * 2);

      instanceIndex++;

      if (options.callback) {
        options.callback(options);
      }
    };

    let onsuccess = function(icons) {
      do_check_eq(icons.length, expected.length);
      for (let i = 0; i < icons.length; i++) {
        let icon = icons[i];
        let exp = expected[i];
        do_check_eq(icon.width, exp.width);
        do_check_eq(icon.height, exp.height);
        do_check_eq(icon.codingScheme, exp.codingScheme);

        do_check_eq(icon.body.length, exp.body.length);
        for (let j = 0; j < icon.body.length; j++) {
          do_check_eq(icon.body[j], exp.body[j]);
        }

        do_check_eq(icon.clut.length, exp.clut.length);
        for (let j = 0; j < icon.clut.length; j++) {
          do_check_eq(icon.clut[j], exp.clut[j]);
        }
      }
    };

    record.readIMG(0, onsuccess);
  }

  for (let i = 0; i< test_data.length; i++) {
    do_test(test_data[i].img, test_data[i].iidf, test_data[i].expected);
  }
  run_next_test();
});
