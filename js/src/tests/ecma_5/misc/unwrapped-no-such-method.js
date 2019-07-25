

var x = "";

try { x.i(); } catch (ex) { }


try { x[x](); } catch (ex) { }


try { true.i(); } catch(ex) { }

reportCompare(true,true);
