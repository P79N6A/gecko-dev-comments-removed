













































const HTTP_FOUND                 = 302;
const HTTP_SEE_OTHER             = 303;
const HTTP_TEMPORARY_REDIRECT    = 307;









function RequestBackoff(maxErrors, errorPeriod, timeoutIncrement, maxTimeout) {
  this.MAX_ERRORS_ = maxErrors;
  this.ERROR_PERIOD_ = errorPeriod;
  this.TIMEOUT_INCREMENT_ = timeoutIncrement;
  this.MAX_TIMEOUT_ = maxTimeout;

  
  this.errorTimes_ = [];
  this.errorTimeout_ = 0;
  this.nextRequestTime_ = 0;
  this.backoffTriggered_ = false;
}




RequestBackoff.prototype.reset = function() {
  this.errorTimes_ = [];
  this.errorTimeout_ = 0;
  this.nextRequestTime_ = 0;
  this.backoffTriggered_ = false;
}




RequestBackoff.prototype.canMakeRequest = function() {
  return Date.now() > this.nextRequestTime_;
}




RequestBackoff.prototype.noteServerResponse = function(status) {
  if (this.isErrorStatus_(status)) {
    var now = Date.now();
    this.errorTimes_.push(now);

    
    if (this.errorTimes_.length > this.MAX_ERRORS_)
      this.errorTimes_.shift();

    
    
    
    
    if ((this.errorTimes_.length == this.MAX_ERRORS_ &&
         now - this.errorTimes_[0] < this.ERROR_PERIOD_)
        || this.backoffTriggered_) {
      this.errorTimeout_ = (this.errorTimeout_ * 2)  + this.TIMEOUT_INCREMENT_;
      this.errorTimeout_ = Math.min(this.errorTimeout_, this.MAX_TIMEOUT_);
      this.nextRequestTime_ = now + this.errorTimeout_;
      this.backoffTriggered_ = true;
    }
  } else {
    
    
    this.errorTimeout_ = 0;
    this.nextRequestTime_ = 0;
    this.backoffTriggered_ = false;
  }
}






RequestBackoff.prototype.isErrorStatus_ = function(status) {
  return ((500 <= status && status <= 599) ||
          HTTP_FOUND == status ||
          HTTP_SEE_OTHER == status ||
          HTTP_TEMPORARY_REDIRECT == status);
}

#ifdef 0

var jslib = Cc["@mozilla.org/url-classifier/jslib;1"].
            getService().wrappedJSObject;
var _Datenow = jslib.Date.now;
function setNow(time) {
  jslib.Date.now = function() {
    return time;
  }
}


var rb = new jslib.RequestBackoff(2, 5, 5, 20);
setNow(1);
rb.noteServerResponse(200)
if (!rb.canMakeRequest()) throw "expected ok";

setNow(2);
rb.noteServerResponse(500);
if (!rb.canMakeRequest()) throw "expected ok";

setNow(3);
rb.noteServerResponse(200)
if (!rb.canMakeRequest()) throw "expected ok";


setNow(4);
rb.noteServerResponse(502)
if (rb.canMakeRequest()) throw "expected failed";
if (rb.nextRequestTime_ != 9) throw "wrong next request time";


setNow(10);
if (!rb.canMakeRequest()) throw "expected ok";
rb.noteServerResponse(503)
if (rb.canMakeRequest()) throw "expected failed";
if (rb.nextRequestTime_ != 25) throw "wrong next request time";


setNow(30);
if (!rb.canMakeRequest()) throw "expected ok";
rb.noteServerResponse(302)
if (rb.canMakeRequest()) throw "expected failed";
if (rb.nextRequestTime_ != 50) throw "wrong next request time";


setNow(100);
if (!rb.canMakeRequest()) throw "expected ok";
rb.noteServerResponse(200)
if (!rb.canMakeRequest()) throw "expected ok";
if (rb.nextRequestTime_ != 0) throw "wrong next request time";


setNow(101);
rb.noteServerResponse(500);
if (!rb.canMakeRequest()) throw "expected ok";


setNow(107);
rb.noteServerResponse(500);
if (!rb.canMakeRequest()) throw "expected ok";

jslib.Date.now = _Datenow;
#endif
