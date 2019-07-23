




































var bug = 351070;
var summary = 'decompilation of let declaration should not change scope';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  var pfx  = "(function f() { var n = 2, a = 2; ",
    decl = " let a = 3;",
    end  = " return a; })";

  var table = [
    ["if (!!true)",       ""],
    ["if (!!true)",       " else foopy();"],
    ["if (!true); else",  ""],
    ["do ",               " while (false);"],
    ["while (--n)",       ""],
    ["for (--n;n;--n)",   ""],
    ["for (a in this)",   ""],
    ["with (this)",       ""],
    ];

  expect = 3;

  for (i = 0; i < table.length; i++) {
    var src = pfx + table[i][0] + decl + table[i][1] + end;
    print('src: ' + src);
    var fun = eval(src);
    var testval = fun();
    reportCompare(expect, testval, summary + ': ' + src);
    if (testval != expect) {
      break;
    }
    print('uneval: ' + uneval(fun));
    var declsrc = '(' + 
      src.slice(1, -1).replace('function f', 'function f' + i) + ')';
    print('declsrc: ' + declsrc);
    this['f' + i] = eval(declsrc);
    print('f' + i + ': ' + this['f' + i]);
  }

  exitFunc ('test');
}
