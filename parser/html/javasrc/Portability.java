





















package nu.validator.htmlparser.impl;

import nu.validator.htmlparser.annotation.Literal;
import nu.validator.htmlparser.annotation.Local;
import nu.validator.htmlparser.annotation.NoLength;
import nu.validator.htmlparser.common.Interner;

public final class Portability {

    

    



    public static @Local String newLocalNameFromBuffer(@NoLength char[] buf, int offset, int length, Interner interner) {
        return new String(buf, offset, length).intern();
    }

    public static String newStringFromBuffer(@NoLength char[] buf, int offset, int length) {
        return new String(buf, offset, length);
    }

    public static String newEmptyString() {
        return "";
    }

    public static String newStringFromLiteral(@Literal String literal) {
        return literal;
    }
    
    public static String newStringFromString(String string) {
        return string;
    }
    
    
    public static char[] newCharArrayFromLocal(@Local String local) {
        return local.toCharArray();
    }

    public static char[] newCharArrayFromString(String string) {
        return string.toCharArray();
    }
    
    public static @Local String newLocalFromLocal(@Local String local, Interner interner) {
        return local;
    }
    
    
    
    public static void releaseString(String str) {
        
    }
    
    
    
    public static boolean localEqualsBuffer(@Local String local, @NoLength char[] buf, int offset, int length) {
        if (local.length() != length) {
            return false;
        }
        for (int i = 0; i < length; i++) {
            if (local.charAt(i) != buf[offset + i]) {
                return false;
            }
        }
        return true;
    }

    public static boolean lowerCaseLiteralIsPrefixOfIgnoreAsciiCaseString(@Literal String lowerCaseLiteral,
            String string) {
        if (string == null) {
            return false;
        }
        if (lowerCaseLiteral.length() > string.length()) {
            return false;
        }
        for (int i = 0; i < lowerCaseLiteral.length(); i++) {
            char c0 = lowerCaseLiteral.charAt(i);
            char c1 = string.charAt(i);
            if (c1 >= 'A' && c1 <= 'Z') {
                c1 += 0x20;
            }
            if (c0 != c1) {
                return false;
            }
        }
        return true;
    }
    
    public static boolean lowerCaseLiteralEqualsIgnoreAsciiCaseString(@Literal String lowerCaseLiteral,
            String string) {
        if (string == null) {
            return false;
        }
        if (lowerCaseLiteral.length() != string.length()) {
            return false;
        }
        for (int i = 0; i < lowerCaseLiteral.length(); i++) {
            char c0 = lowerCaseLiteral.charAt(i);
            char c1 = string.charAt(i);
            if (c1 >= 'A' && c1 <= 'Z') {
                c1 += 0x20;
            }
            if (c0 != c1) {
                return false;
            }
        }
        return true;
    }
    
    public static boolean literalEqualsString(@Literal String literal, String string) {
        return literal.equals(string);
    }

    public static boolean stringEqualsString(String one, String other) {
        return one.equals(other);
    }
    
    public static void delete(Object o) {
        
    }

    public static void deleteArray(Object o) {
        
    }
}
