










function test() {
  
  if ( NaN === null ) {
    $ERROR("#1: NaN === null");
  }

  
  if ( Infinity === null ) {
    $ERROR("#2: Infinity === null");
  }

  
  if ( undefined === null ) {
    $ERROR("#3: undefined === null");
  }
}

test();

