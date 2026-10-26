#include "aria2/http.h"

#include <string.h>

#define TIMEOUT_SECONDS 10

static gssize
find_header_end (const guint8 *data, gsize len)
{
  for (gsize i = 0; i + 3 < len; i++)
    if (memcmp (data + i, "\r\n\r\n", 4) == 0)
      return i + 4;
  return -1;
}

static gssize
parse_content_length (const guint8 *headers, gsize len)
{
  g_autofree char *h = g_ascii_strdown ((const char *) headers, len);
  const char *p = strstr (h, "\r\ncontent-length:");
  if (!p)
    return -1;
  return g_ascii_strtoll (p + strlen ("\r\ncontent-length:"), NULL, 10);
}

static int
parse_status (const guint8 *data, gsize len)
{
  /* "HTTP/1.1 200 OK" */
  g_autofree char *line = g_strndup ((const char *) data, MIN (len, 32));
  const char *sp = strchr (line, ' ');
  return sp ? (int) g_ascii_strtoll (sp + 1, NULL, 10) : 0;
}

GBytes *
yoink_http_post_json (guint16 port, const char *path, const char *body, GCancellable *cancel, GError **error)
{
  g_autoptr (GSocketClient) client = g_socket_client_new ();
  g_socket_client_set_timeout (client, TIMEOUT_SECONDS);

  g_autoptr (GSocketConnection) conn = g_socket_client_connect_to_host (client, "127.0.0.1", port, cancel, error);
  if (!conn)
    return NULL;

  g_autofree char *req = g_strdup_printf ("POST %s HTTP/1.1\r\n"
                                          "Host: 127.0.0.1:%u\r\n"
                                          "Content-Type: application/json\r\n"
                                          "Content-Length: %zu\r\n"
                                          "Connection: close\r\n\r\n%s",
                                          path, port, strlen (body), body);
  GOutputStream *out = g_io_stream_get_output_stream (G_IO_STREAM (conn));
  if (!g_output_stream_write_all (out, req, strlen (req), NULL, cancel, error))
    return NULL;

  /* Read until we have the headers plus Content-Length bytes, or EOF. */
  GInputStream *in = g_io_stream_get_input_stream (G_IO_STREAM (conn));
  g_autoptr (GByteArray) buf = g_byte_array_new ();
  gssize header_len = -1;
  gssize body_len = -1;
  guint8 chunk[16384];

  for (;;) {
    gssize n = g_input_stream_read (in, chunk, sizeof chunk, cancel, error);
    if (n < 0)
      return NULL;
    if (n == 0)
      break;
    g_byte_array_append (buf, chunk, n);

    if (header_len < 0) {
      header_len = find_header_end (buf->data, buf->len);
      if (header_len >= 0)
        body_len = parse_content_length (buf->data, header_len);
    }
    if (header_len >= 0 && body_len >= 0 && buf->len - (gsize) header_len >= (gsize) body_len)
      break;
  }

  if (header_len < 0) {
    g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA, "Malformed HTTP response");
    return NULL;
  }

  gsize available = buf->len - header_len;
  gsize len = body_len >= 0 ? MIN ((gsize) body_len, available) : available;
  if (len == 0) {
    g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "Empty HTTP response (status %d)",
                 parse_status (buf->data, buf->len));
    return NULL;
  }

  return g_bytes_new (buf->data + header_len, len);
}
