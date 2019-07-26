


subscriptLoader.loadSubScript("resource://gre/modules/ril_consts.js", this);

function run_test() {
  run_next_test();
}

add_test(function test_ril_worker_cellbroadcast_activate() {
  let worker = newWorker({
    postRILMessage: function fakePostRILMessage(id, parcel) {
      
    },
    postMessage: function fakePostMessage(message) {
      
    }
  });

  let parcelTypes = [];
  let org_newParcel = worker.Buf.newParcel;
  worker.Buf.newParcel = function(type, options) {
    parcelTypes.push(type);
    org_newParcel.apply(this, arguments);
  };

  function setup(isCdma) {
    worker.RIL._isCdma = isCdma;
    worker.RIL.cellBroadcastDisabled = false;
    worker.RIL.mergedCellBroadcastConfig = [1, 2, 4, 7];  
    parcelTypes = [];
  }

  function test(isCdma, expectedRequest) {
    setup(isCdma);
    worker.RIL.setCellBroadcastDisabled({disabled: true});
    
    do_check_neq(parcelTypes.indexOf(expectedRequest), -1);
    do_check_eq(worker.RIL.cellBroadcastDisabled, true);
  }

  test(false, REQUEST_GSM_SMS_BROADCAST_ACTIVATION);
  test(true, REQUEST_CDMA_SMS_BROADCAST_ACTIVATION);

  run_next_test();
});

add_test(function test_ril_worker_cellbroadcast_config() {
  let currentParcel;
  let worker = newWorker({
    postRILMessage: function fakePostRILMessage(id, parcel) {
      currentParcel = parcel;
    },
    postMessage: function fakePostMessage(message) {
      
    }
  });

  function U32ArrayFromParcelArray(pa) {
    do_print(pa);
    let out = [];
    for (let i = 0; i < pa.length; i += 4) {
      let data = pa[i] + (pa[i+1] << 8) + (pa[i+2] << 16) + (pa[i+3] << 24);
      out.push(data);
    }
    return out;
  }

  function test(isCdma, configs, expected) {
    let parcelType = isCdma ? REQUEST_CDMA_SET_BROADCAST_SMS_CONFIG
                            : REQUEST_GSM_SET_BROADCAST_SMS_CONFIG;

    let found = false;
    worker.postRILMessage = function (id, parcel) {
      u32Parcel = U32ArrayFromParcelArray(Array.slice(parcel));
      if (u32Parcel[1] != parcelType) {
        return;
      }

      found = true;
      
      do_check_eq(u32Parcel.slice(3).toString(), expected);
    };

    worker.RIL._isCdma = isCdma;
    worker.RIL.setSmsBroadcastConfig(configs);

    
    do_check_true(found);
  }

  
  
  test(false,
       [1, 2, 4, 7]  ,
       ["2", "1,2,0,255,1", "4,7,0,255,1"].join());

  
  
  test(true,
       [1, 2, 4, 7]  ,
       ["4", "1,0,1", "4,0,1", "5,0,1", "6,0,1"].join());

  run_next_test();
});

add_test(function test_ril_worker_cellbroadcast_merge_config() {
  let worker = newWorker({
    postRILMessage: function fakePostRILMessage(id, parcel) {
      
    },
    postMessage: function fakePostMessage(message) {
      
    }
  });

  function test(isCdma, configs, expected) {
    worker.RIL._isCdma = isCdma;
    worker.RIL.cellBroadcastConfigs = configs;
    worker.RIL._mergeAllCellBroadcastConfigs();
    do_check_eq(worker.RIL.mergedCellBroadcastConfig.toString(), expected);
  }

  let configs = {
    MMI:    [1, 2, 4, 7],   
    CBMI:   [6, 9],         
    CBMID:  [8, 11],        
    CBMIR:  [10, 13]        
  };

  test(false, configs, "1,2,4,13");
  test(true, configs, "1,2,4,7");

  run_next_test();
});

