


























package ch.boye.httpclientandroidlib.io;

import ch.boye.httpclientandroidlib.HttpMessage;
import ch.boye.httpclientandroidlib.config.MessageConstraints;






public interface HttpMessageParserFactory<T extends HttpMessage> {

    HttpMessageParser<T> create(SessionInputBuffer buffer, MessageConstraints constraints);

}
