



package org.mozilla.gecko.sync;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;
import org.mozilla.gecko.BrowserLocaleManager;
import org.mozilla.gecko.R;
import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.sync.repositories.NullCursorException;
import org.mozilla.gecko.sync.repositories.android.ClientsDatabaseAccessor;
import org.mozilla.gecko.sync.repositories.domain.ClientRecord;
import org.mozilla.gecko.tabqueue.TabQueueDispatcher;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;















public class CommandProcessor {
  private static final String LOG_TAG = "Command";
  private static final AtomicInteger currentId = new AtomicInteger();
  protected ConcurrentHashMap<String, CommandRunner> commands = new ConcurrentHashMap<String, CommandRunner>();

  private final static CommandProcessor processor = new CommandProcessor();

  




  public static CommandProcessor getProcessor() {
    return processor;
  }

  public static class Command {
    public final String commandType;
    public final JSONArray args;
    private List<String> argsList;

    public Command(String commandType, JSONArray args) {
      this.commandType = commandType;
      this.args = args;
    }

    




    public synchronized List<String> getArgsList() {
      if (argsList == null) {
        ArrayList<String> argsList = new ArrayList<String>(args.size());

        for (int i = 0; i < args.size(); i++) {
          final Object arg = args.get(i);
          if (arg == null) {
            argsList.add(null);
            continue;
          }
          argsList.add(arg.toString());
        }
        this.argsList = argsList;
      }
      return this.argsList;
    }

    @SuppressWarnings("unchecked")
    public JSONObject asJSONObject() {
      JSONObject out = new JSONObject();
      out.put("command", this.commandType);
      out.put("args", this.args);
      return out;
    }
  }

  










  public void registerCommand(String commandType, CommandRunner command) {
    commands.put(commandType, command);
  }

  







  public void processCommand(final GlobalSession session, ExtendedJSONObject unparsedCommand) {
    Command command = parseCommand(unparsedCommand);
    if (command == null) {
      Logger.debug(LOG_TAG, "Invalid command: " + unparsedCommand + " will not be processed.");
      return;
    }

    CommandRunner executableCommand = commands.get(command.commandType);
    if (executableCommand == null) {
      Logger.debug(LOG_TAG, "Command \"" + command.commandType + "\" not registered and will not be processed.");
      return;
    }

    executableCommand.executeCommand(session, command.getArgsList());
  }

  






  protected static Command parseCommand(ExtendedJSONObject unparsedCommand) {
    String type = (String) unparsedCommand.get("command");
    if (type == null) {
      return null;
    }

    try {
      JSONArray unparsedArgs = unparsedCommand.getArray("args");
      if (unparsedArgs == null) {
        return null;
      }

      return new Command(type, unparsedArgs);
    } catch (NonArrayJSONException e) {
      Logger.debug(LOG_TAG, "Unable to parse args array. Invalid command");
      return null;
    }
  }

  @SuppressWarnings("unchecked")
  public void sendURIToClientForDisplay(String uri, String clientID, String title, String sender, Context context) {
    Logger.info(LOG_TAG, "Sending URI to client " + clientID + ".");
    if (Logger.LOG_PERSONAL_INFORMATION) {
      Logger.pii(LOG_TAG, "URI is " + uri + "; title is '" + title + "'.");
    }

    final JSONArray args = new JSONArray();
    args.add(uri);
    args.add(sender);
    args.add(title);

    final Command displayURICommand = new Command("displayURI", args);
    this.sendCommand(clientID, displayURICommand, context);
  }

  












  public void sendCommand(String clientID, Command command, Context context) {
    Logger.debug(LOG_TAG, "In sendCommand.");

    CommandRunner commandData = commands.get(command.commandType);

    
    if (commandData == null) {
      Logger.error(LOG_TAG, "Unknown command to send: " + command);
      return;
    }

    
    if (!commandData.argumentsAreValid(command.getArgsList())) {
      Logger.error(LOG_TAG, "Expected " + commandData.argCount + " args for '" +
                   command + "', but got " + command.args);
      return;
    }

    if (clientID != null) {
      this.sendCommandToClient(clientID, command, context);
      return;
    }

    ClientsDatabaseAccessor db = new ClientsDatabaseAccessor(context);
    try {
      Map<String, ClientRecord> clientMap = db.fetchAllClients();
      for (ClientRecord client : clientMap.values()) {
        this.sendCommandToClient(client.guid, command, context);
      }
    } catch (NullCursorException e) {
      Logger.error(LOG_TAG, "NullCursorException when fetching all GUIDs");
    } finally {
      db.close();
    }
  }

  protected void sendCommandToClient(String clientID, Command command, Context context) {
    Logger.info(LOG_TAG, "Sending " + command.commandType + " to " + clientID);

    ClientsDatabaseAccessor db = new ClientsDatabaseAccessor(context);
    try {
      db.store(clientID, command);
    } catch (NullCursorException e) {
      Logger.error(LOG_TAG, "NullCursorException: Unable to send command.");
    } finally {
      db.close();
    }
  }

  private static volatile boolean didUpdateLocale = false;

  @SuppressWarnings("deprecation")
  public static void displayURI(final List<String> args, final Context context) {
    
    final String uri = args.get(0);
    final String clientId = args.get(1);

    Logger.pii(LOG_TAG, "Received a URI for display: " + uri + " from " + clientId);

    String title = null;
    if (args.size() == 3) {
      title = args.get(2);
    }

    
    
    if (!didUpdateLocale) {
      BrowserLocaleManager.getInstance().getAndApplyPersistedLocale(context);
      didUpdateLocale = true;
    }

    final String ns = Context.NOTIFICATION_SERVICE;
    final NotificationManager notificationManager = (NotificationManager) context.getSystemService(ns);

    
    final int icon = R.drawable.flat_icon;
    String notificationTitle = context.getString(R.string.sync_new_tab);
    if (title != null) {
      notificationTitle = notificationTitle.concat(": " + title);
    }

    final long when = System.currentTimeMillis();
    Notification notification = new Notification(icon, notificationTitle, when);
    notification.flags = Notification.FLAG_AUTO_CANCEL;

    
    Intent notificationIntent = new Intent(Intent.ACTION_VIEW, Uri.parse(uri));
    notificationIntent.putExtra(TabQueueDispatcher.SKIP_TAB_QUEUE_FLAG, true);
    PendingIntent contentIntent = PendingIntent.getActivity(context, 0, notificationIntent, 0);
    notification.setLatestEventInfo(context, notificationTitle, uri, contentIntent);

    
    notificationManager.notify(currentId.getAndIncrement(), notification);
  }
}
