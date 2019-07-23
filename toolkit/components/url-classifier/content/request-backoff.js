













































const HTTP_FOUND                 = 302;
const HTTP_SEE_OTHER             = 303;
const HTTP_TEMPORARY_REDIRECT    = 307;












function RequestBackoff(maxErrors, errorPeriod,
                        maxRequests, requestPeriod,
                        timeoutIncrement, maxTimeout) {
  this.MAX_ERRORS_ = maxErrors;
  this.ERROR_PERIOD_ = errorPeriod;
  this.MAX_REQUESTS_ = maxRequests;
  this.REQUEST_PERIOD_ = requestPeriod;
  this.TIMEOUT_INCREMENT_ = timeoutIncrement;
  this.MAX_TIMEOUT_ = maxTimeout;

  
  this.requestTimes_ = [];

  
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
  var now = Date.now();
  if (now <= this.nextRequestTime_) {
    return false;
  }

  return (this.requestTimes_.length < this.MAX_REQUESTS_ ||
          (now - this.requestTimes_[0]) > this.REQUEST_PERIOD_);
}

RequestBackoff.prototype.noteRequest = function() {
  var now = Date.now();
  this.requestTimes_.push(now);

  
  if (this.requestTimes_.length > this.MAX_REQUESTS_)
    this.requestTimes_.shift();
}




RequestBackoff.prototype.noteServerResponse = function(status) {
  if (this.isErrorStatus(status)) {
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






RequestBackoff.prototype.isErrorStatus = function(status) {
  return ((400 <= status && status <= 599) ||
          HTTP_FOUND == status ||
          HTTP_SEE_OTHER == status ||
          HTTP_TEMPORARY_REDIRECT == status);
}

