










function testcase() {
        var tokenCodes = {
            package: 0,
            protected: 1,
            static: 2
        };
        var arr = [
            'package',
            'protected',
            'static'
        ];  
        for (var i = 0; i < arr.length; i++) {
            if (tokenCodes[arr[i]] !== i) {
                return false;
            };
        }
        return true;
    }
runTestCase(testcase);
