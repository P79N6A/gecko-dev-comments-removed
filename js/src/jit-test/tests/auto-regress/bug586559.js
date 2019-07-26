




x = Proxy.create((function () {
    return {
        get: function () {}
    }
}()), Object.e)
Function("\
  for(var a = 0; a < 2; ++a) {\
    if (a == 0) {}\
    else {\
      x > x\
    }\
  }\
")()
