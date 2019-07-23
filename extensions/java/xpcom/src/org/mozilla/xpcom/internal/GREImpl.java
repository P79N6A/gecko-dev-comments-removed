



































package org.mozilla.xpcom.internal;

import java.io.File;

import org.mozilla.xpcom.IAppFileLocProvider;
import org.mozilla.xpcom.IGRE;
import org.mozilla.xpcom.ProfileLock;


public class GREImpl implements IGRE {

  public void initEmbedding(File aLibXULDirectory, File aAppDirectory,
          IAppFileLocProvider aAppDirProvider) {
    initEmbeddingNative(aLibXULDirectory, aAppDirectory, aAppDirProvider);
  }

  public native void initEmbeddingNative(File aLibXULDirectory,
          File aAppDirectory, IAppFileLocProvider aAppDirProvider);

  public native void termEmbedding();

  public native ProfileLock lockProfileDirectory(File aDirectory);

  public native void notifyProfile();

}

