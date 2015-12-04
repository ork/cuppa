#include <stdio.h>
#include <glib.h>

#define GETTEXT_PACKAGE "cuppa"

static gboolean o_display_sleep  = FALSE;
static gboolean o_system_idle    = FALSE;
static gboolean o_disk_idle      = FALSE;
static gboolean o_power_sleep    = FALSE;
static gboolean o_user_active    = FALSE;
static gint     o_timeout        = 5;

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
    "Duration of the override.", "T" },
  { NULL }
};

int
main(int argc, char* argv[])
{
  GError *error = NULL;
  GOptionContext * context = g_option_context_new(
      "- prevent the system from sleeping");
  g_option_context_add_main_entries(context, entries, GETTEXT_PACKAGE);
  if (!g_option_context_parse(context, &argc, &argv, &error)) {
    g_print("Option parsing failed: %s\n", error->message);
    return 1;
  }

  return 0;
}
