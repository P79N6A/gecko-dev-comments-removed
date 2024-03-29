



package org.mozilla.gecko.background.healthreport.upload;

import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.net.URISyntaxException;
import java.util.ArrayList;
import java.util.Collection;

import org.json.JSONException;
import org.json.JSONObject;
import org.mozilla.gecko.BrowserLocaleManager;
import org.mozilla.gecko.background.bagheera.BagheeraClient;
import org.mozilla.gecko.background.bagheera.BagheeraRequestDelegate;
import org.mozilla.gecko.background.common.GlobalConstants;
import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.background.healthreport.AndroidConfigurationProvider;
import org.mozilla.gecko.background.healthreport.Environment;
import org.mozilla.gecko.background.healthreport.EnvironmentBuilder;
import org.mozilla.gecko.background.healthreport.EnvironmentBuilder.ConfigurationProvider;
import org.mozilla.gecko.background.healthreport.HealthReportConstants;
import org.mozilla.gecko.background.healthreport.HealthReportDatabaseStorage;
import org.mozilla.gecko.background.healthreport.HealthReportGenerator;
import org.mozilla.gecko.background.healthreport.HealthReportStorage;
import org.mozilla.gecko.background.healthreport.HealthReportStorage.Field;
import org.mozilla.gecko.background.healthreport.HealthReportStorage.MeasurementFields;
import org.mozilla.gecko.background.healthreport.ProfileInformationCache;
import org.mozilla.gecko.sync.net.BaseResource;

import android.content.ContentProviderClient;
import android.content.Context;
import android.content.SharedPreferences;
import ch.boye.httpclientandroidlib.HttpResponse;

public class AndroidSubmissionClient implements SubmissionClient {
  protected static final String LOG_TAG = AndroidSubmissionClient.class.getSimpleName();

  private static final String MEASUREMENT_NAME_SUBMISSIONS = "org.mozilla.healthreport.submissions";
  private static final int MEASUREMENT_VERSION_SUBMISSIONS = 1;

  protected final Context context;
  protected final SharedPreferences sharedPreferences;
  protected final String profilePath;
  protected final ConfigurationProvider config;

  public AndroidSubmissionClient(Context context, SharedPreferences sharedPreferences, String profilePath) {
    this(context, sharedPreferences, profilePath, new AndroidConfigurationProvider(context));
  }

  public AndroidSubmissionClient(Context context, SharedPreferences sharedPreferences, String profilePath, ConfigurationProvider config) {
    this.context = context;
    this.sharedPreferences = sharedPreferences;
    this.profilePath = profilePath;
    this.config = config;
  }

  public SharedPreferences getSharedPreferences() {
    return sharedPreferences;
  }

  public String getDocumentServerURI() {
    return getSharedPreferences().getString(HealthReportConstants.PREF_DOCUMENT_SERVER_URI, HealthReportConstants.DEFAULT_DOCUMENT_SERVER_URI);
  }

  public String getDocumentServerNamespace() {
    return getSharedPreferences().getString(HealthReportConstants.PREF_DOCUMENT_SERVER_NAMESPACE, HealthReportConstants.DEFAULT_DOCUMENT_SERVER_NAMESPACE);
  }

  public long getLastUploadLocalTime() {
    return getSharedPreferences().getLong(HealthReportConstants.PREF_LAST_UPLOAD_LOCAL_TIME, 0L);
  }

  public String getLastUploadDocumentId() {
    return getSharedPreferences().getString(HealthReportConstants.PREF_LAST_UPLOAD_DOCUMENT_ID, null);
  }

  public boolean hasUploadBeenRequested() {
    return getSharedPreferences().contains(HealthReportConstants.PREF_LAST_UPLOAD_REQUESTED);
  }

  public void setLastUploadLocalTimeAndDocumentId(long localTime, String id) {
    getSharedPreferences().edit()
      .putLong(HealthReportConstants.PREF_LAST_UPLOAD_LOCAL_TIME, localTime)
      .putString(HealthReportConstants.PREF_LAST_UPLOAD_DOCUMENT_ID, id)
      .commit();
  }

  protected HealthReportDatabaseStorage getStorage(final ContentProviderClient client) {
    return EnvironmentBuilder.getStorage(client, profilePath);
  }

  protected JSONObject generateDocument(final long localTime, final long last,
      final SubmissionsTracker tracker) throws JSONException {
    final long since = localTime - GlobalConstants.MILLISECONDS_PER_SIX_MONTHS;
    final HealthReportGenerator generator = tracker.getGenerator();
    return generator.generateDocument(since, last, profilePath, config);
  }

  protected void uploadPayload(String id, String payload, Collection<String> oldIds, BagheeraRequestDelegate uploadDelegate) {
    final BagheeraClient client = new BagheeraClient(getDocumentServerURI());

    Logger.pii(LOG_TAG, "New health report has id " + id +
        "and obsoletes " + (oldIds != null ? Integer.toString(oldIds.size()) : "no") + " old ids.");

    try {
      client.uploadJSONDocument(getDocumentServerNamespace(),
          id,
          payload,
          oldIds,
          uploadDelegate);
    } catch (Exception e) {
      uploadDelegate.handleError(e);
    }
  }

  @Override
  public void upload(long localTime, String id, Collection<String> oldIds, Delegate delegate) {
    
    
    
    
    
    
    ContentProviderClient client = EnvironmentBuilder.getContentProviderClient(context);
    if (client == null) {
      
      delegate.onHardFailure(localTime, null, "Could not fetch content provider client.", null);
      return;
    }

    try {
      
      
      
      final HealthReportDatabaseStorage storage = getStorage(client);
      if (storage == null) {
        
        delegate.onHardFailure(localTime, null, "No storage when generating report.", null);
        return;
      }

      long last = Math.max(getLastUploadLocalTime(), HealthReportConstants.EARLIEST_LAST_PING);
      if (!storage.hasEventSince(last)) {
        delegate.onHardFailure(localTime, null, "No new events in storage.", null);
        return;
      }

      initializeStorageForUploadProviders(storage);

      final SubmissionsTracker tracker =
          getSubmissionsTracker(storage, localTime, hasUploadBeenRequested());
      try {
        
        final JSONObject document = generateDocument(localTime, last, tracker);
        if (document == null) {
          delegate.onHardFailure(localTime, null, "Generator returned null document.", null);
          return;
        }

        final BagheeraRequestDelegate uploadDelegate = tracker.getDelegate(delegate, localTime,
            true, id);
        this.uploadPayload(id, document.toString(), oldIds, uploadDelegate);
      } catch (Exception e) {
        
        
        tracker.incrementUploadClientFailureCount();
        throw e;
      }
    } catch (Exception e) {
      
      Logger.warn(LOG_TAG, "Got exception generating document.", e);
      delegate.onHardFailure(localTime, null, "Got exception uploading.", e);
      return;
    } finally {
      client.release();
    }
  }

  protected SubmissionsTracker getSubmissionsTracker(final HealthReportStorage storage,
      final long localTime, final boolean hasUploadBeenRequested) {
    return new SubmissionsTracker(storage, localTime, hasUploadBeenRequested);
  }

  @Override
  public void delete(final long localTime, final String id, Delegate delegate) {
    final BagheeraClient client = new BagheeraClient(getDocumentServerURI());

    Logger.pii(LOG_TAG, "Deleting health report with id " + id + ".");

    BagheeraRequestDelegate deleteDelegate = new RequestDelegate(delegate, localTime, false, id);
    try {
      client.deleteDocument(getDocumentServerNamespace(), id, deleteDelegate);
    } catch (Exception e) {
      deleteDelegate.handleError(e);
    }
  }

  protected class RequestDelegate implements BagheeraRequestDelegate {
    protected final Delegate delegate;
    protected final boolean isUpload;
    protected final String methodString;
    protected final long localTime;
    protected final String id;

    public RequestDelegate(Delegate delegate, long localTime, boolean isUpload, String id) {
      this.delegate = delegate;
      this.localTime = localTime;
      this.isUpload = isUpload;
      this.methodString = this.isUpload ? "upload" : "delete";
      this.id = id;
    }

    @Override
    public String getUserAgent() {
      return HealthReportConstants.USER_AGENT;
    }

    @Override
    public void handleSuccess(int status, String namespace, String id, HttpResponse response) {
      BaseResource.consumeEntity(response);
      if (isUpload) {
        setLastUploadLocalTimeAndDocumentId(localTime, id);
      }
      Logger.debug(LOG_TAG, "Successful " + methodString + " at " + localTime + ".");
      delegate.onSuccess(localTime, id);
    }

    








    @Override
    public void handleFailure(int status, String namespace, HttpResponse response) {
      BaseResource.consumeEntity(response);
      Logger.debug(LOG_TAG, "Failed " + methodString + " at " + localTime + ".");
      if (status >= 500) {
        delegate.onSoftFailure(localTime, id, "Got status " + status + " from server.", null);
        return;
      }
      
      
      
      delegate.onHardFailure(localTime, id, "Got status " + status + " from server.", null);
    }

    @Override
    public void handleError(Exception e) {
      Logger.debug(LOG_TAG, "Exception during " + methodString + " at " + localTime + ".", e);
      if (e instanceof IOException) {
        
        delegate.onSoftFailure(localTime, id, "Got exception during " + methodString + ".", e);
        return;
      }
      delegate.onHardFailure(localTime, id, "Got exception during " + methodString + ".", e);
    }
  };

  private void initializeStorageForUploadProviders(HealthReportDatabaseStorage storage) {
    storage.beginInitialization();
    try {
      initializeSubmissionsProvider(storage);
      storage.finishInitialization();
    } catch (Exception e) {
      
      storage.abortInitialization();
      throw new IllegalStateException("Could not initialize storage for upload provider.", e);
    }
  }

  private void initializeSubmissionsProvider(HealthReportDatabaseStorage storage) {
    storage.ensureMeasurementInitialized(
        MEASUREMENT_NAME_SUBMISSIONS,
        MEASUREMENT_VERSION_SUBMISSIONS,
        new MeasurementFields() {
          @Override
          public Iterable<FieldSpec> getFields() {
            final ArrayList<FieldSpec> out = new ArrayList<FieldSpec>();
            for (SubmissionsFieldName fieldName : SubmissionsFieldName.values()) {
              FieldSpec spec = new FieldSpec(fieldName.getName(), Field.TYPE_INTEGER_COUNTER);
              out.add(spec);
            }
            return out;
          }
        });
  }

  public static enum SubmissionsFieldName {
    FIRST_ATTEMPT("firstDocumentUploadAttempt"),
    CONTINUATION_ATTEMPT("continuationDocumentUploadAttempt"),
    SUCCESS("uploadSuccess"),
    TRANSPORT_FAILURE("uploadTransportFailure"),
    SERVER_FAILURE("uploadServerFailure"),
    CLIENT_FAILURE("uploadClientFailure");

    private final String name;

    SubmissionsFieldName(String name) {
      this.name = name;
    }

    public String getName() {
      return name;
    }

    public int getID(HealthReportStorage storage) {
      final Field field = storage.getField(MEASUREMENT_NAME_SUBMISSIONS,
                                           MEASUREMENT_VERSION_SUBMISSIONS,
                                           name);
      return field.getID();
    }
  }

  



  public class SubmissionsTracker {
    private final HealthReportStorage storage;
    private final ProfileInformationCache profileCache;
    private final int day;
    private final int envID;

    private boolean isUploadStatusCountIncremented;

    public SubmissionsTracker(final HealthReportStorage storage, final long localTime,
        final boolean hasUploadBeenRequested) throws IllegalStateException {
      this.storage = storage;
      this.profileCache = getProfileInformationCache();
      this.day = storage.getDay(localTime);
      this.envID = registerCurrentEnvironment();

      this.isUploadStatusCountIncremented = false;

      if (!hasUploadBeenRequested) {
        incrementFirstUploadAttemptCount();
      } else {
        incrementContinuationAttemptCount();
      }
    }

    protected ProfileInformationCache getProfileInformationCache() {
      final ProfileInformationCache profileCache = new ProfileInformationCache(profilePath);
      if (!profileCache.restoreUnlessInitialized()) {
        Logger.warn(LOG_TAG, "Not enough profile information to compute current environment.");
        throw new IllegalStateException("Could not retrieve current environment.");
      }
      return profileCache;
    }

    protected int registerCurrentEnvironment() {
      return EnvironmentBuilder.registerCurrentEnvironment(storage, profileCache, config);
    }

    protected void incrementFirstUploadAttemptCount() {
      Logger.debug(LOG_TAG, "Incrementing first upload attempt field.");
      storage.incrementDailyCount(envID, day, SubmissionsFieldName.FIRST_ATTEMPT.getID(storage));
    }

    protected void incrementContinuationAttemptCount() {
      Logger.debug(LOG_TAG, "Incrementing continuation upload attempt field.");
      storage.incrementDailyCount(envID, day, SubmissionsFieldName.CONTINUATION_ATTEMPT.getID(storage));
    }

    public void incrementUploadSuccessCount() {
      incrementStatusCount(SubmissionsFieldName.SUCCESS.getID(storage), "success");
    }

    public void incrementUploadClientFailureCount() {
      incrementStatusCount(SubmissionsFieldName.CLIENT_FAILURE.getID(storage), "client failure");
    }

    public void incrementUploadTransportFailureCount() {
      incrementStatusCount(SubmissionsFieldName.TRANSPORT_FAILURE.getID(storage), "transport failure");
    }

    public void incrementUploadServerFailureCount() {
      incrementStatusCount(SubmissionsFieldName.SERVER_FAILURE.getID(storage), "server failure");
    }

    private void incrementStatusCount(final int fieldID, final String countType) {
      if (!isUploadStatusCountIncremented) {
        Logger.debug(LOG_TAG, "Incrementing upload attempt " + countType + " count.");
        storage.incrementDailyCount(envID, day, fieldID);
        isUploadStatusCountIncremented = true;
      } else {
        Logger.warn(LOG_TAG, "Upload status count already incremented - not incrementing " +
            countType + " count.");
      }
    }

    public TrackingGenerator getGenerator() {
      return new TrackingGenerator();
    }

    public class TrackingGenerator extends HealthReportGenerator {
      public TrackingGenerator() {
        super(storage);
      }

      @Override
      public JSONObject generateDocument(long since, long lastPingTime,
          String generationProfilePath, ConfigurationProvider providedConfig) throws JSONException {

        
        BrowserLocaleManager.getInstance().getAndApplyPersistedLocale(context);

        final JSONObject document;
        
        if (profilePath != null && profilePath.equals(generationProfilePath)) {
          final Environment environment = getCurrentEnvironment();
          document = super.generateDocument(since, lastPingTime, environment);
        } else {
          document = super.generateDocument(since, lastPingTime, generationProfilePath, providedConfig);
        }

        if (document == null) {
          incrementUploadClientFailureCount();
        }
        return document;
      }

      protected Environment getCurrentEnvironment() {
        return EnvironmentBuilder.getCurrentEnvironment(profileCache, config);
      }
    }

    public TrackingRequestDelegate getDelegate(final Delegate delegate, final long localTime,
        final boolean isUpload, final String id) {
      return new TrackingRequestDelegate(delegate, localTime, isUpload, id);
    }

    public class TrackingRequestDelegate extends RequestDelegate {
      public TrackingRequestDelegate(final Delegate delegate, final long localTime,
          final boolean isUpload, final String id) {
        super(delegate, localTime, isUpload, id);
      }

      @Override
      public void handleSuccess(int status, String namespace, String id, HttpResponse response) {
        super.handleSuccess(status, namespace, id, response);
        incrementUploadSuccessCount();
      }

      @Override
      public void handleFailure(int status, String namespace, HttpResponse response) {
        super.handleFailure(status, namespace, response);
        incrementUploadServerFailureCount();
      }

      @Override
      public void handleError(Exception e) {
        super.handleError(e);
        if (e instanceof IllegalArgumentException ||
            e instanceof UnsupportedEncodingException ||
            e instanceof URISyntaxException) {
          incrementUploadClientFailureCount();
        } else {
          incrementUploadTransportFailureCount();
        }
      }
    }
  }
}
