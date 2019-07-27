


subscriptLoader.loadSubScript("resource://gre/modules/ril_consts.js", this);

function run_test() {
  run_next_test();
}




add_test(function test_icc_get_card_lock_enabled() {
  let worker = newUint8Worker();
  let context = worker.ContextPool._contexts[0];
  let buf = context.Buf;
  let ril = context.RIL;
  ril.aid = "123456789";
  ril.v5Legacy = false;

  function do_test(aLock) {
    const serviceClass = ICC_SERVICE_CLASS_VOICE |
                         ICC_SERVICE_CLASS_DATA  |
                         ICC_SERVICE_CLASS_FAX;

    buf.sendParcel = function fakeSendParcel() {
      
      equal(this.readInt32(), REQUEST_QUERY_FACILITY_LOCK)

      
      this.readInt32();

      
      let parcel = this.readStringList();
      equal(parcel.length, ril.v5Legacy ? 3 : 4);
      equal(parcel[0], GECKO_CARDLOCK_TO_FACILITY[aLock]);
      equal(parcel[1], "");
      equal(parcel[2], serviceClass.toString());
      if (!ril.v5Legacy) {
        equal(parcel[3], ril.aid);
      }
    };

    ril.iccGetCardLockEnabled({lockType: aLock});
  }

  do_test(GECKO_CARDLOCK_PIN)
  do_test(GECKO_CARDLOCK_FDN)

  run_next_test();
});

add_test(function test_path_id_for_spid_and_spn() {
  let worker = newWorker({
    postRILMessage: function(data) {
      
    },
    postMessage: function(message) {
      
    }});
  let context = worker.ContextPool._contexts[0];
  let RIL = context.RIL;
  let ICCFileHelper = context.ICCFileHelper;

  
  RIL.appType = CARD_APPTYPE_SIM;
  equal(ICCFileHelper.getEFPath(ICC_EF_SPDI),
              EF_PATH_MF_SIM + EF_PATH_DF_GSM);
  equal(ICCFileHelper.getEFPath(ICC_EF_SPN),
              EF_PATH_MF_SIM + EF_PATH_DF_GSM);

  
  RIL.appType = CARD_APPTYPE_USIM;
  equal(ICCFileHelper.getEFPath(ICC_EF_SPDI),
              EF_PATH_MF_SIM + EF_PATH_ADF_USIM);
  equal(ICCFileHelper.getEFPath(ICC_EF_SPDI),
              EF_PATH_MF_SIM + EF_PATH_ADF_USIM);
  run_next_test();
});




add_test(function test_icc_set_card_lock_enabled() {
  let worker = newUint8Worker();
  let context = worker.ContextPool._contexts[0];
  let buf = context.Buf;
  let ril = context.RIL;
  ril.aid = "123456789";
  ril.v5Legacy = false;

  function do_test(aLock, aPassword, aEnabled) {
    const serviceClass = ICC_SERVICE_CLASS_VOICE |
                         ICC_SERVICE_CLASS_DATA  |
                         ICC_SERVICE_CLASS_FAX;

    buf.sendParcel = function fakeSendParcel() {
      
      equal(this.readInt32(), REQUEST_SET_FACILITY_LOCK);

      
      this.readInt32();

      
      let parcel = this.readStringList();
      equal(parcel.length, ril.v5Legacy ? 4 : 5);
      equal(parcel[0], GECKO_CARDLOCK_TO_FACILITY[aLock]);
      equal(parcel[1], aEnabled ? "1" : "0");
      equal(parcel[2], aPassword);
      equal(parcel[3], serviceClass.toString());
      if (!ril.v5Legacy) {
        equal(parcel[4], ril.aid);
      }
    };

    ril.iccSetCardLockEnabled({
      lockType: aLock,
      enabled: aEnabled,
      password: aPassword});
  }

  do_test(GECKO_CARDLOCK_PIN, "1234", true);
  do_test(GECKO_CARDLOCK_PIN, "1234", false);
  do_test(GECKO_CARDLOCK_FDN, "4321", true);
  do_test(GECKO_CARDLOCK_FDN, "4321", false);

  run_next_test();
});




add_test(function test_icc_change_card_lock_password() {
  let worker = newUint8Worker();
  let context = worker.ContextPool._contexts[0];
  let buf = context.Buf;
  let ril = context.RIL;


  function do_test(aLock, aPassword, aNewPassword) {
    let GECKO_CARDLOCK_TO_REQUEST = {};
    GECKO_CARDLOCK_TO_REQUEST[GECKO_CARDLOCK_PIN] = REQUEST_CHANGE_SIM_PIN;
    GECKO_CARDLOCK_TO_REQUEST[GECKO_CARDLOCK_PIN2] = REQUEST_CHANGE_SIM_PIN2;

    buf.sendParcel = function fakeSendParcel() {
      
      equal(this.readInt32(), GECKO_CARDLOCK_TO_REQUEST[aLock]);

      
      this.readInt32();

      
      let parcel = this.readStringList();
      equal(parcel.length, ril.v5Legacy ? 2 : 3);
      equal(parcel[0], aPassword);
      equal(parcel[1], aNewPassword);
      if (!ril.v5Legacy) {
        equal(parcel[2], ril.aid);
      }
    };

    ril.iccChangeCardLockPassword({
      lockType: aLock,
      password: aPassword,
      newPassword: aNewPassword});
  }

  do_test(GECKO_CARDLOCK_PIN, "1234", "4321");
  do_test(GECKO_CARDLOCK_PIN2, "1234", "4321");

  run_next_test();
});




add_test(function test_icc_unlock_card_lock_pin() {
  let worker = newUint8Worker();
  let context = worker.ContextPool._contexts[0];
  let ril = context.RIL;
  let buf = context.Buf;
  ril.aid = "123456789";
  ril.v5Legacy = false;

  function do_test(aLock, aPassword) {
    let GECKO_CARDLOCK_TO_REQUEST = {};
    GECKO_CARDLOCK_TO_REQUEST[GECKO_CARDLOCK_PIN] = REQUEST_ENTER_SIM_PIN;
    GECKO_CARDLOCK_TO_REQUEST[GECKO_CARDLOCK_PIN2] = REQUEST_ENTER_SIM_PIN2;

    buf.sendParcel = function fakeSendParcel() {
      
      equal(this.readInt32(), GECKO_CARDLOCK_TO_REQUEST[aLock]);

      
      this.readInt32();

      
      let parcel = this.readStringList();
      equal(parcel.length, ril.v5Legacy ? 1 : 2);
      equal(parcel[0], aPassword);
      if (!ril.v5Legacy) {
        equal(parcel[1], ril.aid);
      }
    };

    ril.iccUnlockCardLock({
      lockType: aLock,
      password: aPassword
    });
  }

  do_test(GECKO_CARDLOCK_PIN, "1234");
  do_test(GECKO_CARDLOCK_PIN2, "1234");

  run_next_test();
});




add_test(function test_icc_unlock_card_lock_puk() {
  let worker = newUint8Worker();
  let context = worker.ContextPool._contexts[0];
  let ril = context.RIL;
  let buf = context.Buf;
  ril.aid = "123456789";
  ril.v5Legacy = false;

  function do_test(aLock, aPassword, aNewPin) {
    let GECKO_CARDLOCK_TO_REQUEST = {};
    GECKO_CARDLOCK_TO_REQUEST[GECKO_CARDLOCK_PUK] = REQUEST_ENTER_SIM_PUK;
    GECKO_CARDLOCK_TO_REQUEST[GECKO_CARDLOCK_PUK2] = REQUEST_ENTER_SIM_PUK2;

    buf.sendParcel = function fakeSendParcel() {
      
      equal(this.readInt32(), GECKO_CARDLOCK_TO_REQUEST[aLock]);

      
      this.readInt32();

      
      let parcel = this.readStringList();
      equal(parcel.length, ril.v5Legacy ? 2 : 3);
      equal(parcel[0], aPassword);
      equal(parcel[1], aNewPin);
      if (!ril.v5Legacy) {
        equal(parcel[2], ril.aid);
      }
    };

    ril.iccUnlockCardLock({
      lockType: aLock,
      password: aPassword,
      newPin: aNewPin
    });
  }

  do_test(GECKO_CARDLOCK_PUK, "12345678", "1234");
  do_test(GECKO_CARDLOCK_PUK2, "12345678", "1234");

  run_next_test();
});




add_test(function test_icc_unlock_card_lock_depersonalization() {
  let worker = newUint8Worker();
  let context = worker.ContextPool._contexts[0];
  let ril = context.RIL;
  let buf = context.Buf;

  function do_test(aPassword) {
    buf.sendParcel = function fakeSendParcel() {
      
      equal(this.readInt32(), REQUEST_ENTER_NETWORK_DEPERSONALIZATION_CODE);

      
      this.readInt32();

      
      let parcel = this.readStringList();
      equal(parcel.length, 1);
      equal(parcel[0], aPassword);
    };

    ril.iccUnlockCardLock({
      lockType: GECKO_CARDLOCK_NCK,
      password: aPassword
    });
  }

  do_test("12345678");

  run_next_test();
});
