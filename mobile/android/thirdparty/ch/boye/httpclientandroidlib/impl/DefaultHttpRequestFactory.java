


























package ch.boye.httpclientandroidlib.impl;

import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.HttpRequestFactory;
import ch.boye.httpclientandroidlib.MethodNotSupportedException;
import ch.boye.httpclientandroidlib.RequestLine;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.message.BasicHttpEntityEnclosingRequest;
import ch.boye.httpclientandroidlib.message.BasicHttpRequest;
import ch.boye.httpclientandroidlib.util.Args;






@Immutable
public class DefaultHttpRequestFactory implements HttpRequestFactory {

    public static final DefaultHttpRequestFactory INSTANCE = new DefaultHttpRequestFactory();

    private static final String[] RFC2616_COMMON_METHODS = {
        "GET"
    };

    private static final String[] RFC2616_ENTITY_ENC_METHODS = {
        "POST",
        "PUT"
    };

    private static final String[] RFC2616_SPECIAL_METHODS = {
        "HEAD",
        "OPTIONS",
        "DELETE",
        "TRACE",
        "CONNECT"
    };


    public DefaultHttpRequestFactory() {
        super();
    }

    private static boolean isOneOf(final String[] methods, final String method) {
        for (final String method2 : methods) {
            if (method2.equalsIgnoreCase(method)) {
                return true;
            }
        }
        return false;
    }

    public HttpRequest newHttpRequest(final RequestLine requestline)
            throws MethodNotSupportedException {
        Args.notNull(requestline, "Request line");
        final String method = requestline.getMethod();
        if (isOneOf(RFC2616_COMMON_METHODS, method)) {
            return new BasicHttpRequest(requestline);
        } else if (isOneOf(RFC2616_ENTITY_ENC_METHODS, method)) {
            return new BasicHttpEntityEnclosingRequest(requestline);
        } else if (isOneOf(RFC2616_SPECIAL_METHODS, method)) {
            return new BasicHttpRequest(requestline);
        } else {
            throw new MethodNotSupportedException(method +  " method not supported");
        }
    }

    public HttpRequest newHttpRequest(final String method, final String uri)
            throws MethodNotSupportedException {
        if (isOneOf(RFC2616_COMMON_METHODS, method)) {
            return new BasicHttpRequest(method, uri);
        } else if (isOneOf(RFC2616_ENTITY_ENC_METHODS, method)) {
            return new BasicHttpEntityEnclosingRequest(method, uri);
        } else if (isOneOf(RFC2616_SPECIAL_METHODS, method)) {
            return new BasicHttpRequest(method, uri);
        } else {
            throw new MethodNotSupportedException(method
                    + " method not supported");
        }
    }

}
