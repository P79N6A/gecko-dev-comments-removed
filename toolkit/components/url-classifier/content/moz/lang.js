# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http:
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is Google Safe Browsing.
#
# The Initial Developer of the Original Code is Google Inc.
# Portions created by the Initial Developer are Copyright (C) 2006
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Aaron Boodman <aa@google.com> (original author)
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****

























function bind(fn, self, opt_args) {
  var boundargs = (typeof fn.boundArgs_ != "undefined") ? fn.boundArgs_ : [];
  boundargs = boundargs.concat(Array.prototype.slice.call(arguments, 2));

  if (typeof fn.boundSelf_ != "undefined") {
    self = fn.boundSelf_;
  }

  if (typeof fn.boundFn_ != "undefined") {
    fn = fn.boundFn_;
  }

  var newfn = function() {
    
    var args = boundargs.concat(Array.prototype.slice.call(arguments));
    return fn.apply(self, args);
  }

  newfn.boundArgs_ = boundargs;
  newfn.boundSelf_ = self;
  newfn.boundFn_ = fn;

  return newfn;
}








Function.prototype.bind = function(self, opt_args) {
  return bind.apply(
    null, [this, self].concat(Array.prototype.slice.call(arguments, 1)));
}




























function BindToObject(func, obj, opt_A, opt_B, opt_C, opt_D, opt_E, opt_F) {
  
  var args = Array.prototype.splice.call(arguments, 1, arguments.length);
  return Function.prototype.bind.apply(func, args);
}


























Function.prototype.inherits = function(parentCtor) {
  var tempCtor = function(){};
  tempCtor.prototype = parentCtor.prototype;
  this.superClass_ = parentCtor.prototype;
  this.prototype = new tempCtor();
}
