














package com.squareup.picasso;

import android.content.Context;
import android.graphics.Bitmap;
import java.util.LinkedHashMap;
import java.util.Map;


public class LruCache implements Cache {
  final LinkedHashMap<String, Bitmap> map;
  private final int maxSize;

  private int size;
  private int putCount;
  private int evictionCount;
  private int hitCount;
  private int missCount;

  
  public LruCache(Context context) {
    this(Utils.calculateMemoryCacheSize(context));
  }

  
  public LruCache(int maxSize) {
    if (maxSize <= 0) {
      throw new IllegalArgumentException("Max size must be positive.");
    }
    this.maxSize = maxSize;
    this.map = new LinkedHashMap<String, Bitmap>(0, 0.75f, true);
  }

  @Override public Bitmap get(String key) {
    if (key == null) {
      throw new NullPointerException("key == null");
    }

    Bitmap mapValue;
    synchronized (this) {
      mapValue = map.get(key);
      if (mapValue != null) {
        hitCount++;
        return mapValue;
      }
      missCount++;
    }

    return null;
  }

  @Override public void set(String key, Bitmap bitmap) {
    if (key == null || bitmap == null) {
      throw new NullPointerException("key == null || bitmap == null");
    }

    Bitmap previous;
    synchronized (this) {
      putCount++;
      size += Utils.getBitmapBytes(bitmap);
      previous = map.put(key, bitmap);
      if (previous != null) {
        size -= Utils.getBitmapBytes(previous);
      }
    }

    trimToSize(maxSize);
  }

  private void trimToSize(int maxSize) {
    while (true) {
      String key;
      Bitmap value;
      synchronized (this) {
        if (size < 0 || (map.isEmpty() && size != 0)) {
          throw new IllegalStateException(
              getClass().getName() + ".sizeOf() is reporting inconsistent results!");
        }

        if (size <= maxSize || map.isEmpty()) {
          break;
        }

        Map.Entry<String, Bitmap> toEvict = map.entrySet().iterator().next();
        key = toEvict.getKey();
        value = toEvict.getValue();
        map.remove(key);
        size -= Utils.getBitmapBytes(value);
        evictionCount++;
      }
    }
  }

  
  public final void evictAll() {
    trimToSize(-1); 
  }

  
  public final synchronized int size() {
    return size;
  }

  
  public final synchronized int maxSize() {
    return maxSize;
  }

  public final synchronized void clear() {
    evictAll();
  }

  
  public final synchronized int hitCount() {
    return hitCount;
  }

  
  public final synchronized int missCount() {
    return missCount;
  }

  
  public final synchronized int putCount() {
    return putCount;
  }

  
  public final synchronized int evictionCount() {
    return evictionCount;
  }
}
