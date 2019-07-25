


"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;


let subscriptLoader = Cc["@mozilla.org/moz/jssubscript-loader;1"]
                        .getService(Ci.mozIJSSubScriptLoader);












function do_check_throws(func, result, stack)
{
  if (!stack)
    stack = Components.stack.caller;

  try {
    func();
  } catch (ex) {
    if (ex.name == result) {
      return;
    }
    do_throw("expected result " + result + ", caught " + ex, stack);
  }

  if (result) {
    do_throw("expected result " + result + ", none thrown", stack);
  }
}












function wsp_test_func(func, data, expect) {
  let result_str = JSON.stringify(func(data));
  let expect_str = JSON.stringify(expect);
  if (result_str !== expect_str) {
    do_throw("expect decoded value: '" + expect_str + "', got '" + result_str + "'");
  }
}













function wsp_decode_test_ex(func, input, expect, exception) {
  let data = {array: input, offset: 0};
  do_check_throws(wsp_test_func.bind(null, func, data, expect), exception);
}













function wsp_decode_test(target, input, expect, exception) {
  let func = function decode_func(data) {
    return target.decode(data);
  };

  wsp_decode_test_ex(func, input, expect, exception);
}









function strToCharCodeArray(str, noAppendNull) {
  let result = [];

  for (let i = 0; i < str.length; i++) {
    result.push(str.charCodeAt(i));
  }
  if (!noAppendNull) {
    result.push(0);
  }

  return result;
}

