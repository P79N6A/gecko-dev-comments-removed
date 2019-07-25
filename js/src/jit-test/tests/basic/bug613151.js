
function n() {}
function g() {}
eval("\
  function a() {}\
  function b() {\
    for (w in this) {}\
    Object.defineProperty(\
      this, \
      new QName, \
      ({enumerable: true})\
    )\
  }\
  for (z in [0, 0, 0]) b()\
")



