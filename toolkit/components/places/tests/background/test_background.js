






































Components.utils.import("resource://gre/modules/PlacesBackground.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

function test_service_exists()
{
  do_check_neq(PlacesBackground, null);
}

function test_isOnCurrentThread()
{
  do_check_false(PlacesBackground.isOnCurrentThread());

  let event = {
    run: function()
    {
      do_check_true(PlacesBackground.isOnCurrentThread());
    }
  };
  PlacesBackground.dispatch(event, Ci.nsIEventTarget.DISPATCH_SYNC);
}

function test_two_events_same_thread()
{
  
  
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
}

let tests = [
  test_service_exists,
  test_isOnCurrentThread,
  test_two_events_same_thread,
];

function run_test()
{
  for (let i = 0; i < tests.length; i++)
    tests[i]();

  
  let os = Cc["@mozilla.org/observer-service;1"].
           getService(Ci.nsIObserverService);
  os.notifyObservers(null, "quit-application", null);
}
