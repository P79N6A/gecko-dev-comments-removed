
(function() {
  var first, foo, second, third, _ref;

  foo = function(n) {
    var i, _i;
    for (i = _i = 0; 0 <= n ? _i < n : _i > n; i = 0 <= n ? ++_i : --_i) {
      return "foo" + i;
    }
  };

  _ref = foo(3), first = _ref[0], second = _ref[1], third = _ref[2];

  debugger;

}).call(this);
