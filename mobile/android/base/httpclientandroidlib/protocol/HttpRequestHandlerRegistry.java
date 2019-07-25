


























package ch.boye.httpclientandroidlib.protocol;

import java.util.Map;


















public class HttpRequestHandlerRegistry implements HttpRequestHandlerResolver {

    private final UriPatternMatcher matcher;

    public HttpRequestHandlerRegistry() {
        matcher = new UriPatternMatcher();
    }

    






    public void register(final String pattern, final HttpRequestHandler handler) {
        if (pattern == null) {
            throw new IllegalArgumentException("URI request pattern may not be null");
        }
        if (handler == null) {
            throw new IllegalArgumentException("Request handler may not be null");
        }
        matcher.register(pattern, handler);
    }

    




    public void unregister(final String pattern) {
        matcher.unregister(pattern);
    }

    



    public void setHandlers(final Map map) {
        matcher.setObjects(map);
    }

    public HttpRequestHandler lookup(final String requestURI) {
        return (HttpRequestHandler) matcher.lookup(requestURI);
    }

    


    protected boolean matchUriRequestPattern(final String pattern, final String requestUri) {
        return matcher.matchUriRequestPattern(pattern, requestUri);
    }

}
