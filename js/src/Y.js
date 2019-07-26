






var Y = f => (x => f(v => x(x)(v)))
             (x => f(v => x(x)(v)));


var f = fac => n => (n <= 1) ? 1 : n * fac(n - 1);

print("5! is " + Y(f)(5));
