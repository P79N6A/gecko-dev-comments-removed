


function run_test() {
  run_next_test();
}









function add_test_incoming_parcel(parcel, handler) {
  add_test(function test_incoming_parcel() {
    let worker = newWorker({
      postRILMessage: function(data) {
        
      },
      postMessage: function(message) {
        
      }
    });

    if (!parcel) {
      parcel = newIncomingParcel(-1,
                                 worker.RESPONSE_TYPE_UNSOLICITED,
                                 worker.REQUEST_VOICE_REGISTRATION_STATE,
                                 [0, 0, 0, 0]);
    }

    let context = worker.ContextPool._contexts[0];
    
    let buf = context.Buf;
    let request = parcel[buf.PARCEL_SIZE_SIZE + buf.UINT32_SIZE];
    context.RIL[request] = function ril_request_handler() {
      handler.apply(this, arguments);
    };

    worker.onRILMessage(0, parcel);

    
    run_next_test();
  });
}


add_test_incoming_parcel(null,
  function test_normal_parcel_handling() {
    let self = this;
    try {
      
      self.context.Buf.readInt32();
    } catch (e) {
      ok(false, "Got exception: " + e);
    }
  }
);


add_test_incoming_parcel(null,
  function test_parcel_under_read() {
    let self = this;
    try {
      
      self.context.Buf.readUint16();
    } catch (e) {
      ok(false, "Got exception: " + e);
    }
  }
);


add_test_incoming_parcel(null,
  function test_parcel_over_read() {
    let buf = this.context.Buf;

    
    while (buf.readAvailable > 0) {
      buf.readUint8();
    }

    throws(function over_read_handler() {
      
      buf.readUint8();
    },"Trying to read data beyond the parcel end!");
  }
);


add_test(function test_incoming_parcel_buffer_overwritten() {
  let worker = newWorker({
    postRILMessage: function(data) {
      
    },
    postMessage: function(message) {
      
    }
  });

  let context = worker.ContextPool._contexts[0];
  
  let buf = context.Buf;

  
  function calloc(length, value) {
    let array = new Array(length);
    for (let i = 0; i < length; i++) {
      array[i] = value;
    }
    return array;
  }

  
  let request = worker.REQUEST_VOICE_REGISTRATION_STATE;
  context.RIL[request] = null;

  
  
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
  worker.onRILMessage(0, p1);
  
  equal(buf.readAvailable, 0);
  
  
  equal(buf.currentParcelSize, pA_parcelSize);
  
  equal(buf.readIncoming, p1.length - buf.PARCEL_SIZE_SIZE);
  
  equal(buf.incomingWriteIndex, p1.length);

  
  
  let p2 = new Uint8Array(1 + pB.length);
  p2.set(pA.subarray(pA.length - 1), 0);
  p2.set(pB, 1);
  worker.onRILMessage(0, p2);
  
  equal(buf.readAvailable, 0);
  
  equal(buf.currentParcelSize, 0);
  
  equal(buf.readIncoming, 0);
  
  equal(buf.incomingWriteIndex, pA.length + pB.length);

  
  run_next_test();
});


add_test_incoming_parcel(null,
  function test_buf_readUint8Array() {
    let buf = this.context.Buf;

    let u8array = buf.readUint8Array(1);
    equal(u8array instanceof Uint8Array, true);
    equal(u8array.length, 1);
    equal(buf.readAvailable, 3);

    u8array = buf.readUint8Array(2);
    equal(u8array.length, 2);
    equal(buf.readAvailable, 1);

    throws(function over_read_handler() {
      
      u8array = buf.readUint8Array(2);
    }, "Trying to read data beyond the parcel end!");
  }
);
