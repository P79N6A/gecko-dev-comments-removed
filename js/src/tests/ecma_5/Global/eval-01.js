


var a = 9;
var global = this;

function test() {
    var a = 0;

    
    assertEq(eval('a+1'), 1);
    assertEq(eval('eval("a+1")'), 1);

    
    var foo = eval;
    assertEq(foo('a+1'), 10);
    assertEq(eval('foo("a+1")'), 10); 

    
    assertEq(this.eval("a+1"), 10);
    assertEq(global.eval("a+1"), 10);
    var obj = {foo: eval, eval: eval};
    assertEq(obj.foo('a+1'), 10);
    assertEq(obj.eval('a+1'), 10);
    var name = "eval";
    assertEq(obj[name]('a+1'), 10);
    assertEq([eval][0]('a+1'), 10);

    
    assertEq(eval.call(undefined, 'a+1'), 10);
    assertEq(eval.call(global, 'a+1'), 10);
    assertEq(eval.apply(undefined, ['a+1']), 10);
    assertEq(eval.apply(global, ['a+1']), 10);
    assertEq(['a+1'].map(eval)[0], 10);
}

test();
reportCompare(0, 0);
