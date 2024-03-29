














package com.squareup.picasso;

import android.annotation.TargetApi;
import android.app.ActivityManager;
import android.content.ContentResolver;
import android.content.Context;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.os.Looper;
import android.os.Process;
import android.os.StatFs;
import android.provider.Settings;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.util.List;
import java.util.concurrent.ThreadFactory;

import static android.content.Context.ACTIVITY_SERVICE;
import static android.content.pm.ApplicationInfo.FLAG_LARGE_HEAP;
import static android.os.Build.VERSION.SDK_INT;
import static android.os.Build.VERSION_CODES.HONEYCOMB;
import static android.os.Build.VERSION_CODES.HONEYCOMB_MR1;
import static android.os.Process.THREAD_PRIORITY_BACKGROUND;
import static android.provider.Settings.System.AIRPLANE_MODE_ON;

final class Utils {
  static final String THREAD_PREFIX = "Picasso-";
  static final String THREAD_IDLE_NAME = THREAD_PREFIX + "Idle";
  static final int DEFAULT_READ_TIMEOUT = 20 * 1000; 
  static final int DEFAULT_CONNECT_TIMEOUT = 15 * 1000; 
  private static final String PICASSO_CACHE = "picasso-cache";
  private static final int KEY_PADDING = 50; 
  private static final int MIN_DISK_CACHE_SIZE = 5 * 1024 * 1024; 
  private static final int MAX_DISK_CACHE_SIZE = 50 * 1024 * 1024; 

  










  private static final int WEBP_FILE_HEADER_SIZE = 12;
  private static final String WEBP_FILE_HEADER_RIFF = "RIFF";
  private static final String WEBP_FILE_HEADER_WEBP = "WEBP";

  private Utils() {
    
  }

  static int getBitmapBytes(Bitmap bitmap) {
    int result;
    if (SDK_INT >= HONEYCOMB_MR1) {
      result = BitmapHoneycombMR1.getByteCount(bitmap);
    } else {
      result = bitmap.getRowBytes() * bitmap.getHeight();
    }
    if (result < 0) {
      throw new IllegalStateException("Negative size: " + bitmap);
    }
    return result;
  }

  static void checkNotMain() {
    if (Looper.getMainLooper().getThread() == Thread.currentThread()) {
      throw new IllegalStateException("Method call should not happen from the main thread.");
    }
  }

  static String createKey(Request data) {
    StringBuilder builder;

    if (data.uri != null) {
      String path = data.uri.toString();
      builder = new StringBuilder(path.length() + KEY_PADDING);
      builder.append(path);
    } else {
      builder = new StringBuilder(KEY_PADDING);
      builder.append(data.resourceId);
    }
    builder.append('\n');

    if (data.rotationDegrees != 0) {
      builder.append("rotation:").append(data.rotationDegrees);
      if (data.hasRotationPivot) {
        builder.append('@').append(data.rotationPivotX).append('x').append(data.rotationPivotY);
      }
      builder.append('\n');
    }
    if (data.targetWidth != 0) {
      builder.append("resize:").append(data.targetWidth).append('x').append(data.targetHeight);
      builder.append('\n');
    }
    if (data.centerCrop) {
      builder.append("centerCrop\n");
    } else if (data.centerInside) {
      builder.append("centerInside\n");
    }

    if (data.transformations != null) {
      
      for (int i = 0, count = data.transformations.size(); i < count; i++) {
        builder.append(data.transformations.get(i).key());
        builder.append('\n');
      }
    }

    return builder.toString();
  }

  static void closeQuietly(InputStream is) {
    if (is == null) return;
    try {
      is.close();
    } catch (IOException ignored) {
    }
  }

  
  static boolean parseResponseSourceHeader(String header) {
    if (header == null) {
      return false;
    }
    String[] parts = header.split(" ", 2);
    if ("CACHE".equals(parts[0])) {
      return true;
    }
    if (parts.length == 1) {
      return false;
    }
    try {
      return "CONDITIONAL_CACHE".equals(parts[0]) && Integer.parseInt(parts[1]) == 304;
    } catch (NumberFormatException e) {
      return false;
    }
  }

  static Downloader createDefaultDownloader(Context context) {
    return new UrlConnectionDownloader(context);
  }

  static File createDefaultCacheDir(Context context) {
    File cache = new File(context.getApplicationContext().getCacheDir(), PICASSO_CACHE);
    if (!cache.exists()) {
      cache.mkdirs();
    }
    return cache;
  }

  static long calculateDiskCacheSize(File dir) {
    long size = MIN_DISK_CACHE_SIZE;

    try {
      StatFs statFs = new StatFs(dir.getAbsolutePath());
      long available = ((long) statFs.getBlockCount()) * statFs.getBlockSize();
      
      size = available / 50;
    } catch (IllegalArgumentException ignored) {
    }

    
    return Math.max(Math.min(size, MAX_DISK_CACHE_SIZE), MIN_DISK_CACHE_SIZE);
  }

  static int calculateMemoryCacheSize(Context context) {
    ActivityManager am = (ActivityManager) context.getSystemService(ACTIVITY_SERVICE);
    boolean largeHeap = (context.getApplicationInfo().flags & FLAG_LARGE_HEAP) != 0;
    int memoryClass = am.getMemoryClass();
    if (largeHeap && SDK_INT >= HONEYCOMB) {
      memoryClass = ActivityManagerHoneycomb.getLargeMemoryClass(am);
    }
    
    return 1024 * 1024 * memoryClass / 7;
  }

  static boolean isAirplaneModeOn(Context context) {
    ContentResolver contentResolver = context.getContentResolver();
    return Settings.System.getInt(contentResolver, AIRPLANE_MODE_ON, 0) != 0;
  }

  static boolean hasPermission(Context context, String permission) {
    return context.checkCallingOrSelfPermission(permission) == PackageManager.PERMISSION_GRANTED;
  }

  static byte[] toByteArray(InputStream input) throws IOException {
    ByteArrayOutputStream byteArrayOutputStream = new ByteArrayOutputStream();
    byte[] buffer = new byte[1024 * 4];
    int n = 0;
    while (-1 != (n = input.read(buffer))) {
      byteArrayOutputStream.write(buffer, 0, n);
    }
    return byteArrayOutputStream.toByteArray();
  }

  static boolean isWebPFile(InputStream stream) throws IOException {
    byte[] fileHeaderBytes = new byte[WEBP_FILE_HEADER_SIZE];
    boolean isWebPFile = false;
    if (stream.read(fileHeaderBytes, 0, WEBP_FILE_HEADER_SIZE) == WEBP_FILE_HEADER_SIZE) {
      
      isWebPFile = WEBP_FILE_HEADER_RIFF.equals(new String(fileHeaderBytes, 0, 4, "US-ASCII"))
          && WEBP_FILE_HEADER_WEBP.equals(new String(fileHeaderBytes, 8, 4, "US-ASCII"));
    }
    return isWebPFile;
  }

  static int getResourceId(Resources resources, Request data) throws FileNotFoundException {
    if (data.resourceId != 0 || data.uri == null) {
      return data.resourceId;
    }

    String pkg = data.uri.getAuthority();
    if (pkg == null) throw new FileNotFoundException("No package provided: " + data.uri);

    int id;
    List<String> segments = data.uri.getPathSegments();
    if (segments == null || segments.isEmpty()) {
      throw new FileNotFoundException("No path segments: " + data.uri);
    } else if (segments.size() == 1) {
      try {
        id = Integer.parseInt(segments.get(0));
      } catch (NumberFormatException e) {
        throw new FileNotFoundException("Last path segment is not a resource ID: " + data.uri);
      }
    } else if (segments.size() == 2) {
      String type = segments.get(0);
      String name = segments.get(1);

      id = resources.getIdentifier(name, type, pkg);
    } else {
      throw new FileNotFoundException("More than two path segments: " + data.uri);
    }
    return id;
  }

  static Resources getResources(Context context, Request data) throws FileNotFoundException {
    if (data.resourceId != 0 || data.uri == null) {
      return context.getResources();
    }

    String pkg = data.uri.getAuthority();
    if (pkg == null) throw new FileNotFoundException("No package provided: " + data.uri);
    try {
      PackageManager pm = context.getPackageManager();
      return pm.getResourcesForApplication(pkg);
    } catch (PackageManager.NameNotFoundException e) {
      throw new FileNotFoundException("Unable to obtain resources for package: " + data.uri);
    }
  }

  @TargetApi(HONEYCOMB)
  private static class ActivityManagerHoneycomb {
    static int getLargeMemoryClass(ActivityManager activityManager) {
      return activityManager.getLargeMemoryClass();
    }
  }

  static class PicassoThreadFactory implements ThreadFactory {
    @SuppressWarnings("NullableProblems")
    public Thread newThread(Runnable r) {
      return new PicassoThread(r);
    }
  }

  private static class PicassoThread extends Thread {
    public PicassoThread(Runnable r) {
      super(r);
    }

    @Override public void run() {
      Process.setThreadPriority(THREAD_PRIORITY_BACKGROUND);
      super.run();
    }
  }

  @TargetApi(HONEYCOMB_MR1)
  private static class BitmapHoneycombMR1 {
    static int getByteCount(Bitmap bitmap) {
      return bitmap.getByteCount();
    }
  }
}
