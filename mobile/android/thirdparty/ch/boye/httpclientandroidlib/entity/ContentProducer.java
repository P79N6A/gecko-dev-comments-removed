


























package ch.boye.httpclientandroidlib.entity;

import java.io.IOException;
import java.io.OutputStream;








public interface ContentProducer {

    void writeTo(OutputStream outstream) throws IOException;

}
