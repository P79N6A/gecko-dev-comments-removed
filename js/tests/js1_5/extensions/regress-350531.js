




































var gTestfile = 'regress-350531.js';

var BUGNUMBER = 350531;
var summary = 'exhaustively test parenthesization of binary operator subsets';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 












  String.prototype.push = function (str) { return this + str; };

  function permute(list) {
    if (!list.length)                                   
      return [list];                                    
    var res = [];
    for (var i = 0, n = list.length; i < n; i++) {      
      var rest = list.slice(0, i).concat(list.slice(i+1));
      for each (var x in permute(rest))                 
        res.push(list.slice(i, i+1).concat(x));         
    }
    return res;
  }

  function subset(list, size) {
    if (size == 0 || !list.length)                      
      return [list.slice(0, 0)];                        
    var result = [];
    for (var i = 0, n = list.length; i < n; i++) {
      var pick = list.slice(i, i+1);                    
      var rest = list.slice(0, i).concat(list.slice(i+1)); 
      for each (var x in subset(rest, size-1))
	result.push(pick.concat(x));
    }
    return result;
  }

  function combo(list, size) {
    if (size == 0 || !list.length)                      
      return [list.slice(0, 0)];                        
    var result = [];
    for (var i = 0, n = (list.length - size) + 1; i < n; i++) {
      
      var pick = list.slice(i, i+1);
      var rest = list.slice(i+1);                       
      for each (var x in combo(rest, size - 1))
	result.push(pick.concat(x));
    }
    return result;
  }








  var bops = [
    ["=", "|=", "^=", "&=", "<<=", ">>=", ">>>=", "+=", "-=", "*=", "/=", "%="],
    ["||"],
    ["&&"],
    ["|"],
    ["^"],
    ["&"],
    ["==", "!=", "===", "!=="],
    ["<", "<=", ">=", ">", "in", "instanceof"],
    ["<<", ">>", ">>>"],
    ["+", "-"],
    ["*", "/", "%"],
    ];

  var prec = {};
  var aops = [];

  for (var i = 0; i < bops.length; i++) {
    for (var j = 0; j < bops[i].length; j++) {
      var k = bops[i][j];
      prec[k] = i;
      aops.push(k);
    }
  }




next_subset:
  for (i = 2; i < 5; i++) {
    var sets = subset(aops, i);
    gc();

    for each (var set in sets) {
      
      var src = "(function () {";
      for (j in set) {
        var op = set[j], op2 = set[j-1];

        
        
        
        if (prec[op] && prec[op] < prec[op2])
          src += "(";
      }
      src += "x ";
      for (j in set) {
        var op = set[j], op2 = set[j+1];

        
        
        var term = (prec[op] && prec[op] < prec[op2]) ? " x)" : " x";

        src += op + term;
        if (j < set.length - 1)
          src += " ";
      }
      src += ";})";
      try {
        var ref = uneval(eval(src)).replace(/\s+/g, ' ');
        if (ref != src) {
          actual += "BROKEN! input: " + src + " output: " + ref + " ";
          print("BROKEN! input: " + src + " output: " + ref);
          break next_subset;
        }
      } catch (e) {}
    }
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
