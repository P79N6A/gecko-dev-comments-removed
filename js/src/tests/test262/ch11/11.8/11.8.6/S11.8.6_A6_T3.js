









function MyFunct(){return 0};


if (MyFunct instanceof MyFunct){
	$ERROR('#1 function MyFunct(){return 0}; MyFunct instanceof MyFunct === false');
}


if (MyFunct instanceof Function !== true){
	$ERROR('#2 function MyFunct(){return 0}; MyFunct instanceof Function === true');
}


if (MyFunct instanceof Object !== true){
	$ERROR('#3 function MyFunct(){return 0}; MyFunct instanceof Object === true');
}

