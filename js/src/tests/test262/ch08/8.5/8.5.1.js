























 

var value = 1;
var floatValues = new Array(1076);
for(var power = 0; power <= 1075; power++){
	floatValues[power] = value;
    
    
	value = value * 0.5;
}


if(floatValues[1075] !== 0) {
  $ERROR("Value after min denorm should round to 0");
}


if(floatValues[1074] !== 4.9406564584124654417656879286822e-324) {
  $ERROR("Min denorm value is incorrect: " + floatValues[1074]);
}


for(var index = 1074; index > 0; index--){
  if(floatValues[index] === 0){
	$ERROR("2**-" + index + " should not be 0");
  }
  if(floatValues[index - 1] !== (floatValues[index] * 2)){
	$ERROR("Value should be double adjacent value at index " + index);
  }
}


if(!(1.797693134862315708145274237317e+308 < Infinity)){
	$ERROR("Max Number value 1.797693134862315708145274237317e+308 should not overflow to infinity");
}


if(!(1.797693134862315808e+308 === +Infinity)){
	$ERROR("1.797693134862315808e+308 did not resolve to Infinity");
}
