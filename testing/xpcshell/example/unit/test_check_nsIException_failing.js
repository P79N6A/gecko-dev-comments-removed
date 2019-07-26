


function run_test() {
  do_check_throws_nsIException(function () {
    throw Error("I find your relaxed dishabille unpalatable");
  }, "NS_NOINTERFACE");
}

