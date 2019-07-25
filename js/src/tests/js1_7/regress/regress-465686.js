





































var BUGNUMBER = 465686;
var summary = 'Do not crash @ tiny_free_list_add_ptr';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  for each (let b in [eval, eval, 4, 4]) { 
      ++b; 
      for each (b in [(void 0), (void 0), (void 0), 3, (void 0), 3]) { 
          b ^= b; 
          for each (var c in [1/0]) {
          }
      } 
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
