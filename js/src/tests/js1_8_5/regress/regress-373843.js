


if (typeof disassemble != 'undefined')
{
    var func = disassemble(function() { return "c\\d"; })

    
    
    assertEq(func.indexOf("\\\\\\\\"), -1)
}

reportCompare(0, 0, 'ok');
