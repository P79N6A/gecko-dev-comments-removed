


























package ch.boye.httpclientandroidlib.entity.mime;

import ch.boye.httpclientandroidlib.entity.ContentType;
import ch.boye.httpclientandroidlib.entity.mime.content.AbstractContentBody;
import ch.boye.httpclientandroidlib.entity.mime.content.ContentBody;
import ch.boye.httpclientandroidlib.util.Args;








public class FormBodyPart {

    private final String name;
    private final Header header;

    private final ContentBody body;

    public FormBodyPart(final String name, final ContentBody body) {
        super();
        Args.notNull(name, "Name");
        Args.notNull(body, "Body");
        this.name = name;
        this.body = body;
        this.header = new Header();

        generateContentDisp(body);
        generateContentType(body);
        generateTransferEncoding(body);
    }

    public String getName() {
        return this.name;
    }

    public ContentBody getBody() {
        return this.body;
    }

    public Header getHeader() {
        return this.header;
    }

    public void addField(final String name, final String value) {
        Args.notNull(name, "Field name");
        this.header.addField(new MinimalField(name, value));
    }

    protected void generateContentDisp(final ContentBody body) {
        final StringBuilder buffer = new StringBuilder();
        buffer.append("form-data; name=\"");
        buffer.append(getName());
        buffer.append("\"");
        if (body.getFilename() != null) {
            buffer.append("; filename=\"");
            buffer.append(body.getFilename());
            buffer.append("\"");
        }
        addField(MIME.CONTENT_DISPOSITION, buffer.toString());
    }

    protected void generateContentType(final ContentBody body) {
        final ContentType contentType;
        if (body instanceof AbstractContentBody) {
            contentType = ((AbstractContentBody) body).getContentType();
        } else {
            contentType = null;
        }
        if (contentType != null) {
            addField(MIME.CONTENT_TYPE, contentType.toString());
        } else {
            final StringBuilder buffer = new StringBuilder();
            buffer.append(body.getMimeType()); 
            if (body.getCharset() != null) { 
                buffer.append("; charset=");
                buffer.append(body.getCharset());
            }
            addField(MIME.CONTENT_TYPE, buffer.toString());
        }
    }

    protected void generateTransferEncoding(final ContentBody body) {
        addField(MIME.CONTENT_TRANSFER_ENC, body.getTransferEncoding()); 
    }

}
