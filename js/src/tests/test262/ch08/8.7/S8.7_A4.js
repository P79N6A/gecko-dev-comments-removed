












var item = new String("test");

var itemRef = item;



item += "ing";


if( item == itemRef ){
  $ERROR('#1: var item = new String("test"); var itemRef = item; item += "ing"; item != itemRef');
};



