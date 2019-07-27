



i = 42;
eval("let (y) { \n" +
     "  (function() { \n" +
     "    let ({} = (y, {})) { \n" +
     "      (function() { \n" +
     "         let ({} = y = []) (i); \n" +
     "       })(); \n" +
     "    } \n" +
     "   })(); \n" +
     "}");
reportCompare(0, 0, "ok");
