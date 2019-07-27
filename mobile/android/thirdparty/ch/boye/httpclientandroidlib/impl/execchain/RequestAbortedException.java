


























package ch.boye.httpclientandroidlib.impl.execchain;

import java.io.InterruptedIOException;

import ch.boye.httpclientandroidlib.annotation.Immutable;






@Immutable
public class RequestAbortedException extends InterruptedIOException {

    private static final long serialVersionUID = 4973849966012490112L;

    public RequestAbortedException(final String message) {
        super(message);
    }

    public RequestAbortedException(final String message, final Throwable cause) {
        super(message);
        if (cause != null) {
            initCause(cause);
        }
    }

}
