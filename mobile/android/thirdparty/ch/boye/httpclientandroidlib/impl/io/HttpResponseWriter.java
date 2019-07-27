


























package ch.boye.httpclientandroidlib.impl.io;

import java.io.IOException;

import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.io.SessionOutputBuffer;
import ch.boye.httpclientandroidlib.message.LineFormatter;
import ch.boye.httpclientandroidlib.params.HttpParams;









@NotThreadSafe
@Deprecated
public class HttpResponseWriter extends AbstractMessageWriter<HttpResponse> {

    public HttpResponseWriter(final SessionOutputBuffer buffer,
                              final LineFormatter formatter,
                              final HttpParams params) {
        super(buffer, formatter, params);
    }

    @Override
    protected void writeHeadLine(final HttpResponse message) throws IOException {
        lineFormatter.formatStatusLine(this.lineBuf, message.getStatusLine());
        this.sessionBuffer.writeLine(this.lineBuf);
    }

}
