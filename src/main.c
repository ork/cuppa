#include <glib.h>

#define GETTEXT_PACKAGE "cuppa"

static gboolean o_display_sleep  = FALSE;
static gboolean o_system_idle    = FALSE;
static gboolean o_disk_idle      = FALSE;
static gboolean o_power_sleep    = FALSE;
static gboolean o_user_active    = FALSE;
static gint     o_timeout        = -1;
static gint     o_waitpid        = -1;
static gchar**  o_remaining      = NULL;

static GOptionEntry entries[] = {
  { "display-sleep", 'd', 0, G_OPTION_ARG_NONE, &o_display_sleep,
    "Prevent the display from sleeping.", NULL },
  { "system-idle", 'i', 0, G_OPTION_ARG_NONE, &o_system_idle,
    "Prevent the system from idle sleeping.", NULL },
  { "disk-idle", 'm', 0, G_OPTION_ARG_NONE, &o_disk_idle,
    "Prevent the disk from idle sleeping.", NULL },
  { "power-sleep", 's', 0, G_OPTION_ARG_NONE, &o_power_sleep,
    "Prevent the system from sleeping if on AC power.", NULL },
  { "user-active", 'u', 0, G_OPTION_ARG_NONE, &o_user_active,
    "Declare that user is active.", NULL },
  { "timeout", 't', 0, G_OPTION_ARG_INT, &o_timeout,
    "Duration in seconds of the override.", "T" },
  { "waitpid", 'w', 0, G_OPTION_ARG_INT, &o_waitpid,
    "Wait for process completion.", "PID" },
  { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_STRING_ARRAY,
    &o_remaining, "Command", "-- COMMAND [ARGS...]" },
  { NULL }
};

gboolean remove_restrictions_cb(__attribute__((unused)) gpointer user_data)
{
  g_print("Remove restrictions now!\n");
  return G_SOURCE_REMOVE;
}

int
main(int argc, char* argv[])
{
  GMainLoop *main_loop = NULL;
  GError        *error = NULL;
  gint           pexit = 0;

  GOptionContext * context = g_option_context_new(
    "- prevent the system from sleeping");
  g_option_context_add_main_entries(context, entries, GETTEXT_PACKAGE);
  g_option_context_set_description(context, "This program relies heavily on D-Bus interfaces.");

  if (!g_option_context_parse(context, &argc, &argv, &error)) {
    g_printerr("Option parsing failed: %s\n", error->message);
    pexit = 1;
    goto err_cleanup;
  }

  main_loop = g_main_loop_new(NULL, FALSE);

  if (o_timeout < 0) {
      g_printerr("Only positive values are allowed for the timeout\n");
      pexit = 1;
      goto err_cleanup;
  } else {
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
  g_strfreev(o_remaining);
  g_clear_error(&error);
  return pexit;
}
