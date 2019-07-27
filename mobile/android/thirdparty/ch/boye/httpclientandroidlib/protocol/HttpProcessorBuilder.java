


























package ch.boye.httpclientandroidlib.protocol;

import ch.boye.httpclientandroidlib.HttpRequestInterceptor;
import ch.boye.httpclientandroidlib.HttpResponseInterceptor;






public class HttpProcessorBuilder {

    private ChainBuilder<HttpRequestInterceptor> requestChainBuilder;
    private ChainBuilder<HttpResponseInterceptor> responseChainBuilder;

    public static HttpProcessorBuilder create() {
        return new HttpProcessorBuilder();
    }

    HttpProcessorBuilder() {
        super();
    }

    private ChainBuilder<HttpRequestInterceptor> getRequestChainBuilder() {
        if (requestChainBuilder == null) {
            requestChainBuilder = new ChainBuilder<HttpRequestInterceptor>();
        }
        return requestChainBuilder;
    }

    private ChainBuilder<HttpResponseInterceptor> getResponseChainBuilder() {
        if (responseChainBuilder == null) {
            responseChainBuilder = new ChainBuilder<HttpResponseInterceptor>();
        }
        return responseChainBuilder;
    }

    public HttpProcessorBuilder addFirst(final HttpRequestInterceptor e) {
        if (e == null) {
            return this;
        }
        getRequestChainBuilder().addFirst(e);
        return this;
    }

    public HttpProcessorBuilder addLast(final HttpRequestInterceptor e) {
        if (e == null) {
            return this;
        }
        getRequestChainBuilder().addLast(e);
        return this;
    }

    public HttpProcessorBuilder add(final HttpRequestInterceptor e) {
        return addLast(e);
    }

    public HttpProcessorBuilder addAllFirst(final HttpRequestInterceptor... e) {
        if (e == null) {
            return this;
        }
        getRequestChainBuilder().addAllFirst(e);
        return this;
    }

    public HttpProcessorBuilder addAllLast(final HttpRequestInterceptor... e) {
        if (e == null) {
            return this;
        }
        getRequestChainBuilder().addAllLast(e);
        return this;
    }

    public HttpProcessorBuilder addAll(final HttpRequestInterceptor... e) {
        return addAllLast(e);
    }

    public HttpProcessorBuilder addFirst(final HttpResponseInterceptor e) {
        if (e == null) {
            return this;
        }
        getResponseChainBuilder().addFirst(e);
        return this;
    }

    public HttpProcessorBuilder addLast(final HttpResponseInterceptor e) {
        if (e == null) {
            return this;
        }
        getResponseChainBuilder().addLast(e);
        return this;
    }

    public HttpProcessorBuilder add(final HttpResponseInterceptor e) {
        return addLast(e);
    }

    public HttpProcessorBuilder addAllFirst(final HttpResponseInterceptor... e) {
        if (e == null) {
            return this;
        }
        getResponseChainBuilder().addAllFirst(e);
        return this;
    }

    public HttpProcessorBuilder addAllLast(final HttpResponseInterceptor... e) {
        if (e == null) {
            return this;
        }
        getResponseChainBuilder().addAllLast(e);
        return this;
    }

    public HttpProcessorBuilder addAll(final HttpResponseInterceptor... e) {
        return addAllLast(e);
    }

    public HttpProcessor build() {
        return new ImmutableHttpProcessor(
                requestChainBuilder != null ? requestChainBuilder.build() : null,
                responseChainBuilder != null ? responseChainBuilder.build() : null);
    }

}
