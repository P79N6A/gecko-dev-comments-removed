


























package ch.boye.httpclientandroidlib.entity;

import java.io.IOException;
import java.io.OutputStream;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HttpEntity;
import ch.boye.httpclientandroidlib.message.BasicHeader;
import ch.boye.httpclientandroidlib.protocol.HTTP;








public abstract class AbstractHttpEntity implements HttpEntity {

    protected Header contentType;
    protected Header contentEncoding;
    protected boolean chunked;

    




    protected AbstractHttpEntity() {
        super();
    }


    






    public Header getContentType() {
        return this.contentType;
    }


    






    public Header getContentEncoding() {
        return this.contentEncoding;
    }

    






    public boolean isChunked() {
        return this.chunked;
    }


    







    public void setContentType(final Header contentType) {
        this.contentType = contentType;
    }

    







    public void setContentType(final String ctString) {
        Header h = null;
        if (ctString != null) {
            h = new BasicHeader(HTTP.CONTENT_TYPE, ctString);
        }
        setContentType(h);
    }


    







    public void setContentEncoding(final Header contentEncoding) {
        this.contentEncoding = contentEncoding;
    }

    







    public void setContentEncoding(final String ceString) {
        Header h = null;
        if (ceString != null) {
            h = new BasicHeader(HTTP.CONTENT_ENCODING, ceString);
        }
        setContentEncoding(h);
    }


    













    public void setChunked(boolean b) {
        this.chunked = b;
    }


    





    public void consumeContent() throws IOException {
    }

}
