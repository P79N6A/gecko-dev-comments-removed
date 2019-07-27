# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:


























this.BindToObject = function BindToObject(fn, self, opt_args) {
  var boundargs = fn.boundArgs_ || [];
  boundargs = boundargs.concat(Array.slice(arguments, 2, arguments.length));

  if (fn.boundSelf_)
    self = fn.boundSelf_;
  if (fn.boundFn_)
    fn = fn.boundFn_;

  var newfn = function() {
    
    var args = boundargs.concat(Array.slice(arguments));
    return fn.apply(self, args);
  }

  newfn.boundArgs_ = boundargs;
  newfn.boundSelf_ = self;
  newfn.boundFn_ = fn;

  return newfn;
}


























Function.prototype.inherits = function(parentCtor) {
  var tempCtor = function(){};
  tempCtor.prototype = parentCtor.prototype;
  this.superClass_ = parentCtor.prototype;
  this.prototype = new tempCtor();
}
