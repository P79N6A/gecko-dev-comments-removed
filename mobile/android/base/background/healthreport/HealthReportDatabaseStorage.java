



package org.mozilla.gecko.background.healthreport;

import java.io.File;
import java.util.Collection;
import java.util.HashMap;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.Executor;
import java.util.concurrent.Executors;

import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.background.healthreport.HealthReportStorage.MeasurementFields.FieldSpec;

import android.content.ContentValues;
import android.content.Context;
import android.content.ContextWrapper;
import android.database.Cursor;
import android.database.SQLException;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.os.Build;
import android.util.SparseArray;


























































































public class HealthReportDatabaseStorage implements HealthReportStorage {

  private static final String WHERE_DATE_AND_ENV_AND_FIELD = "date = ? AND env = ? AND field = ?";

  public static final String[] COLUMNS_HASH = new String[] {"hash"};
  public static final String[] COLUMNS_DATE_ENV_FIELD_VALUE = new String[] {"date", "env", "field", "value"};
  public static final String[] COLUMNS_DATE_ENVSTR_M_MV_F_VALUE = new String[] {
    "date", "environment", "measurement_name", "measurement_version",
    "field_name", "field_flags", "value"
  };

  private static final String[] COLUMNS_ENVIRONMENT_DETAILS = new String[] {
      "id", "hash",
      "profileCreation", "cpuCount", "memoryMB",

      "isBlocklistEnabled", "isTelemetryEnabled", "extensionCount",
      "pluginCount", "themeCount",

      "architecture", "sysName", "sysVersion", "vendor", "appName", "appID",
      "appVersion", "appBuildID", "platformVersion", "platformBuildID", "os",
      "xpcomabi", "updateChannel"
  };

  public static final String[] COLUMNS_MEASUREMENT_DETAILS = new String[] {"id", "name", "version"};
  public static final String[] COLUMNS_MEASUREMENT_AND_FIELD_DETAILS =
      new String[] {"measurement_name", "measurement_id", "measurement_version",
                    "field_name", "field_id", "field_flags"};

  private static final String[] ENVIRONMENT_RECORD_COLUMNS = null;

  private static final String[] COLUMNS_VALUE = new String[] {"value"};
  private static final String[] COLUMNS_ID = new String[] {"id"};

  private static final String EVENTS_TEXTUAL = "events_textual";
  private static final String EVENTS_INTEGER = "events_integer";

  protected static final String DB_NAME = "health.db";

  private static final String LOG_TAG = "HealthReportStorage";

  private final Executor executor = Executors.newSingleThreadExecutor();

  @Override
  public void enqueueOperation(Runnable runnable) {
    executor.execute(runnable);
  }

  public HealthReportDatabaseStorage(final Context context,
                                     final File profileDirectory) {
    this.helper = new HealthReportSQLiteOpenHelper(context, profileDirectory,
                                                   DB_NAME);
    executor.execute(new Runnable() {
      @Override
      public void run() {
        Logger.setThreadLogTag(HealthReportConstants.GLOBAL_LOG_TAG);
        Logger.debug(LOG_TAG, "Creating HealthReportDatabaseStorage.");
      }
    });
  }

  @Override
  public void close() {
    this.helper.close();
    this.fields.clear();
    this.envs.clear();
    this.measurementVersions.clear();
  }

  protected final HealthReportSQLiteOpenHelper helper;

  public static class HealthReportSQLiteOpenHelper extends SQLiteOpenHelper {
    public static final int CURRENT_VERSION = 2;
    public static final String LOG_TAG = "HealthReportSQL";

    




    public static class AbsolutePathContext extends ContextWrapper {
      private final File parent;

      public AbsolutePathContext(Context base, File parent) {
        super(base);
        this.parent = parent;
      }

      @Override
      public File getDatabasePath(String name) {
        return new File(getAbsolutePath(parent, name));
      }

      
      
      
      
      @Override
      public SQLiteDatabase openOrCreateDatabase(String name,
                                                 int mode,
                                                 SQLiteDatabase.CursorFactory factory) {
        final File path = getDatabasePath(name);
        Logger.info(LOG_TAG, "Opening database through absolute path " + path.getAbsolutePath());
        return SQLiteDatabase.openOrCreateDatabase(path, null);
      }
    }

    public static String getAbsolutePath(File parent, String name) {
      return parent.getAbsolutePath() + File.separator + name;
    }

    public static boolean CAN_USE_ABSOLUTE_DB_PATH = (Build.VERSION.SDK_INT >= Build.VERSION_CODES.FROYO);
    public HealthReportSQLiteOpenHelper(Context context, File profileDirectory, String name) {
      super(
          (CAN_USE_ABSOLUTE_DB_PATH ? context : new AbsolutePathContext(context, profileDirectory)),
          (CAN_USE_ABSOLUTE_DB_PATH ? getAbsolutePath(profileDirectory, name) : name),
          null,
          CURRENT_VERSION);

      if (CAN_USE_ABSOLUTE_DB_PATH) {
        Logger.info(LOG_TAG, "Opening: " + getAbsolutePath(profileDirectory, name));
      }
    }

    @Override
    public void onCreate(SQLiteDatabase db) {
      db.beginTransaction();
      try {
        db.execSQL("CREATE TABLE environments (id INTEGER PRIMARY KEY AUTOINCREMENT, " +
                   "                           hash TEXT, " +
                   "                           profileCreation INTEGER, " +
                   "                           cpuCount        INTEGER, " +
                   "                           memoryMB        INTEGER, " +
                   "                           isBlocklistEnabled INTEGER, " +
                   "                           isTelemetryEnabled INTEGER, " +
                   "                           extensionCount     INTEGER, " +
                   "                           pluginCount        INTEGER, " +
                   "                           themeCount         INTEGER, " +
                   "                           architecture    TEXT, " +
                   "                           sysName         TEXT, " +
                   "                           sysVersion      TEXT, " +
                   "                           vendor          TEXT, " +
                   "                           appName         TEXT, " +
                   "                           appID           TEXT, " +
                   "                           appVersion      TEXT, " +
                   "                           appBuildID      TEXT, " +
                   "                           platformVersion TEXT, " +
                   "                           platformBuildID TEXT, " +
                   "                           os              TEXT, " +
                   "                           xpcomabi        TEXT, " +
                   "                           updateChannel   TEXT, " +
                   "                           UNIQUE (hash) " +
                   ")");

        db.execSQL("CREATE TABLE measurements (id INTEGER PRIMARY KEY AUTOINCREMENT, " +
                   "                           name TEXT, " +
                   "                           version INTEGER, " +
                   "                           UNIQUE (name, version) " +
                   ")");

        db.execSQL("CREATE TABLE fields (id INTEGER PRIMARY KEY AUTOINCREMENT, " +
                   "                     measurement INTEGER, " +
                   "                     name TEXT, " +
                   "                     flags INTEGER, " +
                   "                     FOREIGN KEY (measurement) REFERENCES measurements(id) ON DELETE CASCADE, " +
                   "                     UNIQUE (measurement, name)" +
                   ")");

        db.execSQL("CREATE TABLE " + EVENTS_INTEGER + "(" +
                   "                 date  INTEGER, " +
                   "                 env   INTEGER, " +
                   "                 field INTEGER, " +
                   "                 value INTEGER, " +
                   "                 FOREIGN KEY (field) REFERENCES fields(id) ON DELETE CASCADE, " +
                   "                 FOREIGN KEY (env) REFERENCES environments(id) ON DELETE CASCADE" +
                   ")");

        db.execSQL("CREATE TABLE " + EVENTS_TEXTUAL + "(" +
                   "                 date  INTEGER, " +
                   "                 env   INTEGER, " +
                   "                 field INTEGER, " +
                   "                 value TEXT, " +
                   "                 FOREIGN KEY (field) REFERENCES fields(id) ON DELETE CASCADE, " +
                   "                 FOREIGN KEY (env) REFERENCES environments(id) ON DELETE CASCADE" +
                   ")");

        db.execSQL("CREATE INDEX idx_events_integer_date_env_field ON events_integer (date, env, field)");
        db.execSQL("CREATE INDEX idx_events_textual_date_env_field ON events_textual (date, env, field)");

        db.execSQL("CREATE VIEW events AS " +
                   "SELECT date, env, field, value FROM " + EVENTS_INTEGER + " " +
                   "UNION ALL " +
                   "SELECT date, env, field, value FROM " + EVENTS_TEXTUAL);

        db.execSQL("CREATE VIEW named_events AS " +
                   "SELECT date, " +
                   "       environments.hash AS environment, " +
                   "       measurements.name AS measurement_name, " +
                   "       measurements.version AS measurement_version, " +
                   "       fields.name AS field_name, " +
                   "       fields.flags AS field_flags, " +
                   "       value FROM " +
                   "events JOIN environments ON events.env = environments.id " +
                   "       JOIN fields ON events.field = fields.id " +
                   "       JOIN measurements ON fields.measurement = measurements.id");

        db.execSQL("CREATE VIEW named_fields AS " +
                   "SELECT measurements.name AS measurement_name, " +
                   "       measurements.id AS measurement_id, " +
                   "       measurements.version AS measurement_version, " +
                   "       fields.name AS field_name, " +
                   "       fields.id AS field_id, " +
                   "       fields.flags AS field_flags " +
                   "FROM fields JOIN measurements ON fields.measurement = measurements.id");

        db.execSQL("CREATE VIEW current_measurements AS " +
                   "SELECT name, MAX(version) AS version FROM measurements GROUP BY name");

        db.setTransactionSuccessful();
      } finally {
        db.endTransaction();
      }
    }

    @Override
    public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
      Logger.info(LOG_TAG, "onUpgrade: from " + oldVersion + " to " + newVersion + ".");
    }

    public void deleteEverything() {
      final SQLiteDatabase db = this.getWritableDatabase();

      Logger.info(LOG_TAG, "Deleting everything.");
      db.beginTransaction();
      try {
        
        db.delete("measurements", null, null);
        db.delete("environments", null, null);
        db.setTransactionSuccessful();
        Logger.info(LOG_TAG, "Deletion successful.");
      } finally {
        db.endTransaction();
      }
    }
  }

  public class DatabaseField extends Field {
    public DatabaseField(String mName, int mVersion, String fieldName) {
      this(mName, mVersion, fieldName, UNKNOWN_TYPE_OR_FIELD_ID, UNKNOWN_TYPE_OR_FIELD_ID);
    }

    public DatabaseField(String mName, int mVersion, String fieldName, int flags) {
      this(mName, mVersion, fieldName, UNKNOWN_TYPE_OR_FIELD_ID, flags);
    }

    public DatabaseField(String mName, int mVersion, String fieldName, int fieldID, int flags) {
      super(mName, mVersion, fieldName, flags);
      this.fieldID = fieldID;
    }

    private void loadFlags() {
      if (this.flags == UNKNOWN_TYPE_OR_FIELD_ID) {
        if (this.fieldID == UNKNOWN_TYPE_OR_FIELD_ID) {
          this.getID();
        }
        this.flags = integerQuery("fields", "flags", "id = ?", new String[] { Integer.toString(this.fieldID, 10) }, -1);
      }
    }

    @Override
    public synchronized boolean isIntegerField() {
      loadFlags();
      return super.isIntegerField();
    }

    @Override
    public synchronized boolean isStringField() {
      loadFlags();
      return super.isStringField();
    }

    @Override
    public synchronized boolean isDiscreteField() {
      loadFlags();
      return super.isDiscreteField();
    }

    @Override
    public synchronized int getID() throws IllegalStateException {
      if (this.fieldID == UNKNOWN_TYPE_OR_FIELD_ID) {
        this.fieldID = integerQuery("named_fields", "field_id",
                                    "measurement_name = ? AND measurement_version = ? AND field_name = ?",
                                    new String[] {measurementName, measurementVersion, fieldName},
                                    -1);
        if (this.fieldID == UNKNOWN_TYPE_OR_FIELD_ID) {
          throw new IllegalStateException("No field with name " + fieldName +
                                          " (" + measurementName + ", " + measurementVersion + ")");
        }
      }
      return this.fieldID;
    }
  }

  
  
  
  
  private final ConcurrentHashMap<String, Integer> envs = new ConcurrentHashMap<String, Integer>();

  


  public static class DatabaseEnvironment extends Environment {
    protected final HealthReportDatabaseStorage storage;

    @Override
    public int register() {
      final String h = getHash();
      if (storage.envs.containsKey(h)) {
        return storage.envs.get(h);
      }

      
      ContentValues v = new ContentValues();
      v.put("hash", h);
      v.put("profileCreation", profileCreation);
      v.put("cpuCount", cpuCount);
      v.put("memoryMB", memoryMB);
      v.put("isBlocklistEnabled", isBlocklistEnabled);
      v.put("isTelemetryEnabled", isTelemetryEnabled);
      v.put("extensionCount", extensionCount);
      v.put("pluginCount", pluginCount);
      v.put("themeCount", themeCount);
      v.put("architecture", architecture);
      v.put("sysName", sysName);
      v.put("sysVersion", sysVersion);
      v.put("vendor", vendor);
      v.put("appName", appName);
      v.put("appID", appID);
      v.put("appVersion", appVersion);
      v.put("appBuildID", appBuildID);
      v.put("platformVersion", platformVersion);
      v.put("platformBuildID", platformBuildID);
      v.put("os", os);
      v.put("xpcomabi", xpcomabi);
      v.put("updateChannel", updateChannel);

      final SQLiteDatabase db = storage.helper.getWritableDatabase();

      
      
      
      
      
      
      
      try {
        this.id = (int) db.insertOrThrow("environments", null, v);
        storage.envs.put(h, this.id);
        return this.id;
      } catch (SQLException e) {
        
        Cursor c = db.query("environments", COLUMNS_ID, "hash = ?", new String[] {hash}, null, null, null);
        try {
          if (!c.moveToFirst()) {
            throw e;
          }
          this.id = (int) c.getLong(0);
          storage.envs.put(hash, this.id);
          return this.id;
        } finally {
          c.close();
        }
      }
    }

    public void init(ContentValues v) {
      profileCreation = v.getAsInteger("profileCreation");
      cpuCount        = v.getAsInteger("cpuCount");
      memoryMB        = v.getAsInteger("memoryMB");

      isBlocklistEnabled = v.getAsInteger("isBlocklistEnabled");
      isTelemetryEnabled = v.getAsInteger("isTelemetryEnabled");
      extensionCount     = v.getAsInteger("extensionCount");
      pluginCount        = v.getAsInteger("pluginCount");
      themeCount         = v.getAsInteger("themeCount");

      architecture    = v.getAsString("architecture");
      sysName         = v.getAsString("sysName");
      sysVersion      = v.getAsString("sysVersion");
      vendor          = v.getAsString("vendor");
      appName         = v.getAsString("appName");
      appID           = v.getAsString("appID");
      appVersion      = v.getAsString("appVersion");
      appBuildID      = v.getAsString("appBuildID");
      platformVersion = v.getAsString("platformVersion");
      platformBuildID = v.getAsString("platformBuildID");
      os              = v.getAsString("os");
      xpcomabi        = v.getAsString("xpcomabi");
      updateChannel   = v.getAsString("updateChannel");

      this.hash = null;
      this.id = -1;
    }

    





    public boolean init(Cursor cursor) {
      int i = 0;
      this.id         = cursor.getInt(i++);
      this.hash       = cursor.getString(i++);

      profileCreation = cursor.getInt(i++);
      cpuCount        = cursor.getInt(i++);
      memoryMB        = cursor.getInt(i++);

      isBlocklistEnabled = cursor.getInt(i++);
      isTelemetryEnabled = cursor.getInt(i++);
      extensionCount     = cursor.getInt(i++);
      pluginCount        = cursor.getInt(i++);
      themeCount         = cursor.getInt(i++);

      architecture    = cursor.getString(i++);
      sysName         = cursor.getString(i++);
      sysVersion      = cursor.getString(i++);
      vendor          = cursor.getString(i++);
      appName         = cursor.getString(i++);
      appID           = cursor.getString(i++);
      appVersion      = cursor.getString(i++);
      appBuildID      = cursor.getString(i++);
      platformVersion = cursor.getString(i++);
      platformBuildID = cursor.getString(i++);
      os              = cursor.getString(i++);
      xpcomabi        = cursor.getString(i++);
      updateChannel   = cursor.getString(i++);

      return cursor.moveToNext();
    }

    public DatabaseEnvironment(HealthReportDatabaseStorage storage) {
      this.storage = storage;
    }
  }

  



  @Override
  public DatabaseEnvironment getEnvironment() {
    return new DatabaseEnvironment(this);
  }

  @Override
  public SparseArray<Environment> getEnvironmentRecordsByID() {
    final SQLiteDatabase db = this.helper.getReadableDatabase();
    Cursor c = db.query("environments", COLUMNS_ENVIRONMENT_DETAILS, null, null, null, null, null);
    try {
      SparseArray<Environment> results = new SparseArray<Environment>();
      if (!c.moveToFirst()) {
        return results;
      }

      DatabaseEnvironment e = getEnvironment();
      while (e.init(c)) {
        results.put(e.id, e);
        e = getEnvironment();
      }
      results.put(e.id, e);
      return results;
    } finally {
      c.close();
    }
  }

  








  @Override
  public Cursor getEnvironmentRecordForID(int id) {
    final SQLiteDatabase db = this.helper.getReadableDatabase();
    return db.query("environments", ENVIRONMENT_RECORD_COLUMNS, "id = " + id, null, null, null, null);
  }

  @Override
  public SparseArray<String> getEnvironmentHashesByID() {
    final SQLiteDatabase db = this.helper.getReadableDatabase();
    Cursor c = db.query("environments", new String[] {"id", "hash"}, null, null, null, null, null);
    try {
      SparseArray<String> results = new SparseArray<String>();
      if (!c.moveToFirst()) {
        return results;
      }

      while (!c.isAfterLast()) {
        results.put(c.getInt(0), c.getString(1));
      }
      return results;
    } finally {
      c.close();
    }
  }

  



  private HashMap<String, Field> fields = new HashMap<String, Field>();
  private boolean fieldsCacheUpdated = false;

  private String getFieldKey(String mName, int mVersion, String fieldName) {
    return mVersion + "." + mName + "/" + fieldName;
  }

  @Override
  public Field getField(String mName, int mVersion, String fieldName) {
    final String key = getFieldKey(mName, mVersion, fieldName);
    synchronized (fields) {
      if (fields.containsKey(key)) {
        return fields.get(key);
      }
      Field f = new DatabaseField(mName, mVersion, fieldName);
      fields.put(key, f);
      return f;
    }
  }

  private void populateFieldCache() {
    synchronized (fields) {
      if (fieldsCacheUpdated) {
        return;
      }

      fields.clear();
      Cursor c = getFieldVersions();
      try {
        if (!c.moveToFirst()) {
          return;
        }
        do {
          
          final String mName = c.getString(0);
          final int mVersion = c.getInt(2);
          final String fieldName = c.getString(3);
          final int fieldID = c.getInt(4);
          final int flags = c.getInt(5);
          final String key = getFieldKey(mName, mVersion, fieldName);

          Field f = new DatabaseField(mName, mVersion, fieldName, fieldID, flags);
          fields.put(key, f);
        } while (c.moveToNext());
        fieldsCacheUpdated = true;
      } finally {
        c.close();
      }
    }
  }

  


  @Override
  public SparseArray<Field> getFieldsByID() {
    final SparseArray<Field> out = new SparseArray<Field>();
    synchronized (fields) {
      populateFieldCache();
      Collection<Field> values = fields.values();
      for (Field field : values) {
        
        out.put(field.getID(), field);
      }
    }
    return out;
  }

  private final HashMap<String, Integer> measurementVersions = new HashMap<String, Integer>();

  private void populateMeasurementVersionsCache(SQLiteDatabase db) {
    HashMap<String, Integer> results = getIntegers(db, "current_measurements", "name", "version");
    if (results == null) {
      measurementVersions.clear();
      return;
    }
    synchronized (measurementVersions) {
      measurementVersions.clear();
      measurementVersions.putAll(results);
    }
  }

  





  private int getMeasurementVersion(String measurement) {
    synchronized (measurementVersions) {
      if (measurementVersions.containsKey(measurement)) {
        return measurementVersions.get(measurement);
      }

      
      int value = integerQuery("measurements", "version", "name = ?", new String[] {measurement}, 0);
      measurementVersions.put(measurement, value);
      return value;
    }
  }

  






  private void notifyMeasurementVersionUpdated(String measurement, int version) {
    Logger.info(LOG_TAG, "Measurement " + measurement + " now at " + version);

    final SQLiteDatabase db = this.helper.getWritableDatabase();
    final ContentValues values = new ContentValues();
    values.put("name", measurement);
    values.put("version", version);

    synchronized (measurementVersions) {
      measurementVersions.put(measurement, version);
    }

    db.insertWithOnConflict("measurements", null, values, SQLiteDatabase.CONFLICT_IGNORE);
  }

  




  @Override
  public void ensureMeasurementInitialized(String measurement, int version, MeasurementFields fields) {
    final int currentVersion = getMeasurementVersion(measurement);
    Logger.info(LOG_TAG, "Initializing measurement " + measurement + " to " +
                         version + " (current " + currentVersion + ")");

    if (currentVersion == version) {
      Logger.info(LOG_TAG, "Measurement " + measurement + " already at v" + version);
      return;
    }

    final SQLiteDatabase db = this.helper.getWritableDatabase();
    if (!db.inTransaction()) {
      Logger.warn(LOG_TAG, "ensureMeasurementInitialized should be called within a transaction.");
    }

    final ContentValues mv = new ContentValues();
    mv.put("name", measurement);
    mv.put("version", version);

    final int measurementID = (int) db.insert("measurements", null, mv);

    final ContentValues v = new ContentValues();
    v.put("measurement", measurementID);
    for (FieldSpec field : fields.getFields()) {
      v.put("name", field.name);
      v.put("flags", field.type);
      Logger.info(LOG_TAG, "M: " + measurementID + " F: " + field.name + " (" + field.type + ")");
      db.insert("fields", null, v);
    }

    notifyMeasurementVersionUpdated(measurement, version);

    
    synchronized (fields) {
      fieldsCacheUpdated = false;
    }
  }

  



  @Override
  public Cursor getFieldVersions() {
    final SQLiteDatabase db = this.helper.getReadableDatabase();
    return db.query("named_fields", COLUMNS_MEASUREMENT_AND_FIELD_DETAILS,
                    null, null, null, null, "measurement_name, measurement_version, field_name");
  }

  @Override
  public Cursor getFieldVersions(String measurement, int measurementVersion) {
    final SQLiteDatabase db = this.helper.getReadableDatabase();
    return db.query("named_fields", COLUMNS_MEASUREMENT_AND_FIELD_DETAILS,
                    "measurement_name = ? AND measurement_version = ?",
                    new String[] {measurement, Integer.toString(measurementVersion)},
                    null, null, "field_name");
  }

  @Override
  public Cursor getMeasurementVersions() {
    final SQLiteDatabase db = this.helper.getReadableDatabase();
    return db.query("measurements", COLUMNS_MEASUREMENT_DETAILS,
                    null, null, null, null, "name, version");
  }

  





  public void beginInitialization() {
    SQLiteDatabase db = this.helper.getWritableDatabase();
    db.beginTransaction();
    populateMeasurementVersionsCache(db);
  }

  public void finishInitialization() {
    SQLiteDatabase db = this.helper.getWritableDatabase();
    db.setTransactionSuccessful();
    db.endTransaction();
  }

  public void abortInitialization() {
    this.helper.getWritableDatabase().endTransaction();
  }

  @Override
  public int getDay(long time) {
    return HealthReportUtils.getDay(time);
  }

  @Override
  public int getDay() {
    return this.getDay(System.currentTimeMillis());
  }

  private void recordDailyLast(int env, int day, int field, Object value, String table) {
    final SQLiteDatabase db = this.helper.getWritableDatabase();

    final String envString = Integer.toString(env);
    final String fieldIDString = Integer.toString(field, 10);
    final String dayString = Integer.toString(day, 10);

    
    final ContentValues v = new ContentValues();
    putValue(v, value);

    
    
    
    final int updated = db.update(table, v, WHERE_DATE_AND_ENV_AND_FIELD,
                                  new String[] {dayString, envString, fieldIDString});
    if (0 == updated) {
      v.put("env", env);
      v.put("field", field);
      v.put("date", day);
      db.insert(table, null, v);
    }
  }

  @Override
  public void recordDailyLast(int env, int day, int field, String value) {
    this.recordDailyLast(env, day, field, value, EVENTS_TEXTUAL);
  }

  @Override
  public void recordDailyLast(int env, int day, int field, int value) {
    this.recordDailyLast(env, day, field, Integer.valueOf(value), EVENTS_INTEGER);
  }

  private void recordDailyDiscrete(int env, int day, int field, Object value, String table) {
    final ContentValues v = new ContentValues();
    v.put("env", env);
    v.put("field", field);
    v.put("date", day);

    final SQLiteDatabase db = this.helper.getWritableDatabase();
    putValue(v, value);
    db.insert(table, null, v);
  }

  @Override
  public void recordDailyDiscrete(int env, int day, int field, String value) {
    this.recordDailyDiscrete(env, day, field, value, EVENTS_TEXTUAL);
  }

  @Override
  public void recordDailyDiscrete(int env, int day, int field, int value) {
    this.recordDailyDiscrete(env, day, field, value, EVENTS_INTEGER);
  }

  












  @Override
  public void incrementDailyCount(int env, int day, int field, int by) {
    final SQLiteDatabase db = this.helper.getWritableDatabase();
    final String envString = Integer.toString(env);
    final String fieldIDString = Integer.toString(field, 10);
    final String dayString = Integer.toString(day, 10);

    
    
    
    final String[] args = new String[] {dayString, envString, fieldIDString};
    final Cursor c = db.query(EVENTS_INTEGER,
                              COLUMNS_VALUE,
                              WHERE_DATE_AND_ENV_AND_FIELD,
                              args, null, null, null, "1");

    boolean present = false;
    try {
      present = c.moveToFirst();
    } finally {
      c.close();
    }

    if (present) {
      
      db.execSQL("UPDATE " + EVENTS_INTEGER + " SET value = value + " + by + " WHERE " +
                 WHERE_DATE_AND_ENV_AND_FIELD,
                 args);
    } else {
      final ContentValues v = new ContentValues();
      v.put("env", env);
      v.put("value", by);
      v.put("field", field);
      v.put("date", day);
      db.insert(EVENTS_INTEGER, null, v);
    }
  }

  @Override
  public void incrementDailyCount(int env, int day, int field) {
    this.incrementDailyCount(env, day, field, 1);
  }

  









  @Override
  public Cursor getRawEventsSince(long time) {
    final int start = this.getDay(time);
    final SQLiteDatabase db = this.helper.getReadableDatabase();
    final String dayString = Integer.toString(start, 10);
    return db.query("events", COLUMNS_DATE_ENV_FIELD_VALUE,
                    "date >= ?", new String[] {dayString}, null, null, "date, env, field");
  }

  










  @Override
  public Cursor getEventsSince(long time) {
    final int start = this.getDay(time);
    final SQLiteDatabase db = this.helper.getReadableDatabase();
    final String dayString = Integer.toString(start, 10);
    return db.query("named_events", COLUMNS_DATE_ENVSTR_M_MV_F_VALUE,
                    "date >= ?", new String[] {dayString}, null, null,
                    "date, environment, measurement_name, measurement_version, field_name");
  }

  



  private static HashMap<String, Integer> getIntegers(SQLiteDatabase db, String table, String columnA, String columnB) {
    Cursor c = db.query(table, new String[] {columnA, columnB}, null, null, null, null, null);
    try {
      if (!c.moveToFirst()) {
        return null;
      }

      HashMap<String, Integer> results = new HashMap<String, Integer>();
      while (!c.isAfterLast()) {
        results.put(c.getString(0), c.getInt(1));
        c.moveToNext();
      }
      return results;
    } finally {
      c.close();
    }
  }

  


  private int integerQuery(String table, String column, String where, String[] args, int defaultValue) {
    final SQLiteDatabase db = this.helper.getReadableDatabase();
    Cursor c = db.query(table, new String[] {column}, where, args, null, null, column + " DESC", "1");
    try {
      if (!c.moveToFirst()) {
        return defaultValue;
      }
      return c.getInt(0);
    } finally {
      c.close();
    }
  }

  








  private static final void putValue(final ContentValues v, Object value) {
    if (value instanceof String) {
      v.put("value", (String) value);
    } else {
      v.put("value", (Integer) value);
    }
  }

  @Override
  public void deleteEverything() {
    this.helper.deleteEverything();
  }

  @Override
  public void deleteEnvironments() {
    final SQLiteDatabase db = this.helper.getWritableDatabase();
    db.beginTransaction();
    try {
      
      db.delete("environments", null, null);
      db.setTransactionSuccessful();
    } finally {
      db.endTransaction();
    }
  }

  @Override
  public void deleteMeasurements() {
    final SQLiteDatabase db = this.helper.getWritableDatabase();
    db.beginTransaction();
    try {
      
      db.delete("measurements", null, null);
      db.setTransactionSuccessful();
    } finally {
      db.endTransaction();
    }
  }
}
