



var callback = arguments[arguments.length - 1];
window.timeout_multiplier = %(timeout_multiplier)d;

window.addEventListener("message", function(event) {
  var tests = event.data[0];
  var status = event.data[1];

  var subtest_results = tests.map(function(x) {
      return [x.name, x.status, x.message, x.stack]
  });

  clearTimeout(timer);
  callback(["%(url)s",
            status.status,
            status.message,
            status.stack,
            subtest_results]);
}, false);

window.win = window.open("%(abs_url)s", "%(window_id)s");

var timer = setTimeout(function() {
  window.win.timeout();
  window.win.close();
}, %(timeout)s);
