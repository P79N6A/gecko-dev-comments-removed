


for (var i = 0; i < 2; i++) {
    try {
        (#1={x: Object.seal(#1#)});  
    } catch (exc) {}
}

for (var i = 0; i < 2; i++)
    (#1={x: (#1#.x = 1, Object.seal(#1#))});  

reportCompare(0, 0, 'ok');
