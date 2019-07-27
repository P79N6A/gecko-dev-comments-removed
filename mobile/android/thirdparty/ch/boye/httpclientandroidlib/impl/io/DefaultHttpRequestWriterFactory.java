


























package ch.boye.httpclientandroidlib.impl.io;

import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.io.HttpMessageWriter;
import ch.boye.httpclientandroidlib.io.HttpMessageWriterFactory;
import ch.boye.httpclientandroidlib.io.SessionOutputBuffer;
import ch.boye.httpclientandroidlib.message.BasicLineFormatter;
import ch.boye.httpclientandroidlib.message.LineFormatter;






@Immutable
public class DefaultHttpRequestWriterFactory implements HttpMessageWriterFactory<HttpRequest> {

    public static final DefaultHttpRequestWriterFactory INSTANCE = new DefaultHttpRequestWriterFactory();

    private final LineFormatter lineFormatter;

    public DefaultHttpRequestWriterFactory(final LineFormatter lineFormatter) {
        super();
        this.lineFormatter = lineFormatter != null ? lineFormatter : BasicLineFormatter.INSTANCE;
    }

    public DefaultHttpRequestWriterFactory() {
        this(null);
    }

    public HttpMessageWriter<HttpRequest> create(final SessionOutputBuffer buffer) {
        return new DefaultHttpRequestWriter(buffer, lineFormatter);
    }

}
