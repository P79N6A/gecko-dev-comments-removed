












var items = new Array( "one", "two", "three" );

var itemsRef = items;

items.push( "four" );var itemsRef = items;


if( itemsRef.length !== 4){
  $ERROR('#1: var items = new Array( "one", "two", "three" ); var itemsRef = items; items.push( "four" );var itemsRef = items; itemsRef.length !== 4');
};






var items = new Array( "one", "two", "three" );

var itemsRef = items;

items[1]="duo";


if( itemsRef[1] !== "duo"){
  $ERROR('#2: var items = new Array( "one", "two", "three" ); var itemsRef = items; items[1]="duo"; itemsRef[1] === "duo". Actual: ' + (itemsRef[1]));
};



