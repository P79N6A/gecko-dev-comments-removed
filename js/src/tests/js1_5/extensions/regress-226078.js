





var BUGNUMBER = 226078;
var summary = 'Do not Crash @ js_Interpret 3127f864';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);
 

function SetLangHead(l){
  with(p){
    for(var i in x)
      if(getElementById("TxtH"+i)!=undefined)
        printStatus('huh');
  }
}
x=[0,1,2,3];
p={getElementById: function (id){printStatus(uneval(this), id); return undefined;}};
SetLangHead(1);

reportCompare(expect, actual, summary);
