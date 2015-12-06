#include <glib.h>
#include <gio/gio.h>

typedef struct _Cuppa Cuppa;

struct _Cuppa {
  GMainLoop *main_loop;
  GDBusProxy *d_proxy;
  GOptionContext  *ctx;
};

#define GETTEXT_PACKAGE "cuppa"

gboolean
display_sleep_toggle(Cuppa *cuppa,
                     gboolean toggle);

gboolean remove_restrictions_cb(gpointer user_data);

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
                 gpointer data,
                 GError **error)
{
  Cuppa *cuppa = data;

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

  g_timeout_add_seconds(o_timeout, remove_restrictions_cb, data);

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

gboolean remove_restrictions_cb(gpointer user_data)
{
  Cuppa *cuppa = user_data;

  g_print("Remove restrictions now!\n");
  display_sleep_toggle(cuppa, FALSE);
  return G_SOURCE_REMOVE;
}

gboolean
display_sleep_toggle(Cuppa *cuppa,
                     gboolean toggle)
{
  static guint32 cookie = 0;
  GVariant *out = NULL;
  GError *error = NULL;

  if (toggle) {

    cuppa->d_proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
      G_DBUS_PROXY_FLAGS_NONE, NULL, "org.freedesktop.ScreenSaver",
      "/org/freedesktop/ScreenSaver", "org.freedesktop.ScreenSaver",
      NULL, &error);

      if (error != NULL) {
        g_printerr("%s\n", error->message);
      }

    if (cookie == 0) {
      out = g_dbus_proxy_call_sync(cuppa->d_proxy, "Inhibit",
        g_variant_new("(ss)", g_get_application_name(), "Keep display on"),
        G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

      if (error != NULL) {
        g_printerr("%s\n", error->message);
      }

      g_variant_get_child(out, 0, "u", &cookie);

      g_print("Received screen inhibit cookie: %u\n", cookie);
    }

  } else {

    if (cookie != 0) {
      out = g_dbus_proxy_call_sync(cuppa->d_proxy, "UnInhibit",
        g_variant_new("(u)", cookie), G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

      if (error != NULL) {
        g_printerr("%s\n", error->message);
      }

      g_print("Destroyed screen inhibit cookie\n");
    }

  }

  if (out != NULL) {
    g_variant_unref(out);
  }

  return error == NULL;
}

int
main(int argc, char* argv[])
{
  Cuppa cuppa = {
    .main_loop = g_main_loop_new(NULL, FALSE),
    .d_proxy  = NULL,
    .ctx       = NULL,
  };

  GError        *error = NULL;
  gint           pexit = 0;

  cuppa.ctx = g_option_context_new("- prevent the system from sleeping");
  g_option_context_set_translation_domain(cuppa.ctx, GETTEXT_PACKAGE);
  GOptionGroup *group = g_option_group_new(NULL, NULL, NULL, &cuppa, NULL);
  g_option_group_add_entries(group, entries);
  g_option_context_set_main_group(cuppa.ctx, group);
  g_option_context_set_description(cuppa.ctx,
    "This program relies heavily on D-Bus interfaces.");

  if (!g_option_context_parse(cuppa.ctx, &argc, &argv, &error)) {
    g_printerr("Option parsing failed: %s\n", error->message);
    pexit = 1;
    goto err_cleanup;
  }

  if (o_display_sleep) {
    display_sleep_toggle(&cuppa, TRUE);
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
    g_main_loop_run(cuppa.main_loop);
  }


err_cleanup:
  remove_restrictions_cb(&cuppa);
  if (cuppa.main_loop != NULL) {
    g_main_loop_unref(cuppa.main_loop);
  }
  if (cuppa.d_proxy != NULL) {
    g_object_unref(cuppa.d_proxy);
  }
  g_strfreev(o_remaining);
  g_clear_error(&error);
  return pexit;
}
