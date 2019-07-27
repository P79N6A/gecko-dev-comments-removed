


























package ch.boye.httpclientandroidlib.entity;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.util.Args;






@NotThreadSafe
public class FileEntity extends AbstractHttpEntity implements Cloneable {

    protected final File file;

    


    @Deprecated
    public FileEntity(final File file, final String contentType) {
        super();
        this.file = Args.notNull(file, "File");
        setContentType(contentType);
    }

    


    public FileEntity(final File file, final ContentType contentType) {
        super();
        this.file = Args.notNull(file, "File");
        if (contentType != null) {
            setContentType(contentType.toString());
        }
    }

    


    public FileEntity(final File file) {
        super();
        this.file = Args.notNull(file, "File");
    }

    public boolean isRepeatable() {
        return true;
    }

    public long getContentLength() {
        return this.file.length();
    }

    public InputStream getContent() throws IOException {
        return new FileInputStream(this.file);
    }

    public void writeTo(final OutputStream outstream) throws IOException {
        Args.notNull(outstream, "Output stream");
        final InputStream instream = new FileInputStream(this.file);
        try {
            final byte[] tmp = new byte[OUTPUT_BUFFER_SIZE];
            int l;
            while ((l = instream.read(tmp)) != -1) {
                outstream.write(tmp, 0, l);
            }
            outstream.flush();
        } finally {
            instream.close();
        }
    }

    




    public boolean isStreaming() {
        return false;
    }

    @Override
    public Object clone() throws CloneNotSupportedException {
        
        
        return super.clone();
    }

} 
