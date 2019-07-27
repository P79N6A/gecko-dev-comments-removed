package org.mozilla.gecko;













public class Assert {
    
    private Assert() {}

    


    public static void equal(Object a, Object b) {
        equal(a, b, "Assertion failure: !" + a + ".equals(" + b + ')');
    }
    public static void equal(Object a, Object b, String message) {
        isTrue(a.equals(b), message);
    }

    


    public static void isTrue(boolean a) {
        isTrue(a, null);
    }
    public static void isTrue(boolean a, String message)  {
        if (!a) {
            throw new AssertionError(message);
        }
    }

    


    public static void isFalse(boolean a) {
        isTrue(a, null);
    }
    public static void isFalse(boolean a, String message)  {
        if (a) {
            throw new AssertionError(message);
        }
    }

    


    public static void isNull(Object o) {
        isNull(o, "Assertion failure: " + o + " must be null!");
    }
    public static void isNull(Object o, String message) {
        isTrue(o == null, message);
    }

    


    public static void isNotNull(Object o) {
        isNotNull(o, "Assertion failure: " + o + " cannot be null!");
    }
    public static void isNotNull(Object o, String message) {
        isTrue(o != null, message);
    }

    




    public static void fail() {
        isTrue(false);
    }
    public static void fail(String message) {
        isTrue(false, message);
    }
}
