
var jslib = Cc["@mozilla.org/url-classifier/jslib;1"].
            getService().wrappedJSObject;
var _Datenow = jslib.Date.now;
function setNow(time) {
  jslib.Date.now = function() {
    return time;
  }
}

function run_test() {
  
  
  var rb = new jslib.RequestBackoff(2, 5, 3, 10, 5, 20);
  setNow(1);
  rb.noteServerResponse(200);
  do_check_true(rb.canMakeRequest());

  setNow(2);
  rb.noteServerResponse(500);
  do_check_true(rb.canMakeRequest());

  setNow(3);
  rb.noteServerResponse(200);
  do_check_true(rb.canMakeRequest());

  
  setNow(4);
  rb.noteServerResponse(502);
  do_check_false(rb.canMakeRequest());
  do_check_eq(rb.nextRequestTime_, 9);

  
  setNow(10);
  do_check_true(rb.canMakeRequest());
  rb.noteServerResponse(503);
  do_check_false(rb.canMakeRequest());
  do_check_eq(rb.nextRequestTime_, 25);

  
  setNow(30);
  do_check_true(rb.canMakeRequest());
  rb.noteServerResponse(302);
  do_check_false(rb.canMakeRequest());
  do_check_eq(rb.nextRequestTime_, 50);

  
  setNow(100);
  do_check_true(rb.canMakeRequest());
  rb.noteServerResponse(200);
  do_check_true(rb.canMakeRequest());
  do_check_eq(rb.nextRequestTime_, 0);

  
  setNow(101);
  rb.noteServerResponse(500);
  do_check_true(rb.canMakeRequest());

  
  setNow(107);
  rb.noteServerResponse(500);
  do_check_true(rb.canMakeRequest());

  setNow(200);
  rb.noteRequest();
  setNow(201);
  rb.noteRequest();
  setNow(202);
  do_check_true(rb.canMakeRequest());
  rb.noteRequest();
  do_check_false(rb.canMakeRequest());
  setNow(211);
  do_check_true(rb.canMakeRequest());

  jslib.Date.now = _Datenow;
}
