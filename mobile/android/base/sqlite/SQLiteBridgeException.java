




package org.mozilla.gecko.sqlite;

import org.mozilla.gecko.mozglue.generatorannotations.WrapEntireClassForJNI;

@WrapEntireClassForJNI
public class SQLiteBridgeException extends RuntimeException {
    static final long serialVersionUID = 1L;

    public SQLiteBridgeException() {}
    public SQLiteBridgeException(String msg) {
        super(msg);
    }
}
