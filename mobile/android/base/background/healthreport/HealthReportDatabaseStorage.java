



package org.mozilla.gecko.background.healthreport;

import java.io.File;
import java.util.Collection;
import java.util.HashMap;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.Executor;
import java.util.concurrent.Executors;

import org.json.JSONObject;
import org.mozilla.gecko.background.common.DateUtils;
import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.background.healthreport.HealthReportStorage.MeasurementFields.FieldSpec;

import android.content.ContentValues;
import android.content.Context;
import android.content.ContextWrapper;
import android.database.Cursor;
import android.database.SQLException;
import android.database.sqlite.SQLiteConstraintException;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
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
      "id", "version", "hash",
      "profileCreation", "cpuCount", "memoryMB",

      "isBlocklistEnabled", "isTelemetryEnabled", "extensionCount",
      "pluginCount", "themeCount",

      "architecture", "sysName", "sysVersion", "vendor", "appName", "appID",
      "appVersion", "appBuildID", "platformVersion", "platformBuildID", "os",
      "xpcomabi", "updateChannel",

      "distribution", "osLocale", "appLocale", "acceptLangSet",

      
      "addonsBody",

      
      "hasHardwareKeyboard",
      "uiMode", "uiType",
      "screenLayout", "screenXInMM", "screenYInMM"
  };

  public static final String[] COLUMNS_MEASUREMENT_DETAILS = new String[] {"id", "name", "version"};
  public static final String[] COLUMNS_MEASUREMENT_AND_FIELD_DETAILS =
      new String[] {"measurement_name", "measurement_id", "measurement_version",
                    "field_name", "field_id", "field_flags"};

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
    public static final int CURRENT_VERSION = 7;
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
        Logger.pii(LOG_TAG, "Opening database through absolute path " + path.getAbsolutePath());
        return SQLiteDatabase.openOrCreateDatabase(path, null);
      }
    }

    public static String getAbsolutePath(File parent, String name) {
      return parent.getAbsolutePath() + File.separator + name;
    }

    public HealthReportSQLiteOpenHelper(Context context, File profileDirectory, String name) {
      this(context, profileDirectory, name, CURRENT_VERSION);
    }

    
    public HealthReportSQLiteOpenHelper(Context context, File profileDirectory, String name, int version) {
      super(context, getAbsolutePath(profileDirectory, name), null, version);
      Logger.pii(LOG_TAG, "Opening: " + getAbsolutePath(profileDirectory, name));
    }

    @Override
    public void onCreate(SQLiteDatabase db) {
      db.execSQL("CREATE TABLE addons (id INTEGER PRIMARY KEY AUTOINCREMENT, " +
                 "                     body TEXT, " +
                 "                     UNIQUE (body) " +
                 ")");

      
      
      db.execSQL("CREATE TABLE environments (id INTEGER PRIMARY KEY AUTOINCREMENT, " +
                 "                           version INTEGER, " +
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

                 "                           distribution    TEXT, " +
                 "                           osLocale        TEXT, " +
                 "                           appLocale       TEXT, " +
                 "                           acceptLangSet   INTEGER, " +

                 "                           addonsID        INTEGER, " +

                 "                           hasHardwareKeyboard INTEGER, " +
                 "                           uiMode          INTEGER, " +
                 "                           uiType          TEXT, " +
                 "                           screenLayout    INTEGER, " +
                 "                           screenXInMM     INTEGER, " +
                 "                           screenYInMM     INTEGER, " +

                 "                           FOREIGN KEY (addonsID) REFERENCES addons(id) ON DELETE RESTRICT, " +
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

      createAddonsEnvironmentsView(db);
    }

    @Override
    public void onOpen(SQLiteDatabase db) {
      if (!db.isReadOnly()) {
        db.execSQL("PRAGMA foreign_keys=ON;");
      }
    }

    private void createAddonsEnvironmentsView(SQLiteDatabase db) {
      db.execSQL("CREATE VIEW environments_with_addons AS " +
          "SELECT e.id AS id, " +
          "       e.version AS version, " +
          "       e.hash AS hash, " +
          "       e.profileCreation AS profileCreation, " +
          "       e.cpuCount AS cpuCount, " +
          "       e.memoryMB AS memoryMB, " +
          "       e.isBlocklistEnabled AS isBlocklistEnabled, " +
          "       e.isTelemetryEnabled AS isTelemetryEnabled, " +
          "       e.extensionCount AS extensionCount, " +
          "       e.pluginCount AS pluginCount, " +
          "       e.themeCount AS themeCount, " +
          "       e.architecture AS architecture, " +
          "       e.sysName AS sysName, " +
          "       e.sysVersion AS sysVersion, " +
          "       e.vendor AS vendor, " +
          "       e.appName AS appName, " +
          "       e.appID AS appID, " +
          "       e.appVersion AS appVersion, " +
          "       e.appBuildID AS appBuildID, " +
          "       e.platformVersion AS platformVersion, " +
          "       e.platformBuildID AS platformBuildID, " +
          "       e.os AS os, " +
          "       e.xpcomabi AS xpcomabi, " +
          "       e.updateChannel AS updateChannel, " +
          "       e.distribution AS distribution, " +
          "       e.osLocale AS osLocale, " +
          "       e.appLocale AS appLocale, " +
          "       e.acceptLangSet AS acceptLangSet, " +
          "       addons.body AS addonsBody, " +

          "       e.hasHardwareKeyboard AS hasHardwareKeyboard, " +
          "       e.uiMode AS uiMode, " +
          "       e.uiType AS uiType, " +
          "       e.screenLayout AS screenLayout, " +
          "       e.screenXInMM AS screenXInMM, " +
          "       e.screenYInMM AS screenYInMM " +

          "FROM environments AS e, addons " +
          "WHERE e.addonsID = addons.id");
    }

    private void upgradeDatabaseFrom2To3(SQLiteDatabase db) {
      db.execSQL("CREATE TABLE addons (id INTEGER PRIMARY KEY AUTOINCREMENT, " +
                 "                     body TEXT, " +
                 "                     UNIQUE (body) " +
                 ")");

      db.execSQL("ALTER TABLE environments ADD COLUMN addonsID INTEGER REFERENCES addons(id) ON DELETE RESTRICT");

      
    }

    private void upgradeDatabaseFrom3To4(SQLiteDatabase db) {
      
      db.execSQL("UPDATE OR IGNORE fields SET flags = " + Field.TYPE_COUNTED_STRING_DISCRETE +
                 " WHERE measurement IN (SELECT id FROM measurements WHERE name = 'org.mozilla.searches.counts')");
    }

    private void upgradeDatabaseFrom4to5(SQLiteDatabase db) {
      
      
      
      db.delete("addons", "body IS NULL", null);

      
      
      db.delete("fields", "measurement NOT IN (SELECT id FROM measurements)", null);
      db.delete("environments", "addonsID NOT IN (SELECT id from addons)", null);
      db.delete(EVENTS_INTEGER, "env NOT IN (SELECT id FROM environments)", null);
      db.delete(EVENTS_TEXTUAL, "env NOT IN (SELECT id FROM environments)", null);
      db.delete(EVENTS_INTEGER, "field NOT IN (SELECT id FROM fields)", null);
      db.delete(EVENTS_TEXTUAL, "field NOT IN (SELECT id FROM fields)", null);
    }

    private void upgradeDatabaseFrom5to6(SQLiteDatabase db) {
      db.execSQL("DROP VIEW IF EXISTS environments_with_addons");

      
      db.execSQL("ALTER TABLE environments ADD COLUMN version INTEGER DEFAULT 1");

      
      db.execSQL("ALTER TABLE environments ADD COLUMN distribution TEXT DEFAULT ''");
      db.execSQL("ALTER TABLE environments ADD COLUMN osLocale TEXT DEFAULT ''");
      db.execSQL("ALTER TABLE environments ADD COLUMN appLocale TEXT DEFAULT ''");
      db.execSQL("ALTER TABLE environments ADD COLUMN acceptLangSet INTEGER DEFAULT 0");

      
    }

    private void upgradeDatabaseFrom6to7(SQLiteDatabase db) {
      db.execSQL("DROP VIEW IF EXISTS environments_with_addons");

      
      db.execSQL("ALTER TABLE environments ADD COLUMN hasHardwareKeyboard INTEGER DEFAULT 0");
      db.execSQL("ALTER TABLE environments ADD COLUMN uiMode INTEGER DEFAULT 0");
      db.execSQL("ALTER TABLE environments ADD COLUMN uiType TEXT DEFAULT ''");
      db.execSQL("ALTER TABLE environments ADD COLUMN screenLayout INTEGER DEFAULT 0");
      db.execSQL("ALTER TABLE environments ADD COLUMN screenXInMM INTEGER DEFAULT 0");
      db.execSQL("ALTER TABLE environments ADD COLUMN screenYInMM INTEGER DEFAULT 0");

      
      createAddonsEnvironmentsView(db);
    }

    @Override
    public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
      if (oldVersion >= newVersion) {
        return;
      }

      Logger.info(LOG_TAG, "onUpgrade: from " + oldVersion + " to " + newVersion + ".");
      try {
        switch (oldVersion) {
        case 2:
          upgradeDatabaseFrom2To3(db);
        case 3:
          upgradeDatabaseFrom3To4(db);
        case 4:
          upgradeDatabaseFrom4to5(db);
        case 5:
          upgradeDatabaseFrom5to6(db);
        case 6:
          upgradeDatabaseFrom6to7(db);
        }
      } catch (Exception e) {
        Logger.error(LOG_TAG, "Failure in onUpgrade.", e);
        throw new RuntimeException(e);
      }
   }

    public void deleteEverything() {
      final SQLiteDatabase db = this.getWritableDatabase();

      Logger.info(LOG_TAG, "Deleting everything.");
      db.beginTransaction();
      try {
        
        db.delete("measurements", null, null);
        db.delete("environments", null, null);
        db.delete("addons", null, null);
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
                                    UNKNOWN_TYPE_OR_FIELD_ID);
        if (this.fieldID == UNKNOWN_TYPE_OR_FIELD_ID) {
          throw new IllegalStateException("No field with name " + fieldName +
                                          " (" + measurementName + ", " + measurementVersion + ")");
        }
      }
      return this.fieldID;
    }
  }

  
  
  
  
  
  
  protected final ConcurrentHashMap<String, Integer> envs = new ConcurrentHashMap<String, Integer>();

  


  public static class DatabaseEnvironment extends Environment {
    protected final HealthReportDatabaseStorage storage;

    @Override
    public int register() {
      final String h = getHash();
      if (storage.envs.containsKey(h)) {
        this.id = storage.envs.get(h);
        return this.id;
      }

      
      ContentValues v = new ContentValues();
      v.put("version", version);
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
      v.put("distribution", distribution);
      v.put("osLocale", osLocale);
      v.put("appLocale", appLocale);
      v.put("acceptLangSet", acceptLangSet);
      v.put("hasHardwareKeyboard", hasHardwareKeyboard ? 1 : 0);
      v.put("uiMode", uiMode);
      v.put("uiType", uiType.toString());
      v.put("screenLayout", screenLayout);
      v.put("screenXInMM", screenXInMM);
      v.put("screenYInMM", screenYInMM);

      final SQLiteDatabase db = storage.helper.getWritableDatabase();

      
      boolean newTransaction = !db.inTransaction();

      
      
      
      
      
      
      
      
      
      
      
      
      
      

      final String addonsJSON = getNormalizedAddonsJSON();
      if (newTransaction) {
        db.beginTransaction();
      }

      try {
        int addonsID = ensureAddons(db, addonsJSON);
        v.put("addonsID", addonsID);

        try {
          int inserted = (int) db.insertOrThrow("environments", null, v);
          Logger.debug(LOG_TAG, "Inserted ID: " + inserted + " for hash " + h);
          if (inserted == -1) {
            throw new SQLException("Insert returned -1!");
          }
          this.id = inserted;
          storage.envs.put(h, this.id);
          if (newTransaction) {
            db.setTransactionSuccessful();
          }
          return inserted;
        } catch (SQLException e) {
          
          
          Cursor c = db.query("environments", COLUMNS_ID, "hash = ?",
                              new String[] { h }, null, null, null);
          try {
            if (!c.moveToFirst()) {
              throw e;
            }
            this.id = (int) c.getLong(0);
            Logger.debug(LOG_TAG, "Found " + this.id + " for hash " + h);
            storage.envs.put(h, this.id);
            if (newTransaction) {
              db.setTransactionSuccessful();
            }
            return this.id;
          } finally {
            c.close();
          }
        }
      } finally {
        if (newTransaction) {
          db.endTransaction();
        }
      }
    }

    protected static int ensureAddons(SQLiteDatabase db, String json) {
      Cursor c = db.query("addons", COLUMNS_ID, "body = ?",
                          new String[] { (json == null) ? "null" : json }, null, null, null);
      try {
        if (c.moveToFirst()) {
          return c.getInt(0);
        }
        ContentValues values = new ContentValues();
        values.put("body", json);
        return (int) db.insert("addons", null, values);
      } finally {
        c.close();
      }
    }

    public void init(ContentValues v) {
      version         = v.containsKey("version") ? v.getAsInteger("version") : Environment.CURRENT_VERSION;

      Logger.debug(LOG_TAG, "Initializing environment with version " + version);

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

      distribution    = v.getAsString("distribution");
      osLocale        = v.getAsString("osLocale");
      appLocale       = v.getAsString("appLocale");
      acceptLangSet   = v.getAsInteger("acceptLangSet");

      try {
        setJSONForAddons(v.getAsString("addonsBody"));
      } catch (Exception e) {
        
      }

      if (version >= 3) {
        hasHardwareKeyboard = v.getAsInteger("hasHardwareKeyboard") != 0;
        uiMode = v.getAsInteger("uiMode");
        uiType = UIType.fromLabel(v.getAsString("uiType"));
        screenLayout = v.getAsInteger("screenLayout");
        screenXInMM = v.getAsInteger("screenXInMM");
        screenYInMM = v.getAsInteger("screenYInMM");
      }

      this.hash = null;
      this.id = -1;
    }

    





    public boolean init(Cursor cursor) {
      int i = 0;
      this.id         = cursor.getInt(i++);
      this.version    = cursor.getInt(i++);
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

      distribution    = cursor.getString(i++);
      osLocale        = cursor.getString(i++);
      appLocale       = cursor.getString(i++);
      acceptLangSet   = cursor.getInt(i++);

      try {
        setJSONForAddons(cursor.getBlob(i++));
      } catch (Exception e) {
        
      }

      if (this.version >= 3) {
        hasHardwareKeyboard = cursor.getInt(i++) != 0;
        uiMode = cursor.getInt(i++);
        uiType = UIType.fromLabel(cursor.getString(i++));
        screenLayout = cursor.getInt(i++);
        screenXInMM = cursor.getInt(i++);
        screenYInMM = cursor.getInt(i++);
      }

      return cursor.moveToNext();
    }

    public DatabaseEnvironment(HealthReportDatabaseStorage storage, Class<? extends EnvironmentAppender> appender) {
      super(appender);
      this.storage = storage;
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
    Cursor c = db.query("environments_with_addons", COLUMNS_ENVIRONMENT_DETAILS, null, null, null, null, null);
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
    return db.query("environments_with_addons", COLUMNS_ENVIRONMENT_DETAILS, "id = " + id, null, null, null, null);
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
        c.moveToNext();
      }
      return results;
    } finally {
      c.close();
    }
  }

  



  private HashMap<String, Field> fields = new HashMap<String, Field>();
  private boolean fieldsCacheUpdated = false;

  private void invalidateFieldsCache() {
    synchronized (this.fields) {
      fieldsCacheUpdated = false;
    }
  }

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
      Logger.debug(LOG_TAG, "M: " + measurementID + " F: " + field.name + " (" + field.type + ")");
      db.insert("fields", null, v);
    }

    notifyMeasurementVersionUpdated(measurement, version);

    
    invalidateFieldsCache();
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

  protected int getIntFromQuery(final String sql, final String[] selectionArgs) {
    final SQLiteDatabase db = this.helper.getReadableDatabase();
    final Cursor c = db.rawQuery(sql, selectionArgs);
    try {
      if (!c.moveToFirst()) {
        throw new IllegalStateException("Cursor is empty.");
      }
      return c.getInt(0);
    } finally {
      c.close();
    }
  }

  @Override
  public int getDay(long time) {
    return DateUtils.getDay(time);
  }

  @Override
  public int getDay() {
    return this.getDay(System.currentTimeMillis());
  }

  private void recordDailyLast(int env, int day, int field, Object value, String table) {
    if (env == -1) {
      Logger.warn(LOG_TAG, "Refusing to record with environment = -1.");
      return;
    }

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
      try {
        db.insertOrThrow(table, null, v);
      } catch (SQLiteConstraintException e) {
        throw new IllegalStateException("Event did not reference existing an environment or field.", e);
      }
    }
  }

  @Override
  public void recordDailyLast(int env, int day, int field, JSONObject value) {
    this.recordDailyLast(env, day, field, value == null ? "null" : value.toString(), EVENTS_TEXTUAL);
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
    if (env == -1) {
      Logger.warn(LOG_TAG, "Refusing to record with environment = -1.");
      return;
    }

    final ContentValues v = new ContentValues();
    v.put("env", env);
    v.put("field", field);
    v.put("date", day);

    final SQLiteDatabase db = this.helper.getWritableDatabase();
    putValue(v, value);

    
    
    
    
    
    
    final long res = db.insert(table, null, v);
    if (res == -1) {
      Logger.error(LOG_TAG, "Unable to record daily discrete event. Ignoring.");
    }
  }

  @Override
  public void recordDailyDiscrete(int env, int day, int field, JSONObject value) {
    this.recordDailyDiscrete(env, day, field, value == null ? "null" : value.toString(), EVENTS_TEXTUAL);
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
    if (env == -1) {
      Logger.warn(LOG_TAG, "Refusing to record with environment = -1.");
      return;
    }

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
      try {
        db.insertOrThrow(EVENTS_INTEGER, null, v);
      } catch (SQLiteConstraintException e) {
        throw new IllegalStateException("Event did not reference existing an environment or field.", e);
      }
    }
  }

  @Override
  public void incrementDailyCount(int env, int day, int field) {
    this.incrementDailyCount(env, day, field, 1);
  }

  





  @Override
  public boolean hasEventSince(long time) {
    final int start = this.getDay(time);
    final SQLiteDatabase db = this.helper.getReadableDatabase();
    final String dayString = Integer.toString(start, 10);
    Cursor cur = db.query("events", COLUMNS_DATE_ENV_FIELD_VALUE,
        "date >= ?", new String[] {dayString}, null, null, null, "1");
    if (cur == null) {
      
      
      return true;
    }
    try {
      return cur.getCount() > 0;
    } finally {
      cur.close();
    }
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

  public int getEventCount() {
    return getRowCount("events");
  }

  public int getEnvironmentCount() {
    return getRowCount("environments");
  }

  private int getRowCount(String table) {
    
    
    return getIntFromQuery("SELECT COUNT(*) from " + table, null);
  }

  








  public int deleteDataBefore(final long time, final int curEnv) {
    final SQLiteDatabase db = this.helper.getWritableDatabase();
    db.beginTransaction();
    int numRowsDeleted = 0;
    try {
      numRowsDeleted += deleteEnvAndEventsBefore(db, time, curEnv);
      numRowsDeleted += deleteOrphanedAddons(db);
      db.setTransactionSuccessful();
    } finally {
      db.endTransaction();
    }
    return numRowsDeleted;
  }

  







  protected int deleteEnvAndEventsBefore(final long time, final int curEnv) {
    final SQLiteDatabase db = this.helper.getWritableDatabase();
    return deleteEnvAndEventsBefore(db, time, curEnv);
  }

  
  protected int deleteEnvAndEventsBefore(final SQLiteDatabase db, final long time, final int curEnv) {
    
    
    String whereClause =
        "(SELECT COUNT(*) FROM events WHERE date >= ? " +
        "    AND events.env = environments.id) = 0 " +
        "AND id IN (SELECT DISTINCT env FROM events WHERE date < ?)";
    final int day = this.getDay(time);
    final String dayString = Integer.toString(day, 10);
    String[] whereArgs = new String[] {dayString, dayString};

    int numEnvDeleted = 0;
    db.beginTransaction();
    try {
      numEnvDeleted += db.delete("environments", whereClause, whereArgs);
      numEnvDeleted += deleteOrphanedEnv(db, curEnv);
      
      
      deleteEventsBefore(db, dayString);
      db.setTransactionSuccessful();
    } finally {
      db.endTransaction();
    }
    return numEnvDeleted;
  }

  


  protected int deleteOrphanedEnv(final int curEnv) {
    final SQLiteDatabase db = this.helper.getWritableDatabase();
    return deleteOrphanedEnv(db, curEnv);
  }

  
  @SuppressWarnings("static-method")
  protected int deleteOrphanedEnv(final SQLiteDatabase db, final int curEnv) {
    final String whereClause =
        "id != ? AND " +
        "id NOT IN (SELECT env FROM events)";
    final String[] whereArgs = new String[] {Integer.toString(curEnv)};
    return db.delete("environments", whereClause, whereArgs);
  }

  protected int deleteEventsBefore(final String dayString) {
    final SQLiteDatabase db = this.helper.getWritableDatabase();
    return deleteEventsBefore(db, dayString);
  }

  
  @SuppressWarnings("static-method")
  protected int deleteEventsBefore(final SQLiteDatabase db, final String dayString) {
    final String whereClause = "date < ?";
    final String[] whereArgs = new String[] {dayString};
    int numEventsDeleted = 0;
    db.beginTransaction();
    try {
      numEventsDeleted += db.delete("events_integer", whereClause, whereArgs);
      numEventsDeleted += db.delete("events_textual", whereClause, whereArgs);
      db.setTransactionSuccessful();
    } finally {
      db.endTransaction();
    }
    return numEventsDeleted;
  }

  


  protected int deleteOrphanedAddons() {
    final SQLiteDatabase db = this.helper.getWritableDatabase();
    return deleteOrphanedAddons(db);
  }

  
  @SuppressWarnings("static-method")
  protected int deleteOrphanedAddons(final SQLiteDatabase db) {
    final String whereClause = "id NOT IN (SELECT addonsID FROM environments)";
    return db.delete("addons", whereClause, null);
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

      
      invalidateFieldsCache(); 
      populateMeasurementVersionsCache(db); 

      db.setTransactionSuccessful();
    } finally {
      db.endTransaction();
    }
  }

  



  public void pruneEnvironments(final int numToPrune) {
    final SQLiteDatabase db = this.helper.getWritableDatabase();
    db.beginTransaction();
    try {
      db.delete("environments",
          "id in (SELECT env " +
          "       FROM events " +
          "       GROUP BY env " +
          "       ORDER BY MAX(date), env " +
          "       LIMIT " + numToPrune + ")",
          null);
      db.setTransactionSuccessful();

      
      this.envs.clear();
    } finally {
      db.endTransaction();
    }
  }

  






  public void pruneEvents(final int maxNumToPrune) {
    final SQLiteDatabase db = this.helper.getWritableDatabase();

    final Cursor c = db.rawQuery(
        "SELECT MAX(date) " +
        "FROM (SELECT date " +
        "      FROM events " +
        "      ORDER BY date " +
        "      LIMIT " + maxNumToPrune + ")",
        null);
    long pruneDate = -1;
    try {
      if (!c.moveToFirst()) {
        Logger.debug(LOG_TAG, "No max date found in events: table is likely empty. Not pruning " +
            "events.");
        return;
      }
      pruneDate = c.getLong(0);
    } finally {
      c.close();
    }

    final String selection = "date < " + pruneDate;
    db.beginTransaction();
    try {
      db.delete(EVENTS_INTEGER, selection, null);
      db.delete(EVENTS_TEXTUAL, selection, null);
      db.setTransactionSuccessful();
    } finally {
      db.endTransaction();
    }
  }

  public void vacuum() {
    final SQLiteDatabase db = this.helper.getWritableDatabase();
    db.execSQL("vacuum");
  }

  


  public void disableAutoVacuuming() {
    final SQLiteDatabase db = this.helper.getWritableDatabase();
    db.execSQL("PRAGMA auto_vacuum=0");
  }
}
