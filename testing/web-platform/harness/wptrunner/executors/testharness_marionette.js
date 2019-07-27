



window.wrappedJSObject.timeout_multiplier = %(timeout_multiplier)d;

window.wrappedJSObject.done = function(tests, status) {
  clearTimeout(timer);
  var test_results = tests.map(function(x) {
    return {name:x.name, status:x.status, message:x.message}
  });
  marionetteScriptFinished({test:"%(url)s",
                            tests:test_results,
                            status: status.status,
                            message: status.message});
}

window.wrappedJSObject.win = window.open("%(abs_url)s", "%(window_id)s");

var timer = setTimeout(function() {
  log("Timeout fired");
  window.wrappedJSObject.win.timeout();
}, %(timeout)s);
