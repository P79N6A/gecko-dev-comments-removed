










function testcase() {
        var tokenCodes  = {};
        tokenCodes['finally'] = 0;
        tokenCodes['return'] = 1;
        tokenCodes['void'] = 2;
        var arr = [
            'finally',
            'return',
            'void'
            ];
        for(var p in tokenCodes) {       
            for(var p1 in arr) {                
                if(arr[p1] === p) {
                    if(!tokenCodes.hasOwnProperty(arr[p1])) {
                        return false;
                    };
                }
            }
        }
        return true;
    }
runTestCase(testcase);
