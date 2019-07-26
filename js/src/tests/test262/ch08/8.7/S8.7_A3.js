












var items = new Array( "one", "two", "three" );


var itemsRef = items;


items = new Array( "new", "array" );



if( items == itemsRef ){
  $ERROR('#1: var items = new Array( "one", "two", "three" ); var itemsRef = items; items = new Array( "new", "array" ); items != itemsRef');
};



