


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
                                 worker.REQUEST_VOICE_REGISTRATION_STATE,
                                 [0, 0, 0, 0]);
    }

    
    let buf = worker.Buf;
    let request = parcel[buf.PARCEL_SIZE_SIZE + buf.UINT32_SIZE];
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
    do_check_throws(function normal_handler() {
      
      worker.Buf.readInt32();
    });
  }
);


add_test_incoming_parcel(null,
  function test_parcel_under_read(worker) {
    do_check_throws(function under_read_handler() {
      
      worker.Buf.readUint16();
    });
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
    },"Trying to read data beyond the parcel end!");
  }
);


add_test(function test_incoming_parcel_buffer_overwritten() {
  let worker = newWorker({
    postRILMessage: function fakePostRILMessage(data) {
      
    },
    postMessage: function fakePostMessage(message) {
      
    }
  });

  
  let buf = worker.Buf;

  
  function calloc(length, value) {
    let array = new Array(length);
    for (let i = 0; i < length; i++) {
      array[i] = value;
    }
    return array;
  }

  
  let request = worker.REQUEST_VOICE_REGISTRATION_STATE;
  worker.RIL[request] = null;

  
  
  let pA_dataLength = buf.incomingBufferLength / 2;
  let pA = newIncomingParcel(-1,
                             worker.RESPONSE_TYPE_UNSOLICITED,
                             request,
                             calloc(pA_dataLength, 1));
  let pA_parcelSize = pA.length - buf.PARCEL_SIZE_SIZE;

  let pB_dataLength = buf.incomingBufferLength * 3 / 4;
  let pB = newIncomingParcel(-1,
                             worker.RESPONSE_TYPE_UNSOLICITED,
                             request,
                             calloc(pB_dataLength, 1));
  let pB_parcelSize = pB.length - buf.PARCEL_SIZE_SIZE;

  
  let p1 = pA.subarray(0, pA.length - 1);
  worker.onRILMessage(p1);
  
  do_check_eq(buf.readAvailable, 0);
  
  
  do_check_eq(buf.currentParcelSize, pA_parcelSize);
  
  do_check_eq(buf.readIncoming, p1.length - buf.PARCEL_SIZE_SIZE);
  
  do_check_eq(buf.incomingWriteIndex, p1.length);

  
  
  let p2 = new Uint8Array(1 + pB.length);
  p2.set(pA.subarray(pA.length - 1), 0);
  p2.set(pB, 1);
  worker.onRILMessage(p2);
  
  do_check_eq(buf.readAvailable, 0);
  
  do_check_eq(buf.currentParcelSize, 0);
  
  do_check_eq(buf.readIncoming, 0);
  
  do_check_eq(buf.incomingWriteIndex, pA.length + pB.length);

  
  run_next_test();
});


add_test_incoming_parcel(null,
  function test_buf_readUint8Array(worker) {
    let buf = worker.Buf;

    let u8array = buf.readUint8Array(1);
    do_check_eq(u8array instanceof Uint8Array, true);
    do_check_eq(u8array.length, 1);
    do_check_eq(buf.readAvailable, 3);

    u8array = buf.readUint8Array(2);
    do_check_eq(u8array.length, 2);
    do_check_eq(buf.readAvailable, 1);

    do_check_throws(function over_read_handler() {
      
      u8array = buf.readUint8Array(2);
    }, "Trying to read data beyond the parcel end!");
  }
);
