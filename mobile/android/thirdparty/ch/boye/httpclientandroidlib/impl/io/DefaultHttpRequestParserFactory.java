


























package ch.boye.httpclientandroidlib.impl.io;

import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.HttpRequestFactory;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.config.MessageConstraints;
import ch.boye.httpclientandroidlib.impl.DefaultHttpRequestFactory;
import ch.boye.httpclientandroidlib.io.HttpMessageParser;
import ch.boye.httpclientandroidlib.io.HttpMessageParserFactory;
import ch.boye.httpclientandroidlib.io.SessionInputBuffer;
import ch.boye.httpclientandroidlib.message.BasicLineParser;
import ch.boye.httpclientandroidlib.message.LineParser;






@Immutable
public class DefaultHttpRequestParserFactory implements HttpMessageParserFactory<HttpRequest> {

    public static final DefaultHttpRequestParserFactory INSTANCE = new DefaultHttpRequestParserFactory();

    private final LineParser lineParser;
    private final HttpRequestFactory requestFactory;

    public DefaultHttpRequestParserFactory(final LineParser lineParser,
            final HttpRequestFactory requestFactory) {
        super();
        this.lineParser = lineParser != null ? lineParser : BasicLineParser.INSTANCE;
        this.requestFactory = requestFactory != null ? requestFactory
                : DefaultHttpRequestFactory.INSTANCE;
    }

    public DefaultHttpRequestParserFactory() {
        this(null, null);
    }

    public HttpMessageParser<HttpRequest> create(final SessionInputBuffer buffer,
            final MessageConstraints constraints) {
        return new DefaultHttpRequestParser(buffer, lineParser, requestFactory, constraints);
    }

}
