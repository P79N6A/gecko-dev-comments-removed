



var gCount = 0;
const MAX = 10;
var inspector = Cc["@mozilla.org/jsinspector;1"].getService(Ci.nsIJSInspector);
var tm = Cc["@mozilla.org/thread-manager;1"].getService(Ci.nsIThreadManager);


var requestor = (count) => ({
  url:"http://foo/bar/" + count,
  connection: "conn" + (count % 3)
});

function run_test()
{
  test_nesting();
}

function test_nesting()
{
  do_check_eq(inspector.eventLoopNestLevel, 0);

  tm.currentThread.dispatch({ run: enterEventLoop}, 0);

  do_check_eq(inspector.enterNestedEventLoop(requestor(gCount)), 0);
  do_check_eq(inspector.eventLoopNestLevel, 0);
  do_check_eq(inspector.lastNestRequestor, null);
}

function enterEventLoop() {
  if (gCount++ < MAX) {
    tm.currentThread.dispatch({ run: enterEventLoop}, 0);

    let r = Object.create(requestor(gCount));

    do_check_eq(inspector.eventLoopNestLevel, gCount);
    do_check_eq(inspector.lastNestRequestor.url, requestor(gCount - 1).url);
    do_check_eq(inspector.lastNestRequestor.connection, requestor(gCount - 1).connection);
    do_check_eq(inspector.enterNestedEventLoop(requestor(gCount)), gCount);
  } else {
    do_check_eq(gCount, MAX + 1);
    tm.currentThread.dispatch({ run: exitEventLoop}, 0);
  }
}

function exitEventLoop() {
  if (inspector.lastNestRequestor != null) {
    do_check_eq(inspector.lastNestRequestor.url, requestor(gCount - 1).url);
    do_check_eq(inspector.lastNestRequestor.connection, requestor(gCount - 1).connection);
    if (gCount-- > 1) {
      tm.currentThread.dispatch({ run: exitEventLoop}, 0);
    }

    do_check_eq(inspector.exitNestedEventLoop(), gCount);
    do_check_eq(inspector.eventLoopNestLevel, gCount);
  }
}
