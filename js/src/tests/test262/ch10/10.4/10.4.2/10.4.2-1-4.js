









var __10_4_2_1_4 = "str";
function testcase() {
        try {
            var o = new Object();
            o.__10_4_2_1_4 = "str2";
            var _eval = eval;
            var __10_4_2_1_4 = "str1";
            with (o) {
                if (_eval("\'str\' === __10_4_2_1_4") === true &&  
                    eval("\'str2\' === __10_4_2_1_4") === true) {  
                    return true;
                }
            }
            return false;
        } finally {
            delete this.__10_4_2_1_4;
        }
    }
runTestCase(testcase);
