





var v="global";
function f(a) {
  
  
  
  eval(a);
  let (b=3) {
    
    
    
    
    
    eval("");
    return v;
  };
}



assertEq("global", f(""));




assertEq("local", f("var v='local'"));


function f2(a) {
  eval(a);
  let (b=3) {
    let (c=4) {
      eval("");
      return v;
    };
  };
}

assertEq("global", f2(""));
assertEq("local",  f2("var v='local'"));

reportCompare(true, true);
