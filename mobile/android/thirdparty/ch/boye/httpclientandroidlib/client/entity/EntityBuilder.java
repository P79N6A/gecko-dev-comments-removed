


























package ch.boye.httpclientandroidlib.client.entity;

import java.io.File;
import java.io.InputStream;
import java.io.Serializable;
import java.util.Arrays;
import java.util.List;

import ch.boye.httpclientandroidlib.HttpEntity;
import ch.boye.httpclientandroidlib.NameValuePair;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.entity.AbstractHttpEntity;
import ch.boye.httpclientandroidlib.entity.BasicHttpEntity;
import ch.boye.httpclientandroidlib.entity.ByteArrayEntity;
import ch.boye.httpclientandroidlib.entity.ContentType;
import ch.boye.httpclientandroidlib.entity.FileEntity;
import ch.boye.httpclientandroidlib.entity.InputStreamEntity;
import ch.boye.httpclientandroidlib.entity.SerializableEntity;
import ch.boye.httpclientandroidlib.entity.StringEntity;


















@NotThreadSafe
public class EntityBuilder {

    private String text;
    private byte[] binary;
    private InputStream stream;
    private List<NameValuePair> parameters;
    private Serializable serializable;
    private File file;
    private ContentType contentType;
    private String contentEncoding;
    private boolean chunked;
    private boolean gzipCompress;

    EntityBuilder() {
        super();
    }

    public static EntityBuilder create() {
        return new EntityBuilder();
    }

    private void clearContent() {
        this.text = null;
        this.binary = null;
        this.stream = null;
        this.parameters = null;
        this.serializable = null;
        this.file = null;
    }

    


    public String getText() {
        return text;
    }

    








    public EntityBuilder setText(final String text) {
        clearContent();
        this.text = text;
        return this;
    }

    



    public byte[] getBinary() {
        return binary;
    }

    








    public EntityBuilder setBinary(final byte[] binary) {
        clearContent();
        this.binary = binary;
        return this;
    }

    



    public InputStream getStream() {
        return stream;
    }

    








    public EntityBuilder setStream(final InputStream stream) {
        clearContent();
        this.stream = stream;
        return this;
    }

    




    public List<NameValuePair> getParameters() {
        return parameters;
    }

    







    public EntityBuilder setParameters(final List<NameValuePair> parameters) {
        clearContent();
        this.parameters = parameters;
        return this;
    }

    







    public EntityBuilder setParameters(final NameValuePair... parameters) {
        return setParameters(Arrays.asList(parameters));
    }

    



    public Serializable getSerializable() {
        return serializable;
    }

    








    public EntityBuilder setSerializable(final Serializable serializable) {
        clearContent();
        this.serializable = serializable;
        return this;
    }

    



    public File getFile() {
        return file;
    }

    








    public EntityBuilder setFile(final File file) {
        clearContent();
        this.file = file;
        return this;
    }

    


    public ContentType getContentType() {
        return contentType;
    }

    


    public EntityBuilder setContentType(final ContentType contentType) {
        this.contentType = contentType;
        return this;
    }

    


    public String getContentEncoding() {
        return contentEncoding;
    }

    


    public EntityBuilder setContentEncoding(final String contentEncoding) {
        this.contentEncoding = contentEncoding;
        return this;
    }

    


    public boolean isChunked() {
        return chunked;
    }

    


    public EntityBuilder chunked() {
        this.chunked = true;
        return this;
    }

    


    public boolean isGzipCompress() {
        return gzipCompress;
    }

    


    public EntityBuilder gzipCompress() {
        this.gzipCompress = true;
        return this;
    }

    private ContentType getContentOrDefault(final ContentType def) {
        return this.contentType != null ? this.contentType : def;
    }

    


    public HttpEntity build() {
        final AbstractHttpEntity e;
        if (this.text != null) {
            e = new StringEntity(this.text, getContentOrDefault(ContentType.DEFAULT_TEXT));
        } else if (this.binary != null) {
            e = new ByteArrayEntity(this.binary, getContentOrDefault(ContentType.DEFAULT_BINARY));
        } else if (this.stream != null) {
            e = new InputStreamEntity(this.stream, 1, getContentOrDefault(ContentType.DEFAULT_BINARY));
        } else if (this.parameters != null) {
            e = new UrlEncodedFormEntity(this.parameters,
                    this.contentType != null ? this.contentType.getCharset() : null);
        } else if (this.serializable != null) {
            e = new SerializableEntity(this.serializable);
            e.setContentType(ContentType.DEFAULT_BINARY.toString());
        } else if (this.file != null) {
            e = new FileEntity(this.file, getContentOrDefault(ContentType.DEFAULT_BINARY));
        } else {
            e = new BasicHttpEntity();
        }
        if (e.getContentType() != null && this.contentType != null) {
            e.setContentType(this.contentType.toString());
        }
        e.setContentEncoding(this.contentEncoding);
        e.setChunked(this.chunked);
        if (this.gzipCompress) {
            return new GzipCompressingEntity(e);
        }
        return e;
    }

}
