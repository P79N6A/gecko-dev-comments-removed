




f = eval("\
  (function() {\
    __proto__ = \
    Proxy.createFunction((\
    function() {\
      return {\
        has: ArrayBuffer,\
      }\
    })\
    (\"\"), \
    JSON.parse\
    )\
  })\
")()
