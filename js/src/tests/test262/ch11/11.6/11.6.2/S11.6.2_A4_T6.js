










if (1 - -0 !== 1 ) {  
  $ERROR('#1: 1 - -0 === 1. Actual: ' + (1 - -0));
}


if (1 - 0 !== 1 ) {  
  $ERROR('#2: 1 - 0 === 1. Actual: ' + (1 - 0));
} 


if (-0 - 1 !== -1 ) {  
  $ERROR('#3: -0 - 1 === -1. Actual: ' + (-0 - 1));
}


if (0 - 1 !== -1 ) {  
  $ERROR('#4: 0 - 1 === -1. Actual: ' + (0 - 1));
} 


if (Number.MAX_VALUE - -0 !== Number.MAX_VALUE ) {  
  $ERROR('#5: Number.MAX_VALUE - -0 === Number.MAX_VALUE. Actual: ' + (Number.MAX_VALUE - -0));
}


if (Number.MAX_VALUE - 0 !== Number.MAX_VALUE ) {  
  $ERROR('#6: Number.MAX_VALUE - 0 === Number.MAX_VALUE. Actual: ' + (Number.MAX_VALUE - 0));
} 


if (-0 - Number.MIN_VALUE !== -Number.MIN_VALUE ) {  
  $ERROR('#7: -0 - Number.MIN_VALUE === -Number.MIN_VALUE. Actual: ' + (-0 - Number.MIN_VALUE));
}


if (0 - Number.MIN_VALUE !== -Number.MIN_VALUE ) {  
  $ERROR('#8: 0 - Number.MIN_VALUE === -Number.MIN_VALUE. Actual: ' + (0 - Number.MIN_VALUE));
} 

