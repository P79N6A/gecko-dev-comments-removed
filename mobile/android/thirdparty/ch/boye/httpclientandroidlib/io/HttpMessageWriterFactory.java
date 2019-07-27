


























package ch.boye.httpclientandroidlib.io;

import ch.boye.httpclientandroidlib.HttpMessage;






public interface HttpMessageWriterFactory<T extends HttpMessage> {

    HttpMessageWriter<T> create(SessionOutputBuffer buffer);

}
