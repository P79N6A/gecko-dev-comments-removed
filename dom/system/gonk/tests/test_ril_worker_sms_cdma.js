


subscriptLoader.loadSubScript("resource://gre/modules/ril_consts.js", this);

function run_test() {
  run_next_test();
}

function _getWorker() {
  let _postedMessage;
  let _worker = newWorker({
    postRILMessage: function fakePostRILMessage(data) {
    },
    postMessage: function fakePostMessage(message) {
      _postedMessage = message;
    }
  });
  return {
    get postedMessage() {
      return _postedMessage;
    },
    get worker() {
      return _worker;
    }
  };
}




add_test(function test_processCdmaSmsStatusReport() {
  let workerHelper = _getWorker();
  let worker = workerHelper.worker;

  function test_StatusReport(errorClass, msgStatus) {
    let msgId = 0;
    let sentSmsMap = worker.RIL._pendingSentSmsMap;

    sentSmsMap[msgId] = {};

    let message = {
      SMSC:             "",
      mti:              0,
      udhi:             0,
      sender:           "0987654321",
      recipient:        null,
      pid:              PDU_PID_DEFAULT,
      epid:             PDU_PID_DEFAULT,
      dcs:              0,
      mwi:              null,
      replace:          false,
      header:           null,
      body:             "Status: Sent, Dest: 0987654321",
      data:             null,
      timestamp:        new Date().valueOf(),
      language:         null,
      status:           null,
      scts:             null,
      dt:               null,
      encoding:         PDU_CDMA_MSG_CODING_7BITS_ASCII,
      messageClass:     GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_NORMAL],
      messageType:      PDU_CDMA_MSG_TYPE_P2P,
      serviceCategory:  0,
      subMsgType:       PDU_CDMA_MSG_TYPE_DELIVER_ACK,
      msgId:            msgId,
      errorClass:       errorClass,
      msgStatus:        msgStatus
    };

    worker.RIL._processCdmaSmsStatusReport(message);

    let postedMessage = workerHelper.postedMessage;

    
    do_check_true((errorClass === 2)? !!sentSmsMap[msgId]: !sentSmsMap[msgId]);

    
    if (errorClass === -1) {
      
      do_check_eq("sms-received", postedMessage.rilMessageType);
    } else if (errorClass === 2) {
      
    } else {
      
      if (errorClass === 0) {
        do_check_eq(postedMessage.deliveryStatus, GECKO_SMS_DELIVERY_STATUS_SUCCESS);
      } else {
        do_check_eq(postedMessage.deliveryStatus, GECKO_SMS_DELIVERY_STATUS_ERROR);
      }
    }
  }

  test_StatusReport(-1, -1); 
  test_StatusReport(0, 0);   
  test_StatusReport(2, 4);   
  test_StatusReport(3, 5);   

  run_next_test();
});
