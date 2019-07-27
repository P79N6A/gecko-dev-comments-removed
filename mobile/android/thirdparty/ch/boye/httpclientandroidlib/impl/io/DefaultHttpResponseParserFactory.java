


























package ch.boye.httpclientandroidlib.impl.io;

import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.HttpResponseFactory;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.config.MessageConstraints;
import ch.boye.httpclientandroidlib.impl.DefaultHttpResponseFactory;
import ch.boye.httpclientandroidlib.io.HttpMessageParser;
import ch.boye.httpclientandroidlib.io.HttpMessageParserFactory;
import ch.boye.httpclientandroidlib.io.SessionInputBuffer;
import ch.boye.httpclientandroidlib.message.BasicLineParser;
import ch.boye.httpclientandroidlib.message.LineParser;






@Immutable
public class DefaultHttpResponseParserFactory implements HttpMessageParserFactory<HttpResponse> {

    public static final DefaultHttpResponseParserFactory INSTANCE = new DefaultHttpResponseParserFactory();

    private final LineParser lineParser;
    private final HttpResponseFactory responseFactory;

    public DefaultHttpResponseParserFactory(final LineParser lineParser,
            final HttpResponseFactory responseFactory) {
        super();
        this.lineParser = lineParser != null ? lineParser : BasicLineParser.INSTANCE;
        this.responseFactory = responseFactory != null ? responseFactory
                : DefaultHttpResponseFactory.INSTANCE;
    }

    public DefaultHttpResponseParserFactory() {
        this(null, null);
    }

    public HttpMessageParser<HttpResponse> create(final SessionInputBuffer buffer,
            final MessageConstraints constraints) {
        return new DefaultHttpResponseParser(buffer, lineParser, responseFactory, constraints);
    }

}
