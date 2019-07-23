
if (options().split().indexOf('strict') == -1)
    options('strict');
if (options().split().indexOf('werror') == -1)
    options('werror');

function expectSyntaxError(stmt) {
    print(stmt);
    var result = 'no error';
    try {
        Function(stmt);
    } catch (exc) {
        result = exc.constructor.name;
    }
    assertEq(result, 'SyntaxError');
}

function test(expr) {
    
    expectSyntaxError('if (' + expr + ') ;');

    
    Function('if ((' + expr + ')) ;');
}


test('a = 0');
test('a = (f(), g)');
test('a = b || c > d');
