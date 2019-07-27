


























package ch.boye.httpclientandroidlib.entity.mime;

import java.io.IOException;
import java.io.OutputStream;
import java.nio.charset.Charset;
import java.util.List;







class HttpBrowserCompatibleMultipart extends AbstractMultipartForm {

    private final List<FormBodyPart> parts;

    public HttpBrowserCompatibleMultipart(
            final String subType,
            final Charset charset,
            final String boundary,
            final List<FormBodyPart> parts) {
        super(subType, charset, boundary);
        this.parts = parts;
    }

    @Override
    public List<FormBodyPart> getBodyParts() {
        return this.parts;
    }

    


    @Override
    protected void formatMultipartHeader(
            final FormBodyPart part,
            final OutputStream out) throws IOException {
        
        
        final Header header = part.getHeader();
        final MinimalField cd = header.getField(MIME.CONTENT_DISPOSITION);
        writeField(cd, this.charset, out);
        final String filename = part.getBody().getFilename();
        if (filename != null) {
            final MinimalField ct = header.getField(MIME.CONTENT_TYPE);
            writeField(ct, this.charset, out);
        }

    }

}
