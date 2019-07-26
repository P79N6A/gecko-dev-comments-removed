









function __func(arg1, arg2){
    arg1++;
    arg2+="BA";
};

var x=1;
y=2;
var a="AB"
b="SAM";

__func(x,a);
__func(y,b);



if (x!==1 || y!==2 || a!=="AB" || b!=="SAM") {
	$ERROR('#1: x === 1 and y === 2 and a === "AB" and b === "SAM". Actual: x ==='+x+' and y ==='+y+' and a ==='+a+' and b ==='+b);
}



