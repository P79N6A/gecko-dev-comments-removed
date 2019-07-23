




































var bug = 363040;
var summary = 'Array.prototype.reduce application in continued fraction';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  


  function contfrac(x, epsilon) {
    let i = Math.floor(x);
    let a = [i];
    x = x - i;
    let maxerr = x;
    while (maxerr > epsilon) {
      x = 1 / x;
      i = Math.floor(x);
      a.push(i);
      x = x - i;
      maxerr = x * maxerr / i;
    }
    print(uneval(a));
    a.push([1, 0]);
    a.reverse();
    return a.reduce(function (x, y) {return [x[0] * y + x[1], x[0]];});
  }

  if (!Array.prototype.reduce)
  {
    print('Test skipped. Array.prototype.reduce not implemented');
  }
  else
  {

    for each (num in [Math.PI, Math.sqrt(2), 1 / (Math.sqrt(Math.E) - 1)]) {
      print('Continued fractions for', num);
      for each (eps in [1e-2, 1e-3, 1e-5, 1e-7, 1e-10]) {
        let frac = contfrac(num, eps);
        let est = frac[0] / frac[1];
        let err = num - est;
        print(uneval(frac), est, err);
      }
      print();
    }
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
