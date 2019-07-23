





































gTestfile = '15.6.4.3.js';

















new TestCase( "15.8.6.4",   "new Boolean(1)",       true,   (new Boolean(1)).valueOf() );

new TestCase( "15.8.6.4",   "new Boolean(0)",       false,  (new Boolean(0)).valueOf() );
new TestCase( "15.8.6.4",   "new Boolean(-1)",      true,   (new Boolean(-1)).valueOf() );
new TestCase( "15.8.6.4",   "new Boolean('1')",     true,   (new Boolean("1")).valueOf() );
new TestCase( "15.8.6.4",   "new Boolean('0')",     true,   (new Boolean("0")).valueOf() );
new TestCase( "15.8.6.4",   "new Boolean(true)",    true,   (new Boolean(true)).valueOf() );
new TestCase( "15.8.6.4",   "new Boolean(false)",   false,  (new Boolean(false)).valueOf() );
new TestCase( "15.8.6.4",   "new Boolean('true')",  true,   (new Boolean("true")).valueOf() );
new TestCase( "15.8.6.4",   "new Boolean('false')", true,   (new Boolean('false')).valueOf() );

new TestCase( "15.8.6.4",   "new Boolean('')",      false,  (new Boolean('')).valueOf() );
new TestCase( "15.8.6.4",   "new Boolean(null)",    false,  (new Boolean(null)).valueOf() );
new TestCase( "15.8.6.4",   "new Boolean(void(0))", false,  (new Boolean(void(0))).valueOf() );
new TestCase( "15.8.6.4",   "new Boolean(-Infinity)", true, (new Boolean(Number.NEGATIVE_INFINITY)).valueOf() );
new TestCase( "15.8.6.4",   "new Boolean(NaN)",     false,  (new Boolean(Number.NaN)).valueOf() );
new TestCase( "15.8.6.4",   "new Boolean()",        false,  (new Boolean()).valueOf() );

new TestCase( "15.8.6.4",   "new Boolean(x=1)",     true,   (new Boolean(x=1)).valueOf() );
new TestCase( "15.8.6.4",   "new Boolean(x=0)",     false,  (new Boolean(x=0)).valueOf() );
new TestCase( "15.8.6.4",   "new Boolean(x=false)", false,  (new Boolean(x=false)).valueOf() );
new TestCase( "15.8.6.4",   "new Boolean(x=true)",  true,   (new Boolean(x=true)).valueOf() );
new TestCase( "15.8.6.4",   "new Boolean(x=null)",  false,  (new Boolean(x=null)).valueOf() );
new TestCase( "15.8.6.4",   "new Boolean(x='')",    false,  (new Boolean(x="")).valueOf() );
new TestCase( "15.8.6.4",   "new Boolean(x=' ')",   true,   (new Boolean(x=" ")).valueOf() );

test();
