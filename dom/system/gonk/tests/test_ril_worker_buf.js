


function run_test() {
  run_next_test();
}









function add_test_incoming_parcel(parcel, handler) {
  add_test(function test_incoming_parcel() {
    let worker = newWorker({
      postRILMessage: function fakePostRILMessage(data) {
        
      },
      postMessage: function fakePostMessage(message) {
        
      }
    });

    if (!parcel) {
      parcel = newIncomingParcel(-1,
                                 worker.RESPONSE_TYPE_UNSOLICITED,
                                 worker.REQUEST_REGISTRATION_STATE,
                                 [0, 0, 0, 0]);
    }

    
    let request = parcel[worker.PARCEL_SIZE_SIZE + worker.UINT32_SIZE];
    worker.RIL[request] = function ril_request_handler() {
      handler(worker);
      worker.postMessage();
    };

    worker.onRILMessage(parcel);

    
    run_next_test();
  });
}


add_test_incoming_parcel(null,
  function test_normal_parcel_handling(worker) {
    do_check_throws(function normal_handler(worker) {
      
      worker.Buf.readUint32();
    });
  }
);


add_test_incoming_parcel(null,
  function test_parcel_under_read(worker) {
    do_check_throws(function under_read_handler() {
      
      worker.Buf.readUint16();
    }, false);
  }
);


add_test_incoming_parcel(null,
  function test_parcel_over_read(worker) {
    let buf = worker.Buf;

    
    while (buf.readAvailable > 0) {
      buf.readUint8();
    }

    do_check_throws(function over_read_handler() {
      
      buf.readUint8();
    }, new Error("Trying to read data beyond the parcel end!").result);
  }
);

