


var g = newGlobal('new-compartment');
var dbg = new Debugger(g);




function test(type, provocation) {
    
    print("type:        " + uneval(type));
    print("provocation: " + uneval(provocation));

    var log;
    dbg.onEnterFrame = function handleFirstFrame(f) {
        log += 'f';
        dbg.onDebuggerStatement = function handleDebugger(f) {
            log += 'd';
            return null;
        };

        dbg.onEnterFrame = function handleSecondFrame(f) {
            log += 'e';
            assertEq(f.type, 'eval');

            dbg.onEnterFrame = function handleThirdFrame(f) {
                log += '(';
                assertEq(f.type, type);

                dbg.onEnterFrame = function handleExtraFrames(f) {
                    
                    assertEq(false, true);
                };

                f.onPop = function handlePop(c) {
                    log += ')';
                    assertEq(c, null);
                };
            };
        };

        assertEq(f.eval(provocation), null);
    };

    log = '';
    
    assertEq(typeof g.eval('eval'), 'function');
    assertEq(log, 'fe(d)');

    print();
}

g.eval('function f() { debugger; return \'termination fail\'; }');
test('call', 'f();');
test('call', 'new f;');
test('eval', 'eval(\'debugger; \\\'termination fail\\\';\');');
test('global', 'evaluate(\'debugger; \\\'termination fail\\\';\');');
throw 'TestComplete';
