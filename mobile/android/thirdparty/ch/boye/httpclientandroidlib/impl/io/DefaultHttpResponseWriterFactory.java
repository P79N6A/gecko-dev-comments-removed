


























package ch.boye.httpclientandroidlib.impl.io;

import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.io.HttpMessageWriter;
import ch.boye.httpclientandroidlib.io.HttpMessageWriterFactory;
import ch.boye.httpclientandroidlib.io.SessionOutputBuffer;
import ch.boye.httpclientandroidlib.message.BasicLineFormatter;
import ch.boye.httpclientandroidlib.message.LineFormatter;






@Immutable
public class DefaultHttpResponseWriterFactory implements HttpMessageWriterFactory<HttpResponse> {

    public static final DefaultHttpResponseWriterFactory INSTANCE = new DefaultHttpResponseWriterFactory();

    private final LineFormatter lineFormatter;

    public DefaultHttpResponseWriterFactory(final LineFormatter lineFormatter) {
        super();
        this.lineFormatter = lineFormatter != null ? lineFormatter : BasicLineFormatter.INSTANCE;
    }

    public DefaultHttpResponseWriterFactory() {
        this(null);
    }

    public HttpMessageWriter<HttpResponse> create(final SessionOutputBuffer buffer) {
        return new DefaultHttpResponseWriter(buffer, lineFormatter);
    }

}
