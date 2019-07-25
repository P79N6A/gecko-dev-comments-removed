



eval("\
  let(b)((\
    function(){\
      let(d=b)\
      ((function(){\
        b=b\
      })())\
    }\
  )())\
")
reportCompare(0, 0, "ok");
