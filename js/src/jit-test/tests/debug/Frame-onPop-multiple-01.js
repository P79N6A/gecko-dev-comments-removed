

function completionsEqual(c1, c2) {
    if (c1 && c2) {
        if (c1.throw)
            return c1.throw === c2.throw;
        else
            return c1.return === c2.return;
    }
    return c1 === c2;
}

function completionString(c) {
    if (c == null)
        return 'x';
    if (c.return)
        return 'r' + c.return;
    if (c.throw)
        return 't' + c.throw;
    return '?';
}

var g = newGlobal('new-compartment'); 
g.eval('function f() { debugger; return "1"; }');







var sequence = [{ expect: { return: '1' }, resume: { return: '2'} },
                { expect: { return: '2' }, resume: { throw:  '3'} },
                { expect: { throw:  '3' }, resume: { return: '4'} },
                { expect: { return: '4' }, resume: null },
                { expect: null,            resume: { throw:  '5'} },
                { expect: { throw:  '5' }, resume: { throw:  '6'} },
                { expect: { throw:  '6' }, resume: null           },
                { expect: null,            resume: null           },
                { expect: null,            resume: { return: '7'} }];



var frames = [];




var dbg0 = new Debugger(g);
dbg0.onEnterFrame = function handleOriginalEnter(frame) {
    dbg0.log += '(';
    dbg0.onEnterFrame = undefined;

    assertEq(frame.live, true);
    frames.push(frame);

    var dbgs = [];
    var log;

    
    for (s in sequence) {
        
        
        
        
        let dbg = new Debugger(g);
        dbgs.push(dbg);

        dbg.onDebuggerStatement = function handleDebuggerStatement(f) {
            log += 'd';  
            assertEq(f.live, true);
            frames.push(f);
        };

        
        dbg.onEnterFrame = function handleEnterEval(f) {
            log += 'e';
            assertEq(f.type, 'eval');
            assertEq(f.live, true);
            frames.push(f);

            
            dbg.onEnterFrame = function handleEnterCall(f) {
                log += '(';
                assertEq(f.type, 'call');
                assertEq(f.live, true);
                frames.push(f);

                
                dbg.onEnterFrame = function handleExtraEnter(f) {
                    log += 'z';
                };

                f.onPop = function handlePop(c) {
                    log += ')' + completionString(c);
                    assertEq(this.live, true);
                    frames.push(this);

                    
                    var i = dbgs.indexOf(dbg);
                    assertEq(i != -1, true);
                    dbgs.splice(i,1);

                    
                    assertEq(completionsEqual(c, sequence[0].expect), true);

                    
                    return sequence.shift().resume;
                };
            };
        };
    }

    log = '';
    assertEq(completionsEqual(frame.eval('f()'), { return: '7' }), true);
    assertEq(log, "eeeeeeeee(((((((((ddddddddd)r1)r2)t3)r4)x)t5)t6)x)x");

    dbg0.log += '.';    
};

dbg0.log = '';
g.eval('eval');
assertEq(dbg0.log, '(.');


for (var i = 0; i < frames.length; i++)
    assertEq(frames[i].live, false);
