


























package ch.boye.httpclientandroidlib.impl.client;

import java.io.IOException;
import java.io.InterruptedIOException;
import java.net.ConnectException;
import java.net.UnknownHostException;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashSet;
import java.util.Set;

import javax.net.ssl.SSLException;

import ch.boye.httpclientandroidlib.HttpEntityEnclosingRequest;
import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.client.HttpRequestRetryHandler;
import ch.boye.httpclientandroidlib.client.methods.HttpUriRequest;
import ch.boye.httpclientandroidlib.client.protocol.HttpClientContext;
import ch.boye.httpclientandroidlib.protocol.HttpContext;
import ch.boye.httpclientandroidlib.util.Args;






@Immutable
public class DefaultHttpRequestRetryHandler implements HttpRequestRetryHandler {

    public static final DefaultHttpRequestRetryHandler INSTANCE = new DefaultHttpRequestRetryHandler();

    
    private final int retryCount;

    
    private final boolean requestSentRetryEnabled;

    private final Set<Class<? extends IOException>> nonRetriableClasses;

    







    protected DefaultHttpRequestRetryHandler(
            final int retryCount,
            final boolean requestSentRetryEnabled,
            final Collection<Class<? extends IOException>> clazzes) {
        super();
        this.retryCount = retryCount;
        this.requestSentRetryEnabled = requestSentRetryEnabled;
        this.nonRetriableClasses = new HashSet<Class<? extends IOException>>();
        for (final Class<? extends IOException> clazz: clazzes) {
            this.nonRetriableClasses.add(clazz);
        }
    }

    











    @SuppressWarnings("unchecked")
    public DefaultHttpRequestRetryHandler(final int retryCount, final boolean requestSentRetryEnabled) {
        this(retryCount, requestSentRetryEnabled, Arrays.asList(
                InterruptedIOException.class,
                UnknownHostException.class,
                ConnectException.class,
                SSLException.class));
    }

    









    public DefaultHttpRequestRetryHandler() {
        this(3, false);
    }
    



    public boolean retryRequest(
            final IOException exception,
            final int executionCount,
            final HttpContext context) {
        Args.notNull(exception, "Exception parameter");
        Args.notNull(context, "HTTP context");
        if (executionCount > this.retryCount) {
            
            return false;
        }
        if (this.nonRetriableClasses.contains(exception.getClass())) {
            return false;
        } else {
            for (final Class<? extends IOException> rejectException : this.nonRetriableClasses) {
                if (rejectException.isInstance(exception)) {
                    return false;
                }
            }
        }
        final HttpClientContext clientContext = HttpClientContext.adapt(context);
        final HttpRequest request = clientContext.getRequest();

        if(requestIsAborted(request)){
            return false;
        }

        if (handleAsIdempotent(request)) {
            
            return true;
        }

        if (!clientContext.isRequestSent() || this.requestSentRetryEnabled) {
            
            
            return true;
        }
        
        return false;
    }

    



    public boolean isRequestSentRetryEnabled() {
        return requestSentRetryEnabled;
    }

    


    public int getRetryCount() {
        return retryCount;
    }

    


    protected boolean handleAsIdempotent(final HttpRequest request) {
        return !(request instanceof HttpEntityEnclosingRequest);
    }

    




    @Deprecated
    protected boolean requestIsAborted(final HttpRequest request) {
        HttpRequest req = request;
        if (request instanceof RequestWrapper) { 
            req = ((RequestWrapper) request).getOriginal();
        }
        return (req instanceof HttpUriRequest && ((HttpUriRequest)req).isAborted());
    }

}
