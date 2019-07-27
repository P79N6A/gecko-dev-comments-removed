



































#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#ifdef HAVE_LIBXSS
#include <X11/extensions/scrnsaver.h>
#endif

#include <errno.h>
#include <stdio.h>

gboolean save_to_stdout(const gchar *buf, gsize count,
                        GError **error, gpointer data)
{
  size_t written = fwrite(buf, 1, count, stdout);
  if (written != count) {
    g_set_error(error, G_FILE_ERROR, g_file_error_from_errno(errno),
                "Write to stdout failed: %s", g_strerror(errno));
    return FALSE;
  }

  return TRUE;
}

int main(int argc, char** argv)
{
  gdk_init(&argc, &argv);


#if defined(HAVE_LIBXSS) && (MOZ_WIDGET_GTK == 2)
  int event_base, error_base;
  Bool have_xscreensaver =
    XScreenSaverQueryExtension(GDK_DISPLAY(), &event_base, &error_base);

  if (!have_xscreensaver) {
    fprintf(stderr, "No XScreenSaver extension on display\n");
  } else {
    XScreenSaverInfo* info = XScreenSaverAllocInfo();
    if (!info) {
      fprintf(stderr, "%s: Out of memory\n", argv[0]);
      return 1;
    }
    XScreenSaverQueryInfo(GDK_DISPLAY(), GDK_ROOT_WINDOW(), info);

    const char* state;
    const char* til_or_since = nullptr;
    switch (info->state) {
    case ScreenSaverOff:
      state = "Off";
      til_or_since = "XScreenSaver will activate after another %lu seconds idle time\n";
      break;
    case ScreenSaverOn:
      state = "On";
      if (info->til_or_since) {
        til_or_since = "XScreenSaver idle timer activated %lu seconds ago\n";
      } else {
        til_or_since = "XScreenSaver idle activation is disabled\n";
      }
      break;
    case ScreenSaverDisabled:
      state = "Disabled";
      break;
    default:
      state = "unknown";
    }

    const char* kind;
    switch (info->kind) {
    case ScreenSaverBlanked:
      kind = "Blanked";
      break;
    case ScreenSaverInternal:
      state = "Internal";
      break;
    case ScreenSaverExternal:
      state = "External";
      break;
    default:
      state = "unknown";
    }

    fprintf(stderr, "XScreenSaver state: %s\n", state);

    if (til_or_since) {
      fprintf(stderr, "XScreenSaver kind: %s\n", kind);
      fprintf(stderr, til_or_since, info->til_or_since / 1000);
    }

    fprintf(stderr, "User input has been idle for %lu seconds\n", info->idle / 1000);

    XFree(info);
  }
#endif

  GdkPixbuf* screenshot = nullptr;

#if (MOZ_WIDGET_GTK == 2)
  GdkWindow* window = gdk_get_default_root_window();
  screenshot = gdk_pixbuf_get_from_drawable(nullptr, window, nullptr,
                                            0, 0, 0, 0,
                                            gdk_screen_width(),
                                            gdk_screen_height());
#endif
  if (!screenshot) {
    fprintf(stderr, "%s: failed to create screenshot GdkPixbuf\n", argv[0]);
    return 1;
  }

  GError* error = nullptr;
  if (argc > 1) {
    gdk_pixbuf_save(screenshot, argv[1], "png", &error, nullptr);
  } else {
    gdk_pixbuf_save_to_callback(screenshot, save_to_stdout, nullptr,
                                "png", &error, nullptr);
  }
  if (error) {
    fprintf(stderr, "%s: failed to write screenshot as png: %s\n",
            argv[0], error->message);
    return error->code;
  }

  return 0;
}
