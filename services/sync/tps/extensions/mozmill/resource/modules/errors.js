



var EXPORTED_SYMBOLS = ['BaseError',
                        'ApplicationQuitError',
                        'AssertionError',
                        'TimeoutError'];















function BaseError(aMessage, aFileName, aLineNumber, aFunctionName) {
  this.name = this.constructor.name;

  var err = new Error();
  if (err.stack) {
    this.stack = err.stack;
  }

  this.message = aMessage || err.message;
  this.fileName = aFileName || err.fileName;
  this.lineNumber = aLineNumber || err.lineNumber;
  this.functionName = aFunctionName;
}
















function ApplicationQuitError(aMessage, aFileName, aLineNumber, aFunctionName) {
  BaseError.apply(this, arguments);
}

ApplicationQuitError.prototype = Object.create(BaseError.prototype, {
  constructor : { value : ApplicationQuitError }
});















function AssertionError(aMessage, aFileName, aLineNumber, aFunctionName) {
  BaseError.apply(this, arguments);
}

AssertionError.prototype = Object.create(BaseError.prototype, {
  constructor : { value : AssertionError }
});














function TimeoutError(aMessage, aFileName, aLineNumber, aFunctionName) {
  AssertionError.apply(this, arguments);
}

TimeoutError.prototype = Object.create(AssertionError.prototype, {
  constructor : { value : TimeoutError }
});
