

























package ch.boye.httpclientandroidlib.impl.client.cache;

import java.io.IOException;
import java.io.InputStream;
import java.lang.reflect.Proxy;

import ch.boye.httpclientandroidlib.HttpEntity;
import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.client.cache.InputLimit;
import ch.boye.httpclientandroidlib.client.cache.Resource;
import ch.boye.httpclientandroidlib.client.cache.ResourceFactory;
import ch.boye.httpclientandroidlib.client.methods.CloseableHttpResponse;
import ch.boye.httpclientandroidlib.message.BasicHttpResponse;




@NotThreadSafe
class SizeLimitedResponseReader {

    private final ResourceFactory resourceFactory;
    private final long maxResponseSizeBytes;
    private final HttpRequest request;
    private final CloseableHttpResponse response;

    private InputStream instream;
    private InputLimit limit;
    private Resource resource;
    private boolean consumed;

    



    public SizeLimitedResponseReader(
            final ResourceFactory resourceFactory,
            final long maxResponseSizeBytes,
            final HttpRequest request,
            final CloseableHttpResponse response) {
        super();
        this.resourceFactory = resourceFactory;
        this.maxResponseSizeBytes = maxResponseSizeBytes;
        this.request = request;
        this.response = response;
    }

    protected void readResponse() throws IOException {
        if (!consumed) {
            doConsume();
        }
    }

    private void ensureNotConsumed() {
        if (consumed) {
            throw new IllegalStateException("Response has already been consumed");
        }
    }

    private void ensureConsumed() {
        if (!consumed) {
            throw new IllegalStateException("Response has not been consumed");
        }
    }

    private void doConsume() throws IOException {
        ensureNotConsumed();
        consumed = true;

        limit = new InputLimit(maxResponseSizeBytes);

        final HttpEntity entity = response.getEntity();
        if (entity == null) {
            return;
        }
        final String uri = request.getRequestLine().getUri();
        instream = entity.getContent();
        try {
            resource = resourceFactory.generate(uri, instream, limit);
        } finally {
            if (!limit.isReached()) {
                instream.close();
            }
        }
    }

    boolean isLimitReached() {
        ensureConsumed();
        return limit.isReached();
    }

    Resource getResource() {
        ensureConsumed();
        return resource;
    }

    CloseableHttpResponse getReconstructedResponse() throws IOException {
        ensureConsumed();
        final HttpResponse reconstructed = new BasicHttpResponse(response.getStatusLine());
        reconstructed.setHeaders(response.getAllHeaders());

        final CombinedEntity combinedEntity = new CombinedEntity(resource, instream);
        final HttpEntity entity = response.getEntity();
        if (entity != null) {
            combinedEntity.setContentType(entity.getContentType());
            combinedEntity.setContentEncoding(entity.getContentEncoding());
            combinedEntity.setChunked(entity.isChunked());
        }
        reconstructed.setEntity(combinedEntity);
        return (CloseableHttpResponse) Proxy.newProxyInstance(
                ResponseProxyHandler.class.getClassLoader(),
                new Class<?>[] { CloseableHttpResponse.class },
                new ResponseProxyHandler(reconstructed) {

                    @Override
                    public void close() throws IOException {
                        response.close();
                    }

                });
    }

}
