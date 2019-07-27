


























package ch.boye.httpclientandroidlib.entity;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.ObjectOutputStream;
import java.io.OutputStream;
import java.io.Serializable;

import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.util.Args;









@NotThreadSafe
public class SerializableEntity extends AbstractHttpEntity {

    private byte[] objSer;

    private Serializable objRef;

    







    public SerializableEntity(final Serializable ser, final boolean bufferize) throws IOException {
        super();
        Args.notNull(ser, "Source object");
        if (bufferize) {
            createBytes(ser);
        } else {
            this.objRef = ser;
        }
    }

    


    public SerializableEntity(final Serializable ser) {
        super();
        Args.notNull(ser, "Source object");
        this.objRef = ser;
    }

    private void createBytes(final Serializable ser) throws IOException {
        final ByteArrayOutputStream baos = new ByteArrayOutputStream();
        final ObjectOutputStream out = new ObjectOutputStream(baos);
        out.writeObject(ser);
        out.flush();
        this.objSer = baos.toByteArray();
    }

    public InputStream getContent() throws IOException, IllegalStateException {
        if (this.objSer == null) {
            createBytes(this.objRef);
        }
        return new ByteArrayInputStream(this.objSer);
    }

    public long getContentLength() {
        if (this.objSer ==  null) {
            return -1;
        } else {
            return this.objSer.length;
        }
    }

    public boolean isRepeatable() {
        return true;
    }

    public boolean isStreaming() {
        return this.objSer == null;
    }

    public void writeTo(final OutputStream outstream) throws IOException {
        Args.notNull(outstream, "Output stream");
        if (this.objSer == null) {
            final ObjectOutputStream out = new ObjectOutputStream(outstream);
            out.writeObject(this.objRef);
            out.flush();
        } else {
            outstream.write(this.objSer);
            outstream.flush();
        }
    }

}
