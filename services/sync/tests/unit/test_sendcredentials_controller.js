


Cu.import("resource://services-sync/constants.js");
Cu.import("resource://services-sync/jpakeclient.js");
Cu.import("resource://services-sync/service.js");
Cu.import("resource://services-sync/util.js");

function run_test() {
  setBasicCredentials("johndoe", "ilovejane", Utils.generatePassphrase());
  Service.serverURL  = "http://weave.server/";

  initTestLogging("Trace");
  Log4Moz.repository.getLogger("Sync.SendCredentialsController").level = Log4Moz.Level.Trace;
  Log4Moz.repository.getLogger("Sync.SyncScheduler").level = Log4Moz.Level.Trace;
  run_next_test();
}

function make_sendCredentials_test(topic) {
  return function test_sendCredentials() {
    _("Test sending credentials on " + topic + " observer notification.");

    let sendAndCompleteCalled = false;
    let jpakeclient = {
      sendAndComplete: function sendAndComplete(data) {
        
        
        do_check_false(sendAndCompleteCalled);
        sendAndCompleteCalled = true;

        
        do_check_eq(data.account,   Identity.account);
        do_check_eq(data.password,  Identity.basicPassword);
        do_check_eq(data.synckey,   Identity.syncKey);
        do_check_eq(data.serverURL, Service.serverURL);

        this.controller.onComplete();
        
        let expectedInterval = Service.scheduler.activeInterval;
        do_check_true(Service.scheduler.nextSync - Date.now() <= expectedInterval);

        
        
        Svc.Obs.notify(topic);

        Service.scheduler.setDefaults();
        Utils.nextTick(run_next_test);
      }
    };
    jpakeclient.controller = new SendCredentialsController(jpakeclient, Service);
    Svc.Obs.notify(topic);
  };
}

add_test(make_sendCredentials_test("weave:service:sync:finish"));
add_test(make_sendCredentials_test("weave:service:sync:error"));


add_test(function test_abort() {
  _("Test aborting the J-PAKE exchange.");

  let jpakeclient = {
    sendAndComplete: function sendAndComplete() {
      do_throw("Shouldn't get here!");
    }
  };
  jpakeclient.controller = new SendCredentialsController(jpakeclient, Service);

  
  
  jpakeclient.controller.onAbort(JPAKE_ERROR_USERABORT);
  Svc.Obs.notify("weave:service:sync:finish");
  Utils.nextTick(run_next_test);
});


add_test(function test_startOver() {
  _("Test wiping local Sync config aborts transaction.");

  let abortCalled = false;
  let jpakeclient = {
    abort: function abort() {
      abortCalled = true;
      this.controller.onAbort(JPAKE_ERROR_USERABORT);
    },
    sendAndComplete: function sendAndComplete() {
      do_throw("Shouldn't get here!");
    }
  };
  jpakeclient.controller = new SendCredentialsController(jpakeclient, Service);

  Svc.Obs.notify("weave:service:start-over");
  do_check_true(abortCalled);

  
  
  Svc.Obs.notify("weave:service:sync:error");

  Utils.nextTick(run_next_test);
});
