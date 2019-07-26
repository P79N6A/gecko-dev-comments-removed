










var __PLANT="flower";
var __ROSE="rose";

function __PROTO(){};

try{
    __PROTO.type=__PLANT;
}
catch(e){
    $ERROR('#0: __PROTO.type=__PLANT does not lead to throwing exception')
}

function __FACTORY(){this.name=__ROSE};

__FACTORY.prototype=__PROTO;

var __rose = new __FACTORY();



try{
    __rose();
    $ERROR('#1: __rose() lead to throwing exception');
} catch(e){
    if (!(e instanceof TypeError)) {
    	$ERROR('#2: Exception Type is TypeError. Actual: exception ==='+e);
    }
}



