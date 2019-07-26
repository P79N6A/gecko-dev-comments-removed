


"use strict";

(function initFxAccountsTestingInfrastructure() {
  do_get_profile();

  let ns = {};
  Components.utils.import("resource://testing-common/services-common/logging.js",
                          ns);

  ns.initTestLogging("Trace");
}).call(this);













function do_check_throws(func, message, stack)
{
  if (!stack)
    stack = Components.stack.caller;

  try {
    func();
  } catch (exc) {
    if (exc.message === message) {
      return;
    }
    do_throw("expecting exception '" + message
             + "', caught '" + exc.message + "'", stack);
  }

  if (message) {
    do_throw("expecting exception '" + message + "', none thrown", stack);
  }
}
