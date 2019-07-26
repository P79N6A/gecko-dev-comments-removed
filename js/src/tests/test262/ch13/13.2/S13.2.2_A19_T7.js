









var a = 1;

var __obj = {a:2};

with (__obj)
{
    var __func = function()
    {
        return a;
    }
}



if (__obj.hasOwnProperty('__func')) {
	$ERROR('#1: __obj.hasOwnProperty(\'__func\') === false');
}





if (!(this.hasOwnProperty('__func'))) {
	$ERROR('#2: this.hasOwnProperty(\'__func\') === true');
}





if (__func in __obj) {
	$ERROR('#3: (__func in __obj) === false');
}





if (this.__func === undefined) {
	$ERROR('#4: this.__func !== undefined');
}



