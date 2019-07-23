




(function(e) {
  eval("\
    [(function() {\
      x.k = function(){}\
    })() \
    for (x in [0])]\
  ")
})();
reportCompare(0, 0, "");
