#include "protocol.h"

#include <string.h>

/* Chrome allows 64 MiB towards the host, but our requests are tiny. */
#define MAX_INCOMING (4u * 1024 * 1024)
/* Chrome rejects host replies over 1 MiB */
#define MAX_OUTGOING (1u * 1024 * 1024)

JsonNode *
yoink_host_read_message (GInputStream *in, GError **error)
{
  guint32 len;
  gsize got = 0;

  if (!g_input_stream_read_all (in, &len, sizeof len, &got, NULL, error))
    return NULL;
  if (got == 0)
    return NULL;
  if (got < sizeof len) {
    g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_PARTIAL_INPUT, "Truncated message header");
    return NULL;
  }
  if (len == 0 || len > MAX_INCOMING) {
    g_set_error (error, G_IO_ERROR, G_IO_ERROR_MESSAGE_TOO_LARGE, "Bad message length %u", len);
    return NULL;
  }

  g_autofree char *buf = g_malloc (len);
  if (!g_input_stream_read_all (in, buf, len, &got, NULL, error))
    return NULL;
  if (got < len) {
    g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_PARTIAL_INPUT, "Truncated message");
    return NULL;
  }

  g_autoptr (JsonParser) parser = json_parser_new ();
  if (!json_parser_load_from_data (parser, buf, len, error))
    return NULL;
  return json_node_copy (json_parser_get_root (parser));
}

gboolean
yoink_host_write_message (GOutputStream *out, JsonNode *message, GError **error)
{
  gsize size;
  g_autofree char *json = json_to_string (message, FALSE);
  size = strlen (json);

  if (size > MAX_OUTGOING) {
    g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_MESSAGE_TOO_LARGE, "Reply too large");
    return FALSE;
  }

  guint32 len = size;
  return g_output_stream_write_all (out, &len, sizeof len, NULL, NULL, error) &&
         g_output_stream_write_all (out, json, len, NULL, NULL, error) &&
         g_output_stream_flush (out, NULL, error);
}
