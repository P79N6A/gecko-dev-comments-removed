




load(libdir + "../../tests/ecma_6/shell.js");

if (typeof assertWarning === 'undefined') {
    var assertWarning = function assertWarning(f, errorClass, msg) {
        var hadWerror = options().split(",").indexOf("werror") !== -1;

        
        if (hadWerror)
            options("werror");

        try {
            f();
        } catch (exc) {
            if (hadWerror)
                options("werror");

            
            
            if (msg)
                print("assertWarning: " + msg);
            print("assertWarning: Unexpected exception calling " + f +
                  " with warnings-as-errors disabled");
            throw exc;
        }

        
        options("werror");

        try {
            assertThrowsInstanceOf(f, errorClass, msg);
        } catch (exc) {
            if (msg)
                print("assertWarning: " + msg);
            throw exc;
        } finally {
            if (!hadWerror)
                options("werror");
        }
    };
}

if (typeof assertNoWarning === 'undefined') {
    var assertNoWarning = function assertWarning(f, msg) {
        
        var hadWerror = options().split(",").indexOf("werror") !== -1;
        if (!hadWerror)
            options("werror");

        try {
            f();
        } catch (exc) {
            if (msg)
                print("assertNoWarning: " + msg);
            print("assertNoWarning: Unexpected exception calling " + f +
                  "with warnings-as-errors enabled");
            throw exc;
        } finally {
            if (!hadWerror)
                options("werror");
        }
    };
}
