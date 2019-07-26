



package org.mozilla.gecko;

public class RoboCopException extends RuntimeException {
    
    public RoboCopException() {
        super();
    }
    
    public RoboCopException(String message) {
        super(message);
    }
    
    public RoboCopException(Throwable cause) {
        super(cause);
    }
    
    public RoboCopException(String message, Throwable cause) {
        super(message, cause);
    }
}
