


function f(param) {
  var w;
  return eval("\
    (function(){\
      __defineGetter__(\"y\", function()({\
        x: function(){ return w }()\
      }))\
    });\
  ");
}
(f())();
(new Function("eval(\"y\")"))();
