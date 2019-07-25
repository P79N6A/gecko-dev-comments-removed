





try {
  (function() {
    __defineSetter__("x", Math.sin);
  } ());
  function::x =
    Proxy.createFunction(function() {
        return {
          get: function() {
            return [];
          }
        };
      } (),
      function() {});
} catch(e) {}

reportCompare(0, 0, "ok");
