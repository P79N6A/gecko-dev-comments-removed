const Cc = Components.classes;
const Ci = Components.interfaces;

var timer;










var exception = {
  message: "oops, something failed!",

  tries: 0,
  get result() {
    ++this.tries;
    return 3;
  }
};

var callback = {
  tries: 0,
  notify: function (timer) {
    if (++this.tries === 1)
      throw exception;

    try {
        do_check_true(exception.tries >= 1);
    } finally {
        timer.cancel();
        timer = null;
        do_test_finished();
    }
  }
};

function run_test() {
  do_test_pending();

  timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
  timer.initWithCallback(callback, 0, timer.TYPE_REPEATING_SLACK);
}
