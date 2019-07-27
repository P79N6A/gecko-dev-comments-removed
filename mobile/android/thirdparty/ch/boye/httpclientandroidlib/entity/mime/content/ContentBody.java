


























package ch.boye.httpclientandroidlib.entity.mime.content;

import java.io.IOException;
import java.io.OutputStream;





public interface ContentBody extends ContentDescriptor {

    String getFilename();

    void writeTo(OutputStream out) throws IOException;

}
