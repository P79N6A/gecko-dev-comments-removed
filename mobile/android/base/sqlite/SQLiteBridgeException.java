




package org.mozilla.gecko.sqlite;

public class SQLiteBridgeException extends RuntimeException {
    static final long serialVersionUID = 1L;

    public SQLiteBridgeException() {}
    public SQLiteBridgeException(String msg) {
        super(msg);
    }
}
