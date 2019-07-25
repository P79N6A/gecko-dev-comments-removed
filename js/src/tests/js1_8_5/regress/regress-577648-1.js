




var count = 0;

function testCaller(obj) {
    switch (++count) {
      case 1:
      case 2:
        






        assertEq(obj, objA);
        break;

      case 3: {
        assertEq(obj, objB);

        


















        obj.go = objA.go;

        let caller = arguments.callee.caller;
        let obj_go = obj.go;
        return caller != obj_go && caller.__proto__ == obj_go.__proto__;
      }

      case 4: {
        assertEq(obj, objC);

        let save = obj.go;
        delete obj.go;
        return arguments.callee.caller == save;
      }

      case 5: {
        assertEq(obj, objD);

        let read = obj.go;
        break;
      }
    }

    return arguments.callee.caller == obj.go;
}

function make() {
    return {
        go: function(obj) {
            return testCaller(obj);
        }
    };
}

var objA = make(),
    objB = make(),
    objC = make(),
    objD = make();

reportCompare(true, objA.go(objA), "1");
reportCompare(true, objA.go(objA), "2");
reportCompare(true, objB.go(objB), "3");
reportCompare(true, objC.go(objC), "4");
reportCompare(true, objD.go(objD), "5");
