

offThreadCompileScript('Error()');
assertEq(!!runOffThreadScript().stack.match(/^@<string>:1\n/), true);

offThreadCompileScript('Error()',
                       { fileName: "candelabra", lineNumber: 6502 });
assertEq(!!runOffThreadScript().stack.match(/^@candelabra:6502\n/), true);

var element = {};
offThreadCompileScript('Error()', { element: element }); 
runOffThreadScript();

var elementAttribute = "molybdenum";
elementAttribute += elementAttribute + elementAttribute + elementAttribute;
offThreadCompileScript('Error()', { elementProperty: elementAttribute }); 
runOffThreadScript();
