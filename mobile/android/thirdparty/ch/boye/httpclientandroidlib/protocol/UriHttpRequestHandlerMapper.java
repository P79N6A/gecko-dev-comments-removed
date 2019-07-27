


























package ch.boye.httpclientandroidlib.protocol;

import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.annotation.ThreadSafe;
import ch.boye.httpclientandroidlib.util.Args;


















@ThreadSafe 
public class UriHttpRequestHandlerMapper implements HttpRequestHandlerMapper {

    private final UriPatternMatcher<HttpRequestHandler> matcher;

    protected UriHttpRequestHandlerMapper(final UriPatternMatcher<HttpRequestHandler> matcher) {
        super();
        this.matcher = Args.notNull(matcher, "Pattern matcher");
    }

    public UriHttpRequestHandlerMapper() {
        this(new UriPatternMatcher<HttpRequestHandler>());
    }

    






    public void register(final String pattern, final HttpRequestHandler handler) {
        Args.notNull(pattern, "Pattern");
        Args.notNull(handler, "Handler");
        matcher.register(pattern, handler);
    }

    




    public void unregister(final String pattern) {
        matcher.unregister(pattern);
    }

    


    protected String getRequestPath(final HttpRequest request) {
        String uriPath = request.getRequestLine().getUri();
        int index = uriPath.indexOf("?");
        if (index != -1) {
            uriPath = uriPath.substring(0, index);
        } else {
            index = uriPath.indexOf("#");
            if (index != -1) {
                uriPath = uriPath.substring(0, index);
            }
        }
        return uriPath;
    }

    





    public HttpRequestHandler lookup(final HttpRequest request) {
        Args.notNull(request, "HTTP request");
        return matcher.lookup(getRequestPath(request));
    }

}
