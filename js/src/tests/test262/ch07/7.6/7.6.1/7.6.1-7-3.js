










function testcase() {
        var tokenCodes = {};
        tokenCodes['instanceof'] = 0;
        tokenCodes['typeof'] = 1;
        tokenCodes['else'] = 2;     
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
