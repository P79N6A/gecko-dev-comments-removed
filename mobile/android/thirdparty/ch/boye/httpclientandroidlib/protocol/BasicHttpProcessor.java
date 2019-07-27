


























package ch.boye.httpclientandroidlib.protocol;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.HttpRequestInterceptor;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.HttpResponseInterceptor;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.util.Args;











@NotThreadSafe
@Deprecated
public final class BasicHttpProcessor implements
    HttpProcessor, HttpRequestInterceptorList, HttpResponseInterceptorList, Cloneable {

    
    protected final List<HttpRequestInterceptor> requestInterceptors = new ArrayList<HttpRequestInterceptor>();
    protected final List<HttpResponseInterceptor> responseInterceptors = new ArrayList<HttpResponseInterceptor>();

    public void addRequestInterceptor(final HttpRequestInterceptor itcp) {
        if (itcp == null) {
            return;
        }
        this.requestInterceptors.add(itcp);
    }

    public void addRequestInterceptor(
            final HttpRequestInterceptor itcp, final int index) {
        if (itcp == null) {
            return;
        }
        this.requestInterceptors.add(index, itcp);
    }

    public void addResponseInterceptor(
            final HttpResponseInterceptor itcp, final int index) {
        if (itcp == null) {
            return;
        }
        this.responseInterceptors.add(index, itcp);
    }

    public void removeRequestInterceptorByClass(final Class<? extends HttpRequestInterceptor> clazz) {
        for (final Iterator<HttpRequestInterceptor> it = this.requestInterceptors.iterator();
             it.hasNext(); ) {
            final Object request = it.next();
            if (request.getClass().equals(clazz)) {
                it.remove();
            }
        }
    }

    public void removeResponseInterceptorByClass(final Class<? extends HttpResponseInterceptor> clazz) {
        for (final Iterator<HttpResponseInterceptor> it = this.responseInterceptors.iterator();
             it.hasNext(); ) {
            final Object request = it.next();
            if (request.getClass().equals(clazz)) {
                it.remove();
            }
        }
    }

    public final void addInterceptor(final HttpRequestInterceptor interceptor) {
        addRequestInterceptor(interceptor);
    }

     public final void addInterceptor(final HttpRequestInterceptor interceptor, final int index) {
        addRequestInterceptor(interceptor, index);
    }

    public int getRequestInterceptorCount() {
        return this.requestInterceptors.size();
    }

    public HttpRequestInterceptor getRequestInterceptor(final int index) {
        if ((index < 0) || (index >= this.requestInterceptors.size())) {
            return null;
        }
        return this.requestInterceptors.get(index);
    }

    public void clearRequestInterceptors() {
        this.requestInterceptors.clear();
    }

    public void addResponseInterceptor(final HttpResponseInterceptor itcp) {
        if (itcp == null) {
            return;
        }
        this.responseInterceptors.add(itcp);
    }

    public final void addInterceptor(final HttpResponseInterceptor interceptor) {
        addResponseInterceptor(interceptor);
    }

    public final void addInterceptor(final HttpResponseInterceptor interceptor, final int index) {
        addResponseInterceptor(interceptor, index);
    }

    public int getResponseInterceptorCount() {
        return this.responseInterceptors.size();
    }

    public HttpResponseInterceptor getResponseInterceptor(final int index) {
        if ((index < 0) || (index >= this.responseInterceptors.size())) {
            return null;
        }
        return this.responseInterceptors.get(index);
    }

    public void clearResponseInterceptors() {
        this.responseInterceptors.clear();
    }

    
















    public void setInterceptors(final List<?> list) {
        Args.notNull(list, "Inteceptor list");
        this.requestInterceptors.clear();
        this.responseInterceptors.clear();
        for (final Object obj : list) {
            if (obj instanceof HttpRequestInterceptor) {
                addInterceptor((HttpRequestInterceptor) obj);
            }
            if (obj instanceof HttpResponseInterceptor) {
                addInterceptor((HttpResponseInterceptor) obj);
            }
        }
    }

    


    public void clearInterceptors() {
        clearRequestInterceptors();
        clearResponseInterceptors();
    }

    public void process(
            final HttpRequest request,
            final HttpContext context)
            throws IOException, HttpException {
        for (final HttpRequestInterceptor interceptor : this.requestInterceptors) {
            interceptor.process(request, context);
        }
    }

    public void process(
            final HttpResponse response,
            final HttpContext context)
            throws IOException, HttpException {
        for (final HttpResponseInterceptor interceptor : this.responseInterceptors) {
            interceptor.process(response, context);
        }
    }

    





    protected void copyInterceptors(final BasicHttpProcessor target) {
        target.requestInterceptors.clear();
        target.requestInterceptors.addAll(this.requestInterceptors);
        target.responseInterceptors.clear();
        target.responseInterceptors.addAll(this.responseInterceptors);
    }

    




    public BasicHttpProcessor copy() {
        final BasicHttpProcessor clone = new BasicHttpProcessor();
        copyInterceptors(clone);
        return clone;
    }

    @Override
    public Object clone() throws CloneNotSupportedException {
        final BasicHttpProcessor clone = (BasicHttpProcessor) super.clone();
        copyInterceptors(clone);
        return clone;
    }

}
