


























package ch.boye.httpclientandroidlib.entity.mime.content;

import java.nio.charset.Charset;

import ch.boye.httpclientandroidlib.entity.ContentType;
import ch.boye.httpclientandroidlib.util.Args;





public abstract class AbstractContentBody implements ContentBody {

    private final ContentType contentType;

    


    public AbstractContentBody(final ContentType contentType) {
        super();
        Args.notNull(contentType, "Content type");
        this.contentType = contentType;
    }

    


    @Deprecated
    public AbstractContentBody(final String mimeType) {
        this(ContentType.parse(mimeType));
    }

    


    public ContentType getContentType() {
        return this.contentType;
    }

    public String getMimeType() {
        return this.contentType.getMimeType();
    }

    public String getMediaType() {
        final String mimeType = this.contentType.getMimeType();
        final int i = mimeType.indexOf('/');
        if (i != -1) {
            return mimeType.substring(0, i);
        } else {
            return mimeType;
        }
    }

    public String getSubType() {
        final String mimeType = this.contentType.getMimeType();
        final int i = mimeType.indexOf('/');
        if (i != -1) {
            return mimeType.substring(i + 1);
        } else {
            return null;
        }
    }

    public String getCharset() {
        final Charset charset = this.contentType.getCharset();
        return charset != null ? charset.name() : null;
    }

}
