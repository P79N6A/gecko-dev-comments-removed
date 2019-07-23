







































var gTestfile = 'regress-24712.js';

test();

function test()
{   
  enterFunc ("test");

  printBugNumber (24712);
   
  var re = /([\S]+([ \t]+[\S]+)*)[ \t]*=[ \t]*[\S]+/;
  var result = re.exec("Course_Creator = Test") + '';

  reportCompare('Course_Creator = Test,Course_Creator,', result, 'exec() returned null');
   
  exitFunc ("test");
   
}

