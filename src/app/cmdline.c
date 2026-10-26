#include "app/cmdline.h"

#include "util/links.h"

typedef struct {
  GApplicationCommandLine *cmd;
  GOutputStream           *buffer;
  YoinkCmdlineSubmit       submit;
  gpointer                 user_data;
} StdinRead;

void
yoink_cmdline_register (GApplication *app)
{
  static const GOptionEntry entries[] = {
    { "header", 'H', 0, G_OPTION_ARG_STRING_ARRAY, NULL, "Extra HTTP header, e.g. \"Cookie: a=b\" (repeatable)", "HEADER" },
    { "referer", 0, 0, G_OPTION_ARG_STRING, NULL, "Referer to send", "URL" },
    { "user-agent", 0, 0, G_OPTION_ARG_STRING, NULL, "User-Agent to send", "AGENT" },
    { "out", 'o', 0, G_OPTION_ARG_STRING, NULL, "Save as this file name (single link only)", "NAME" },
    { "request-stdin", 0, 0, G_OPTION_ARG_NONE, NULL, "Read a JSON download request from stdin (used by yoink-host)", NULL },
    { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, NULL, NULL, "LINK|FILE.torrent…" },
    G_OPTION_ENTRY_NULL,
  };
  g_application_add_main_option_entries (app, entries);
  g_application_set_option_context_summary (app, "Download links and torrents with aria2.");
}

/* ---------- JSON request on stdin (from the browser bridge) ---------- */

static void
stdin_read_free (StdinRead *sr)
{
  g_object_unref (sr->buffer);
  g_object_unref (sr->cmd);  /* lets a remote `yoink` exit with our status */
  g_free (sr);
}

static void
fail (GApplicationCommandLine *cmd, const char *message)
{
  g_application_command_line_printerr (cmd, "yoink: %s\n", message);
  g_application_command_line_set_exit_status (cmd, 1);
}

static void
on_stdin_read (GObject *src, GAsyncResult *res, gpointer user_data)
{
  StdinRead *sr = user_data;
  g_autoptr (GError) error = NULL;

  if (g_output_stream_splice_finish (G_OUTPUT_STREAM (src), res, &error) < 0) {
    fail (sr->cmd, error->message);
    stdin_read_free (sr);
    return;
  }

  g_autoptr (GBytes) bytes = g_memory_output_stream_steal_as_bytes (G_MEMORY_OUTPUT_STREAM (sr->buffer));
  gsize len;
  const char *data = g_bytes_get_data (bytes, &len);

  g_autoptr (JsonParser) parser = json_parser_new ();
  if (!json_parser_load_from_data (parser, data, len, &error)) {
    fail (sr->cmd, error->message);
    stdin_read_free (sr);
    return;
  }

  JsonNode *root = json_parser_get_root (parser);
  YoinkRequest *request = JSON_NODE_HOLDS_OBJECT (root) ? yoink_request_from_json (json_node_get_object (root), &error) : NULL;
  if (request)
    sr->submit (request, NULL, sr->user_data);
  else
    fail (sr->cmd, error ? error->message : "Request must be a JSON object");

  stdin_read_free (sr);
}

static int
read_stdin_request (GApplicationCommandLine *cmd, YoinkCmdlineSubmit submit, gpointer user_data)
{
  /* for a remote `yoink`, GApplication passes its stdin over D-Bus */
  GInputStream *in = g_application_command_line_get_stdin (cmd);
  if (!in) {
    fail (cmd, "No stdin to read the request from");
    return 1;
  }

  StdinRead *sr = g_new0 (StdinRead, 1);
  sr->cmd = g_object_ref (cmd);
  sr->buffer = g_memory_output_stream_new_resizable ();
  sr->submit = submit;
  sr->user_data = user_data;

  g_output_stream_splice_async (sr->buffer, in,
                                G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET,
                                G_PRIORITY_DEFAULT, NULL, on_stdin_read, sr);
  g_object_unref (in);
  return 0;
}

/* ---------- plain arguments ---------- */

static gboolean
apply_options (GApplicationCommandLine *cmd, GVariantDict *opts, YoinkRequest *req)
{
  g_autoptr (GError) error = NULL;
  g_autofree const char **headers = NULL;
  const char *s;

  if (g_variant_dict_lookup (opts, "header", "^a&s", &headers))
    for (const char **h = headers; *h; h++)
      if (!yoink_request_add_header (req, *h, &error))
        goto bad;

  if (g_variant_dict_lookup (opts, "referer", "&s", &s))
    g_set_str (&req->referer, s);
  if (g_variant_dict_lookup (opts, "user-agent", "&s", &s))
    g_set_str (&req->user_agent, s);
  if (g_variant_dict_lookup (opts, "out", "&s", &s) && !yoink_request_set_out (req, s, &error))
    goto bad;
  return TRUE;

bad:
  fail (cmd, error->message);
  return FALSE;
}

static gboolean
collect_targets (GApplicationCommandLine *cmd, GVariantDict *opts, YoinkRequest *req, GListStore *torrents)
{
  g_autofree const char **args = NULL;
  if (!g_variant_dict_lookup (opts, G_OPTION_REMAINING, "^a&ay", &args))
    return TRUE;

  gboolean ok = TRUE;
  for (const char **a = args; *a; a++) {
    g_autoptr (GError) error = NULL;

    if (yoink_is_link (*a)) {
      if (!yoink_request_add_uri (req, *a, &error)) {
        fail (cmd, error->message);
        ok = FALSE;
      }
    } else if (g_str_has_suffix (*a, ".torrent")) {
      g_autoptr (GFile) file = g_application_command_line_create_file_for_arg (cmd, *a);
      g_list_store_append (torrents, file);
    } else {
      g_application_command_line_printerr (cmd, "yoink: ignoring '%s' (not a link or .torrent file)\n", *a);
    }
  }
  return ok;
}

int
yoink_cmdline_handle (GApplicationCommandLine *cmd, YoinkCmdlineSubmit submit, gpointer user_data)
{
  GVariantDict *opts = g_application_command_line_get_options_dict (cmd);
  gboolean from_stdin = FALSE;

  if (g_variant_dict_lookup (opts, "request-stdin", "b", &from_stdin) && from_stdin)
    return read_stdin_request (cmd, submit, user_data);

  g_autoptr (YoinkRequest) req = yoink_request_new ();
  g_autoptr (GListStore) torrents = g_list_store_new (G_TYPE_FILE);

  if (!apply_options (cmd, opts, req) || !collect_targets (cmd, opts, req, torrents))
    return 1;

  gboolean has_links = req->uris->len > 0;
  gboolean has_torrents = g_list_model_get_n_items (G_LIST_MODEL (torrents)) > 0;
  if (has_links || has_torrents)
    submit (has_links ? g_steal_pointer (&req) : NULL,
            has_torrents ? G_LIST_MODEL (g_steal_pointer (&torrents)) : NULL,
            user_data);
  return 0;
}
