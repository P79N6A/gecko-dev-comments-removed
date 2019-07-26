


var o0 = Iterator.prototype;
function f0(o) {}
try {
  for(var i=0; i<7; i++) {
    try { o0.prototype(); } catch(e) {
      if (o0.next() != 7)
        throw "7 not yielded";
    };
  }
} catch(exc1) {}
