



package org.mozilla.gecko.background.bagheera;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import ch.boye.httpclientandroidlib.entity.AbstractHttpEntity;
import ch.boye.httpclientandroidlib.entity.ByteArrayEntity;







public class BoundedByteArrayEntity extends AbstractHttpEntity implements
    Cloneable {
  protected final byte[] content;
  protected final int    start;
  protected final int    end;
  protected final int    length;

  








  public BoundedByteArrayEntity(final byte[] b, final int start, final int end) {
    if (b == null) {
      throw new IllegalArgumentException("Source byte array may not be null.");
    }

    if (end < start ||
        start < 0   ||
        end   < 0   ||
        start > b.length ||
        end   > b.length) {
      throw new IllegalArgumentException("Bounds out of range.");
    }
    this.content = b;
    this.start = start;
    this.end = end;
    this.length = end - start;
  }

  @Override
  public boolean isRepeatable() {
    return true;
  }

  @Override
  public long getContentLength() {
    return this.length;
  }

  @Override
  public InputStream getContent() {
    return new ByteArrayInputStream(this.content, this.start, this.length);
  }

  @Override
  public void writeTo(final OutputStream outstream) throws IOException {
    if (outstream == null) {
      throw new IllegalArgumentException("Output stream may not be null.");
    }
    outstream.write(this.content);
    outstream.flush();
  }

  @Override
  public boolean isStreaming() {
    return false;
  }

  @Override
  public Object clone() throws CloneNotSupportedException {
    return super.clone();
  }
}