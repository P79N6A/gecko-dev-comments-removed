





try {
  (function() {
    __defineSetter__("x", Math.sin);
  } ());
} catch(e) {}
function::x =
  Proxy.createFunction(function() {
      return {
        get: function() {
          return [];
        }
      };
    } (),
    function() {});

reportCompare(0, 0, "ok");
