#include <glib.h>
#include <gio/gio.h>

#define GETTEXT_PACKAGE "cuppa"

static gboolean o_display_sleep  = FALSE;
static gboolean o_system_idle    = FALSE;
static gboolean o_disk_idle      = FALSE;
static gboolean o_power_sleep    = FALSE;
static gboolean o_user_active    = FALSE;
static guint64  o_timeout        = 0;
static gint     o_waitpid        = -1;
static gchar**  o_remaining      = NULL;

gboolean
parse_timeout_cb(const gchar *option_name,
                 const gchar *value,
                 __attribute__((unused)) gpointer data,
                 GError **error)
{
  if (value == NULL) {
    o_timeout = 5;
  } else {
    o_timeout = g_ascii_strtoull(value, NULL, 0);
    if (o_timeout == 0) {
      g_set_error(error, G_OPTION_ERROR, 1,
        "'%s' is not a valid duration for %s.", value, option_name);
    } else if (o_timeout == G_MAXUINT64) {
      g_set_error(error, G_OPTION_ERROR, 2,
        "%s seconds is a gigantic duration for %s.", value, option_name);
    }
  }

  return (*error == NULL);
}

static GOptionEntry entries[] = {
  { "display-sleep", 'd', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &o_display_sleep,
    "Prevent the display from sleeping.", NULL },
  { "system-idle", 'i', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &o_system_idle,
    "Prevent the system from idle sleeping.", NULL },
  { "disk-idle", 'm', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &o_disk_idle,
    "Prevent the disk from idle sleeping.", NULL },
  { "power-sleep", 's', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &o_power_sleep,
    "Prevent the system from sleeping if on AC power.", NULL },
  { "user-active", 'u', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &o_user_active,
    "Declare that user is active.", NULL },
  { "timeout", 't', G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_CALLBACK, parse_timeout_cb,
    "Duration in seconds of the override, or 5 seconds if used as a flag.", "T" },
  { "waitpid", 'w', G_OPTION_FLAG_NONE, G_OPTION_ARG_INT, &o_waitpid,
    "Wait for process completion.", "PID" },
  { G_OPTION_REMAINING, G_OPTION_FLAG_NONE, 0, G_OPTION_ARG_STRING_ARRAY,
    &o_remaining, "Command", "-- COMMAND [ARGS...]" },
  { NULL }
};

gboolean remove_restrictions_cb(__attribute__((unused)) gpointer user_data)
{
  g_print("Remove restrictions now!\n");
  return G_SOURCE_REMOVE;
}

gboolean
display_sleep_toggle(__attribute__((unused)) GMainLoop *ml,
                     __attribute__((unused)) gboolean t)
{
  return TRUE;
}

int
main(int argc, char* argv[])
{
  GMainLoop *main_loop = NULL;
  GDBusProxy *gd_proxy = NULL;
  GError        *error = NULL;
  gint           pexit = 0;

  GOptionContext * context = g_option_context_new(
    "- prevent the system from sleeping");
  g_option_context_add_main_entries(context, entries, GETTEXT_PACKAGE);
  g_option_context_set_description(context,
    "This program relies heavily on D-Bus interfaces.");

  gd_proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
    G_DBUS_PROXY_FLAGS_NONE, NULL, "org.freedesktop.ScreenSaver",
    "/org/freedesktop/ScreenSaver", "org.freedesktop.ScreenSaver",
    NULL, &error);

  if (!g_option_context_parse(context, &argc, &argv, &error)) {
    g_printerr("Option parsing failed: %s\n", error->message);
    pexit = 1;
    goto err_cleanup;
  }

  main_loop = g_main_loop_new(NULL, FALSE);

  if (o_timeout != 0) {
    g_timeout_add_seconds(o_timeout, remove_restrictions_cb, NULL);
  }

  if (o_remaining != NULL) {
    if (!g_spawn_sync(NULL, o_remaining, NULL,
        G_SPAWN_SEARCH_PATH | G_SPAWN_CHILD_INHERITS_STDIN,
        NULL, NULL, NULL, NULL, &pexit, &error)) {
      g_printerr("%s\n", error->message);
      pexit = error->code;
      goto err_cleanup;
    }
  } else {
    g_main_loop_run(main_loop);
  }


err_cleanup:
  if (main_loop != NULL) {
    g_main_loop_unref(main_loop);
  }
  if (gd_proxy != NULL) {
    g_object_unref(gd_proxy);
  }
  g_strfreev(o_remaining);
  g_clear_error(&error);
  return pexit;
}
