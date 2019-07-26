










function testcase() {
        var tokenCodes  = { 
            instanceof: 0,
            typeof: 1,
            else: 2
        };
        var arr = [
            'instanceof',
            'typeof',
            'else'
        ];
        for (var i = 0; i < arr.length; i++) {
            if (tokenCodes[arr[i]] !== i) {
                return false;
            };
        }
        return true;
    }
runTestCase(testcase);
