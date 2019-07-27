













this.HTTP_FOUND                 = 302;
this.HTTP_SEE_OTHER             = 303;
this.HTTP_TEMPORARY_REDIRECT    = 307;











this.RequestBackoff =
function RequestBackoff(maxErrors, retryIncrement,
                        maxRequests, requestPeriod,
                        timeoutIncrement, maxTimeout) {
  this.MAX_ERRORS_ = maxErrors;
  this.RETRY_INCREMENT_ = retryIncrement;
  this.MAX_REQUESTS_ = maxRequests;
  this.REQUEST_PERIOD_ = requestPeriod;
  this.TIMEOUT_INCREMENT_ = timeoutIncrement;
  this.MAX_TIMEOUT_ = maxTimeout;

  
  this.requestTimes_ = [];

  this.numErrors_ = 0;
  this.errorTimeout_ = 0;
  this.nextRequestTime_ = 0;
}




RequestBackoff.prototype.reset = function() {
  this.numErrors_ = 0;
  this.errorTimeout_ = 0;
  this.nextRequestTime_ = 0;
}




RequestBackoff.prototype.canMakeRequest = function() {
  var now = Date.now();
  if (now < this.nextRequestTime_) {
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

RequestBackoff.prototype.nextRequestDelay = function() {
  return Math.max(0, this.nextRequestTime_ - Date.now());
}




RequestBackoff.prototype.noteServerResponse = function(status) {
  if (this.isErrorStatus(status)) {
    this.numErrors_++;

    if (this.numErrors_ < this.MAX_ERRORS_)
      this.errorTimeout_ = this.RETRY_INCREMENT_;
    else if (this.numErrors_ == this.MAX_ERRORS_)
      this.errorTimeout_ = this.TIMEOUT_INCREMENT_;
    else
      this.errorTimeout_ *= 2;

    this.errorTimeout_ = Math.min(this.errorTimeout_, this.MAX_TIMEOUT_);
    this.nextRequestTime_ = Date.now() + this.errorTimeout_;
  } else {
    
    this.reset();
  }
}






RequestBackoff.prototype.isErrorStatus = function(status) {
  return ((400 <= status && status <= 599) ||
          HTTP_FOUND == status ||
          HTTP_SEE_OTHER == status ||
          HTTP_TEMPORARY_REDIRECT == status);
}

