










var evalStr = 
'//CHECK#4\n'+
'if ( eval === null ) {\n'+
'  $ERROR("#4: eval === null");\n'+
'}\n'+

'//CHECK#5\n'+
'if ( parseInt === null ) {\n'+
'  $ERROR("#5: parseInt === null");\n'+
'}\n'+

'//CHECK#6\n'+
'if ( parseFloat === null ) {\n'+
'  $ERROR("#6: parseFloat === null");\n'+
'}\n'+

'//CHECK#7\n'+
'if ( isNaN === null ) {\n'+
'  $ERROR("#7: isNaN === null");\n'+
'}\n'+

'//CHECK#8\n'+
'if ( isFinite === null ) {\n'+
'  $ERROR("#8: isFinite === null");\n'+
'}\n'+

'//CHECK#9\n'+
'if ( decodeURI === null ) {\n'+
'  $ERROR("#9: decodeURI === null");\n'+
'}\n'+

'//CHECK#10\n'+
'if ( decodeURIComponent === null ) {\n'+
'  $ERROR("#10: decodeURIComponent === null");\n'+
'}\n'+

'//CHECK#11\n'+
'if ( encodeURI === null ) {\n'+
'  $ERROR("#11: encodeURI === null");\n'+
'}\n'+

'//CHECK#12\n'+
'if ( encodeURIComponent === null ) {\n'+
'  $ERROR("#12: encodeURIComponent === null");\n'+
'}\n'+
';\n';

eval(evalStr);

