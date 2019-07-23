






































Components.utils.import("resource://gre/modules/PlacesBackground.jsm");

function test_service_exists()
{
  print("begin test_service_exists");
  do_check_neq(PlacesBackground, null);
  print("end test_service_exists");
}

function test_isOnCurrentThread()
{
  print("begin test_isOnCurrentThread");
  do_check_false(PlacesBackground.isOnCurrentThread());

  let event = {
    run: function()
    {
      do_check_true(PlacesBackground.isOnCurrentThread());
    }
  };
  PlacesBackground.dispatch(event, Ci.nsIEventTarget.DISPATCH_SYNC);
  print("end test_isOnCurrentThread");
}

function test_two_events_same_thread()
{
   print("begin test_two_events_same_thread");
  
  
  let event = {
    run: function()
    {
      let tm = Cc["@mozilla.org/thread-manager;1"].
               getService(Ci.nsIThreadManager);

      if (!this.thread1)
        this.thread1 = tm.currentThread;
      else
        this.thread2 = tm.currentThread;
    }
  };

  let obj1 = { };
  Components.utils.import("resource://gre/modules/PlacesBackground.jsm", obj1);
  obj1.PlacesBackground.dispatch(event, Ci.nsIEventTarget.DISPATCH_SYNC);
  let obj2 = { };
  Components.utils.import("resource://gre/modules/PlacesBackground.jsm", obj2);
  obj2.PlacesBackground.dispatch(event, Ci.nsIEventTarget.DISPATCH_SYNC);
  do_check_eq(event.thread1, event.thread2);
  print("end test_two_events_same_thread");
}

function test_places_background_shutdown_topic()
{
  print("begin test_places_background_shutdown_topic");
  
  
  let os = Cc["@mozilla.org/observer-service;1"].
           getService(Ci.nsIObserverService);
  os.addObserver({
    observe: function(aSubject, aTopic, aData)
    {
      
      PlacesBackground.dispatch({
        run: function()
        {
          do_test_finished();
        }
      }, Ci.nsIEventTarget.DISPATCH_NORMAL);
    }
  }, "places-background-shutdown", false);
  do_test_pending();
  print("end test_places_background_shutdown_topic");
}

let tests = [
  test_service_exists,
  test_isOnCurrentThread,
  test_two_events_same_thread,
  test_places_background_shutdown_topic,
];

function run_test()
{
  print("begin run_test");
  for (let i = 0; i < tests.length; i++)
    tests[i]();

  print("dispatch shutdown");
  
  let os = Cc["@mozilla.org/observer-service;1"].
           getService(Ci.nsIObserverService);
  os.notifyObservers(null, "quit-application", null);
  print("end run_test");
}
