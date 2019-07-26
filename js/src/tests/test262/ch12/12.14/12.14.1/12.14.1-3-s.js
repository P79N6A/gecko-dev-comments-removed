











function testcase() {
        "use strict";

       try{ eval(" try { \
             throw new Error(\"...\");\
             return false;\
         } catch (EVAL) {\
             try\
             {\
               throw new Error(\"...\");\
             }catch(eval)\
             {\
                 return EVAL instanceof Error;\
              }\
         }");
         return false;
        } catch(e) {
             return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
