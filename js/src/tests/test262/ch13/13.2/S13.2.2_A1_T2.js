









var __MONSTER="monster";
var __PREDATOR="predator";

var __PROTO = function(){};

try{
    __PROTO.type=__MONSTER;
}
catch(e){
    $FAIL('#0: __PROTO.type=__MONSTER does not lead to throwing exception')
}

var __FACTORY = function(){this.name=__PREDATOR};

__FACTORY.prototype=__PROTO;

var __monster = new __FACTORY();



if (!(__PROTO.isPrototypeOf(__monster))) {
	$ERROR('#1: __PROTO.isPrototypeOf(__monster) must be true');
}





if (__monster.type !==__MONSTER) {
	$ERROR('#2: __monster.type ===__MONSTER. Actual: __monster.type ==='+__monster.type);
}



