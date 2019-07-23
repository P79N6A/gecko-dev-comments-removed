




































var gTestfile = 'regress-380237-04.js';

var BUGNUMBER = 380237;
var summary = 'Generator expressions parenthesization test';
var actual = '';
var expect = '';

















genexp = "x * x for (x in [])";
genexpParened = "(" + genexp + ")";
genexpParenedTwice = "(" + genexpParened + ")";






doesNotNeedParens("if (xx) { }");
needParens("if (1, xx) { }");
needParens("if (xx, 1) { }");
doesNotNeedParens("do { } while (xx);");
doesNotNeedParens("while (xx) { }");
doesNotNeedParens("switch (xx) { }");
doesNotNeedParens("with (xx) { }");
needParens("switch (x) { case xx: }");
needParens("return xx;");
needParens("yield xx;");
needParens("for (xx;;) { }");
needParens("for (;xx;) { }");
needParens("for (;;xx) { }");
needParens("for (i in xx) { }");
needParens("throw xx");
needParens("try { } catch (e if xx) { }");
needParens("let (x=3) xx");
needParens("let (x=xx) 3");


doesNotNeedParens("f(xx);");
needParens("f(xx, 1);");
needParens("f(1, xx);");
doesNotNeedParens("/x/(xx);");
needParens("/x/(xx, 1);");
needParens("/x/(1, xx);");


doesNotNeedParens("eval(xx);");
needParens("eval(xx, 1);");
needParens("eval(1, xx);");


needParens("xx;");            
needParens("var g = xx;");    
needParens("g += xx;");
needParens("xx();");
needParens("xx() = 3;");
needParens("a ? xx : c");
needParens("xx ? b : c");
needParens("a ? b : xx");
needParens("1 ? xx : c");
needParens("0 ? b : xx");
needParens("1 + xx");
needParens("xx + 1");
needParens("1, xx");
doesNotNeedParens("+(xx)");
doesNotNeedParens("!(xx)");
needParens("xx, 1");
needParens("[1, xx]");
needParens("[xx, 1]");
needParens("[#1=xx,3]");
needParens("[#1=xx,#1#]");
needParens("xx.p");
needParens("xx.@p");
needParens("typeof xx;");
needParens("void xx;");
needParens("({ a: xx })");
needParens("({ a: 1, b: xx })");
needParens("({ a: xx, b: 1 })");
needParens("({ a getter: xx })");
needParens("<x a={xx}/>");
doesNotNeedParens("new (xx);");
doesNotNeedParens("new a(xx);");





rejectLHS("++ (xx);");
rejectLHS("delete xx;");
rejectLHS("delete (xx);");
rejectLHS("for (xx in []) { }");
rejectLHS("for ((xx) in []) { }");
rejectLHS("try { } catch(xx) { }");
rejectLHS("try { } catch([(xx)]) { }");
rejectLHS("xx += 3;");
rejectLHS("(xx) += 3;");
rejectLHS("xx = 3;");


rejectLHS("        (xx) = 3;");
rejectLHS("var     (xx) = 3;");
rejectLHS("const   (xx) = 3;");
rejectLHS("let     (xx) = 3;");


rejectLHS("        [(xx)] = 3;");
rejectLHS("var     [(xx)] = 3;");
rejectLHS("const   [(xx)] = 3;");
rejectLHS("let     [(xx)] = 3;");



rejectLHS("        [(xx)] = [3];");
rejectLHS("var     [(xx)] = [3];");
rejectLHS("const   [(xx)] = [3];");
rejectLHS("let     [(xx)] = [3];");


rejectLHS("        [xx] = [3];");
rejectLHS("var     [xx] = [3];");
rejectLHS("const   [xx] = [3];");
rejectLHS("let     [xx] = 3;");
rejectLHS("        [xx] = 3;");
rejectLHS("var     [xx] = 3;");
rejectLHS("const   [xx] = 3;");
rejectLHS("let     [xx] = 3;");





print("Done!");

function doesNotNeedParens(pat)
{
  print("Testing " + pat);

  var f, ft;
  sanityCheck(pat);

  expect = 'No Error';
  actual = '';
  ft = pat.replace(/xx/, genexp);
    try {
      f = new Function(ft);
      actual = 'No Error';
    } catch(e) {
      print("Unparenthesized genexp SHOULD have been accepted here!");
      actual = e + '';
    }
  reportCompare(expect, actual, summary + ': doesNotNeedParens ' + pat);

  roundTripTest(f);

  
  var uf = "" + f;
  if (pat.indexOf("(xx)") != -1)
    overParenTest(f);
  
  
}

function needParens(pat)
{
  print("Testing " + pat);

  var f, ft;
  sanityCheck(pat);

  expect = 'SyntaxError';
  actual = '';
  ft = pat.replace(/xx/, genexp);
  try {
    f = new Function(ft);
    print("Unparenthesized genexp should NOT have been accepted here!");
  } catch(e) { 
     
    actual = e.name;
  }
  reportCompare(expect, actual, summary + ': needParens ' + pat);

  expect = 'No Error';
  actual = '';
  ft = pat.replace(/xx/, genexpParened);
  try {
    f = new Function(ft);
    actual = 'No Error';
  } catch(e) { 
    print("Yikes!"); 
    actual = e + '';
  }
  reportCompare(expect, actual, summary + ': needParens ' + ft);

  roundTripTest(f);
  overParenTest(f);
}

function rejectLHS(pat)
{
  print("Testing " + pat);

  
    
  var ft;
    
  expect = 'SyntaxError';
  actual = '';
  ft = pat.replace(/xx/, genexp)
    try {
      new Function(ft);
      print("That should have been a syntax error!");
      actual = 'No Error';
    } catch(e) { 
      actual = e.name;
    }
  reportCompare(expect, actual, summary + ': rejectLHS');
}


function overParenTest(f)
{
  var uf = "" + f;

  reportCompare(false, uf.indexOf(genexpParened) == -1, summary + 
                ': overParenTest genexp snugly in parentheses: ' + uf);

  if (uf.indexOf(genexpParened) != -1) {
    reportCompare(true, uf.indexOf(genexpParenedTwice) == -1, summary + 
      ': overParensTest decompilation should not be over-parenthesized: ' + uf);
  }
}

function sanityCheck(pat)
{
  expect = '';
  actual = '';

  if (pat.indexOf("xx") == -1)
  {
    actual += "No 'xx' in this pattern? ";
  }

  var f, ft;
  ft = pat.replace(/xx/, "z");
  try {
    f = new Function(ft);
  } catch(e) { 
    actual += "Yowzers! Probably a bogus test!";
  }
  reportCompare(expect, actual, summary + ': sanityCheck ' + pat);
}

function roundTripTest(f)
{
  
  var uf = "" + f;

  
  expect = 'No Error';
  actual = '';
  var euf;
  try {
    euf = eval("(" + uf + ")");
    actual = 'No Error';
    reportCompare(expect, actual, summary + ': roundTripTest: ' + uf);
  } catch(e) {
    actual = e + '';
    reportCompare(expect, actual, summary + ': roundTripTest: ' + uf);
    return;
  }

  
  expect = uf;
  actual = "" + euf;
  reportCompare(expect, actual, summary + ': roundTripTest no round-trip change');
}
