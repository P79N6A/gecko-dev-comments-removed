









var __map={};



if (!isNaN(__map.foo++)) {
  $ERROR('#1: var __map={}; __map.foo === Not-a-Number. Actual: ' + (__map.foo));
}





if (!("foo" in __map)) {
  $ERROR('#2: var __map={}; "foo" in __map');
}



