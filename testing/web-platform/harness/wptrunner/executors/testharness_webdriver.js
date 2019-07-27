



var callback = arguments[arguments.length - 1];
window.timeout_multiplier = %(timeout_multiplier)d;

window.addEventListener("message", function(event) {
  var tests = event.data[0];
  var status = event.data[1];
  clearTimeout(timer);
  callback({test:"%(url)s",
            tests: tests,
            status: status.status,
            message: status.message,
            stack: status.stack});
}, false);

window.win = window.open("%(abs_url)s", "%(window_id)s");

var timer = setTimeout(function() {
  window.win.timeout();
  window.win.close();
}, %(timeout)s);
