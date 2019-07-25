



a = evalcx('');
a.__proto__ = ''.__proto__;
a.length = 3;  

reportCompare(0, 0, 'ok');
