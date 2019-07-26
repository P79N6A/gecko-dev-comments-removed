

























package ch.boye.httpclientandroidlib.protocol;

import java.io.IOException;

import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.HttpRequestInterceptor;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.HttpResponseInterceptor;







public final class ImmutableHttpProcessor implements HttpProcessor {

    private final HttpRequestInterceptor[] requestInterceptors;
    private final HttpResponseInterceptor[] responseInterceptors;

    public ImmutableHttpProcessor(
            final HttpRequestInterceptor[] requestInterceptors,
            final HttpResponseInterceptor[] responseInterceptors) {
        super();
        if (requestInterceptors != null) {
            int count = requestInterceptors.length;
            this.requestInterceptors = new HttpRequestInterceptor[count];
            for (int i = 0; i < count; i++) {
                this.requestInterceptors[i] = requestInterceptors[i];
            }
        } else {
            this.requestInterceptors = new HttpRequestInterceptor[0];
        }
        if (responseInterceptors != null) {
            int count = responseInterceptors.length;
            this.responseInterceptors = new HttpResponseInterceptor[count];
            for (int i = 0; i < count; i++) {
                this.responseInterceptors[i] = responseInterceptors[i];
            }
        } else {
            this.responseInterceptors = new HttpResponseInterceptor[0];
        }
    }

    public ImmutableHttpProcessor(
            final HttpRequestInterceptorList requestInterceptors,
            final HttpResponseInterceptorList responseInterceptors) {
        super();
        if (requestInterceptors != null) {
            int count = requestInterceptors.getRequestInterceptorCount();
            this.requestInterceptors = new HttpRequestInterceptor[count];
            for (int i = 0; i < count; i++) {
                this.requestInterceptors[i] = requestInterceptors.getRequestInterceptor(i);
            }
        } else {
            this.requestInterceptors = new HttpRequestInterceptor[0];
        }
        if (responseInterceptors != null) {
            int count = responseInterceptors.getResponseInterceptorCount();
            this.responseInterceptors = new HttpResponseInterceptor[count];
            for (int i = 0; i < count; i++) {
                this.responseInterceptors[i] = responseInterceptors.getResponseInterceptor(i);
            }
        } else {
            this.responseInterceptors = new HttpResponseInterceptor[0];
        }
    }

    public ImmutableHttpProcessor(final HttpRequestInterceptor[] requestInterceptors) {
        this(requestInterceptors, null);
    }

    public ImmutableHttpProcessor(final HttpResponseInterceptor[] responseInterceptors) {
        this(null, responseInterceptors);
    }

    public void process(
            final HttpRequest request,
            final HttpContext context) throws IOException, HttpException {
        for (int i = 0; i < this.requestInterceptors.length; i++) {
            this.requestInterceptors[i].process(request, context);
        }
    }

    public void process(
            final HttpResponse response,
            final HttpContext context) throws IOException, HttpException {
        for (int i = 0; i < this.responseInterceptors.length; i++) {
            this.responseInterceptors[i].process(response, context);
        }
    }

}
