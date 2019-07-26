














package com.squareup.picasso;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.net.Uri;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.Process;
import android.widget.ImageView;
import java.io.File;
import java.lang.ref.ReferenceQueue;
import java.util.List;
import java.util.Map;
import java.util.WeakHashMap;
import java.util.concurrent.ExecutorService;

import static android.os.Process.THREAD_PRIORITY_BACKGROUND;
import static com.squareup.picasso.Action.RequestWeakReference;
import static com.squareup.picasso.Dispatcher.HUNTER_BATCH_COMPLETE;
import static com.squareup.picasso.Dispatcher.REQUEST_GCED;
import static com.squareup.picasso.Utils.THREAD_PREFIX;







public class Picasso {

  
  public interface Listener {
    



    void onImageLoadFailed(Picasso picasso, Uri uri, Exception exception);
  }

  









  public interface RequestTransformer {
    




    Request transformRequest(Request request);

    
    RequestTransformer IDENTITY = new RequestTransformer() {
      @Override public Request transformRequest(Request request) {
        return request;
      }
    };
  }

  static final Handler HANDLER = new Handler(Looper.getMainLooper()) {
    @Override public void handleMessage(Message msg) {
      switch (msg.what) {
        case HUNTER_BATCH_COMPLETE: {
          @SuppressWarnings("unchecked") List<BitmapHunter> batch = (List<BitmapHunter>) msg.obj;
          for (BitmapHunter hunter : batch) {
            hunter.picasso.complete(hunter);
          }
          break;
        }
        case REQUEST_GCED: {
          Action action = (Action) msg.obj;
          action.picasso.cancelExistingRequest(action.getTarget());
          break;
        }
        default:
          throw new AssertionError("Unknown handler message received: " + msg.what);
      }
    }
  };

  static Picasso singleton = null;

  private final Listener listener;
  private final RequestTransformer requestTransformer;
  private final CleanupThread cleanupThread;

  final Context context;
  final Dispatcher dispatcher;
  final Cache cache;
  final Stats stats;
  final Map<Object, Action> targetToAction;
  final Map<ImageView, DeferredRequestCreator> targetToDeferredRequestCreator;
  final ReferenceQueue<Object> referenceQueue;

  boolean debugging;
  boolean shutdown;

  Picasso(Context context, Dispatcher dispatcher, Cache cache, Listener listener,
      RequestTransformer requestTransformer, Stats stats, boolean debugging) {
    this.context = context;
    this.dispatcher = dispatcher;
    this.cache = cache;
    this.listener = listener;
    this.requestTransformer = requestTransformer;
    this.stats = stats;
    this.targetToAction = new WeakHashMap<Object, Action>();
    this.targetToDeferredRequestCreator = new WeakHashMap<ImageView, DeferredRequestCreator>();
    this.debugging = debugging;
    this.referenceQueue = new ReferenceQueue<Object>();
    this.cleanupThread = new CleanupThread(referenceQueue, HANDLER);
    this.cleanupThread.start();
  }

  
  public void cancelRequest(ImageView view) {
    cancelExistingRequest(view);
  }

  
  public void cancelRequest(Target target) {
    cancelExistingRequest(target);
  }

  









  public RequestCreator load(Uri uri) {
    return new RequestCreator(this, uri, 0);
  }

  














  public RequestCreator load(String path) {
    if (path == null) {
      return new RequestCreator(this, null, 0);
    }
    if (path.trim().length() == 0) {
      throw new IllegalArgumentException("Path must not be empty.");
    }
    return load(Uri.parse(path));
  }

  










  public RequestCreator load(File file) {
    if (file == null) {
      return new RequestCreator(this, null, 0);
    }
    return load(Uri.fromFile(file));
  }

  






  public RequestCreator load(int resourceId) {
    if (resourceId == 0) {
      throw new IllegalArgumentException("Resource ID must not be zero.");
    }
    return new RequestCreator(this, null, resourceId);
  }

  
  @SuppressWarnings("UnusedDeclaration") public boolean isDebugging() {
    return debugging;
  }

  
  @SuppressWarnings("UnusedDeclaration") public void setDebugging(boolean debugging) {
    this.debugging = debugging;
  }

  
  @SuppressWarnings("UnusedDeclaration") public StatsSnapshot getSnapshot() {
    return stats.createSnapshot();
  }

  
  public void shutdown() {
    if (this == singleton) {
      throw new UnsupportedOperationException("Default singleton instance cannot be shutdown.");
    }
    if (shutdown) {
      return;
    }
    cache.clear();
    cleanupThread.shutdown();
    stats.shutdown();
    dispatcher.shutdown();
    for (DeferredRequestCreator deferredRequestCreator : targetToDeferredRequestCreator.values()) {
      deferredRequestCreator.cancel();
    }
    targetToDeferredRequestCreator.clear();
    shutdown = true;
  }

  Request transformRequest(Request request) {
    Request transformed = requestTransformer.transformRequest(request);
    if (transformed == null) {
      throw new IllegalStateException("Request transformer "
          + requestTransformer.getClass().getCanonicalName()
          + " returned null for "
          + request);
    }
    return transformed;
  }

  void defer(ImageView view, DeferredRequestCreator request) {
    targetToDeferredRequestCreator.put(view, request);
  }

  void enqueueAndSubmit(Action action) {
    Object target = action.getTarget();
    if (target != null) {
      cancelExistingRequest(target);
      targetToAction.put(target, action);
    }
    submit(action);
  }

  void submit(Action action) {
    dispatcher.dispatchSubmit(action);
  }

  Bitmap quickMemoryCacheCheck(String key) {
    Bitmap cached = cache.get(key);
    if (cached != null) {
      stats.dispatchCacheHit();
    } else {
      stats.dispatchCacheMiss();
    }
    return cached;
  }

  void complete(BitmapHunter hunter) {
    List<Action> joined = hunter.getActions();
    if (joined.isEmpty()) {
      return;
    }

    Uri uri = hunter.getData().uri;
    Exception exception = hunter.getException();
    Bitmap result = hunter.getResult();
    LoadedFrom from = hunter.getLoadedFrom();

    for (Action join : joined) {
      if (join.isCancelled()) {
        continue;
      }
      targetToAction.remove(join.getTarget());
      if (result != null) {
        if (from == null) {
          throw new AssertionError("LoadedFrom cannot be null.");
        }
        join.complete(result, from);
      } else {
        join.error();
      }
    }

    if (listener != null && exception != null) {
      listener.onImageLoadFailed(this, uri, exception);
    }
  }

  private void cancelExistingRequest(Object target) {
    Action action = targetToAction.remove(target);
    if (action != null) {
      action.cancel();
      dispatcher.dispatchCancel(action);
    }
    if (target instanceof ImageView) {
      ImageView targetImageView = (ImageView) target;
      DeferredRequestCreator deferredRequestCreator =
          targetToDeferredRequestCreator.remove(targetImageView);
      if (deferredRequestCreator != null) {
        deferredRequestCreator.cancel();
      }
    }
  }

  private static class CleanupThread extends Thread {
    private final ReferenceQueue<?> referenceQueue;
    private final Handler handler;

    CleanupThread(ReferenceQueue<?> referenceQueue, Handler handler) {
      this.referenceQueue = referenceQueue;
      this.handler = handler;
      setDaemon(true);
      setName(THREAD_PREFIX + "refQueue");
    }

    @Override public void run() {
      Process.setThreadPriority(THREAD_PRIORITY_BACKGROUND);
      while (true) {
        try {
          RequestWeakReference<?> remove = (RequestWeakReference<?>) referenceQueue.remove();
          handler.sendMessage(handler.obtainMessage(REQUEST_GCED, remove.action));
        } catch (InterruptedException e) {
          break;
        } catch (final Exception e) {
          handler.post(new Runnable() {
            @Override public void run() {
              throw new RuntimeException(e);
            }
          });
          break;
        }
      }
    }

    void shutdown() {
      interrupt();
    }
  }

  















  public static Picasso with(Context context) {
    if (singleton == null) {
      singleton = new Builder(context).build();
    }
    return singleton;
  }

  
  @SuppressWarnings("UnusedDeclaration") 
  public static class Builder {
    private final Context context;
    private Downloader downloader;
    private ExecutorService service;
    private Cache cache;
    private Listener listener;
    private RequestTransformer transformer;
    private boolean debugging;

    
    public Builder(Context context) {
      if (context == null) {
        throw new IllegalArgumentException("Context must not be null.");
      }
      this.context = context.getApplicationContext();
    }

    
    public Builder downloader(Downloader downloader) {
      if (downloader == null) {
        throw new IllegalArgumentException("Downloader must not be null.");
      }
      if (this.downloader != null) {
        throw new IllegalStateException("Downloader already set.");
      }
      this.downloader = downloader;
      return this;
    }

    
    public Builder executor(ExecutorService executorService) {
      if (executorService == null) {
        throw new IllegalArgumentException("Executor service must not be null.");
      }
      if (this.service != null) {
        throw new IllegalStateException("Executor service already set.");
      }
      this.service = executorService;
      return this;
    }

    
    public Builder memoryCache(Cache memoryCache) {
      if (memoryCache == null) {
        throw new IllegalArgumentException("Memory cache must not be null.");
      }
      if (this.cache != null) {
        throw new IllegalStateException("Memory cache already set.");
      }
      this.cache = memoryCache;
      return this;
    }

    
    public Builder listener(Listener listener) {
      if (listener == null) {
        throw new IllegalArgumentException("Listener must not be null.");
      }
      if (this.listener != null) {
        throw new IllegalStateException("Listener already set.");
      }
      this.listener = listener;
      return this;
    }

    





    public Builder requestTransformer(RequestTransformer transformer) {
      if (transformer == null) {
        throw new IllegalArgumentException("Transformer must not be null.");
      }
      if (this.transformer != null) {
        throw new IllegalStateException("Transformer already set.");
      }
      this.transformer = transformer;
      return this;
    }

    
    public Builder debugging(boolean debugging) {
      this.debugging = debugging;
      return this;
    }

    
    public Picasso build() {
      Context context = this.context;

      if (downloader == null) {
        downloader = Utils.createDefaultDownloader(context);
      }
      if (cache == null) {
        cache = new LruCache(context);
      }
      if (service == null) {
        service = new PicassoExecutorService();
      }
      if (transformer == null) {
        transformer = RequestTransformer.IDENTITY;
      }

      Stats stats = new Stats(cache);

      Dispatcher dispatcher = new Dispatcher(context, service, HANDLER, downloader, cache, stats);

      return new Picasso(context, dispatcher, cache, listener, transformer, stats, debugging);
    }
  }

  
  public enum LoadedFrom {
    MEMORY(Color.GREEN),
    DISK(Color.YELLOW),
    NETWORK(Color.RED);

    final int debugColor;

    private LoadedFrom(int debugColor) {
      this.debugColor = debugColor;
    }
  }
}
