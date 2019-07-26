














package com.squareup.picasso;

import java.io.BufferedInputStream;
import java.io.IOException;
import java.io.InputStream;






final class MarkableInputStream extends InputStream {
  private final InputStream in;

  private long offset;
  private long reset;
  private long limit;

  private long defaultMark = -1;

  public MarkableInputStream(InputStream in) {
    if (!in.markSupported()) {
      in = new BufferedInputStream(in);
    }
    this.in = in;
  }

  
  @Override public void mark(int readLimit) {
    defaultMark = savePosition(readLimit);
  }

  





  public long savePosition(int readLimit) {
    long offsetLimit = offset + readLimit;
    if (limit < offsetLimit) {
      setLimit(offsetLimit);
    }
    return offset;
  }

  







  private void setLimit(long limit) {
    try {
      if (reset < offset && offset <= this.limit) {
        in.reset();
        in.mark((int) (limit - reset));
        skip(reset, offset);
      } else {
        reset = offset;
        in.mark((int) (limit - offset));
      }
      this.limit = limit;
    } catch (IOException e) {
      throw new IllegalStateException("Unable to mark: " + e);
    }
  }

  
  @Override public void reset() throws IOException {
    reset(defaultMark);
  }

  
  public void reset(long token) throws IOException {
    if (offset > limit || token < reset) {
      throw new IOException("Cannot reset");
    }
    in.reset();
    skip(reset, token);
    offset = token;
  }

  
  private void skip(long current, long target) throws IOException {
    while (current < target) {
      long skipped = in.skip(target - current);
      if (skipped == 0) {
        if (read() == -1) {
          break; 
        } else {
          skipped = 1;
        }
      }
      current += skipped;
    }
  }

  @Override public int read() throws IOException {
    int result = in.read();
    if (result != -1) {
      offset++;
    }
    return result;
  }

  @Override public int read(byte[] buffer) throws IOException {
    int count = in.read(buffer);
    if (count != -1) {
      offset += count;
    }
    return count;
  }

  @Override public int read(byte[] buffer, int offset, int length) throws IOException {
    int count = in.read(buffer, offset, length);
    if (count != -1) {
      this.offset += count;
    }
    return count;
  }

  @Override public long skip(long byteCount) throws IOException {
    long skipped = in.skip(byteCount);
    offset += skipped;
    return skipped;
  }

  @Override public int available() throws IOException {
    return in.available();
  }

  @Override public void close() throws IOException {
    in.close();
  }

  @Override public boolean markSupported() {
    return in.markSupported();
  }
}
