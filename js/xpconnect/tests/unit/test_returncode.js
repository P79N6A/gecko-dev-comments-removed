



const {interfaces: Ci, classes: Cc, utils: Cu, manager: Cm, results: Cr} = Components;

Cu.import("resource:///modules/XPCOMUtils.jsm");

function getConsoleMessages() {
  let consoleService = Cc["@mozilla.org/consoleservice;1"].getService(Ci.nsIConsoleService);
  let messages = [m.toString() for (m of consoleService.getMessageArray())];
  
  consoleService.reset();
  return messages;
}

function run_test() {
  
  Cm.autoRegister(do_get_file('../components/native/xpctest.manifest'));
  Cm.autoRegister(do_get_file('../components/js/xpctest.manifest'));

  
  test_simple();
  test_nested();
}

function test_simple() {
  let parent = Cc["@mozilla.org/js/xpc/test/native/ReturnCodeParent;1"]
               .createInstance(Ci.nsIXPCTestReturnCodeParent);
  let result;

  
  getConsoleMessages();

  
  result = parent.callChild(Ci.nsIXPCTestReturnCodeChild.CHILD_SHOULD_THROW);
  Assert.equal(result, Cr.NS_ERROR_XPC_JAVASCRIPT_ERROR_WITH_DETAILS,
               "exception caused NS_ERROR_XPC_JAVASCRIPT_ERROR_WITH_DETAILS");

  let messages = getConsoleMessages();
  Assert.equal(messages.length, 1, "got a console message from the exception");
  Assert.ok(messages[0].indexOf("a requested error") != -1, "got the message text");

  
  result = parent.callChild(Ci.nsIXPCTestReturnCodeChild.CHILD_SHOULD_RETURN_SUCCESS);
  Assert.equal(result, Cr.NS_OK, "success is success");

  Assert.deepEqual(getConsoleMessages(), [], "no messages reported on success.");

  
  
  result = parent.callChild(Ci.nsIXPCTestReturnCodeChild.CHILD_SHOULD_RETURN_RESULTCODE);
  Assert.equal(result, Cr.NS_ERROR_FAILURE,
               "NS_ERROR_FAILURE was seen as the error code.");

  Assert.deepEqual(getConsoleMessages(), [], "no messages reported with .returnCode");
}

function test_nested() {
  let parent = Cc["@mozilla.org/js/xpc/test/native/ReturnCodeParent;1"]
               .createInstance(Ci.nsIXPCTestReturnCodeParent);
  let result;

  
  getConsoleMessages();

  
  
  
  
  result = parent.callChild(Ci.nsIXPCTestReturnCodeChild.CHILD_SHOULD_NEST_RESULTCODES);
  Assert.equal(result, Cr.NS_ERROR_UNEXPECTED,
               "NS_ERROR_UNEXPECTED was seen as the error code.");
  
  
  let expected = ["nested child returned " + Cr.NS_ERROR_FAILURE];
  Assert.deepEqual(getConsoleMessages(), expected, "got the correct sub-error");
}
