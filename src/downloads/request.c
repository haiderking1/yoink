#include "downloads/request.h"

#include <string.h>

#include "util/json.h"
#include "util/links.h"

YoinkRequest *
yoink_request_new (void)
{
  YoinkRequest *r = g_new0 (YoinkRequest, 1);
  r->uris = g_ptr_array_new_with_free_func (g_free);
  r->headers = g_ptr_array_new_with_free_func (g_free);
  return r;
}

void
yoink_request_free (YoinkRequest *r)
{
  if (!r)
    return;
  g_ptr_array_unref (r->uris);
  g_ptr_array_unref (r->headers);
  g_free (r->referer);
  g_free (r->user_agent);
  g_free (r->out);
  g_free (r);
}

static gboolean
has_line_break (const char *s)
{
  return strpbrk (s, "\r\n") != NULL;
}

static gboolean
invalid (GError **error, const char *what, const char *value)
{
  g_set_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT, "Invalid %s: %s", what, value);
  return FALSE;
}

gboolean
yoink_request_add_uri (YoinkRequest *r, const char *uri, GError **error)
{
  if (!yoink_is_link (uri) || has_line_break (uri))
    return invalid (error, "link", uri);
  g_ptr_array_add (r->uris, g_strdup (uri));
  return TRUE;
}

gboolean
yoink_request_add_header (YoinkRequest *r, const char *header, GError **error)
{
  /* a line break would let a header smuggle in another one */
  const char *colon = strchr (header, ':');
  if (!colon || colon == header || has_line_break (header))
    return invalid (error, "header", header);
  g_ptr_array_add (r->headers, g_strdup (header));
  return TRUE;
}

gboolean
yoink_request_set_out (YoinkRequest *r, const char *name, GError **error)
{
  /* a bare file name only: never let a request write outside the download dir */
  if (!*name || strchr (name, '/') || g_str_equal (name, ".") || g_str_equal (name, "..") ||
      has_line_break (name))
    return invalid (error, "file name", name);
  g_set_str (&r->out, name);
  return TRUE;
}

static gboolean
set_plain_string (char **field, const char *what, const char *value, GError **error)
{
  if (has_line_break (value))
    return invalid (error, what, value);
  g_set_str (field, value);
  return TRUE;
}

static gboolean
add_string_array (JsonArray *a, gboolean (*add) (YoinkRequest *, const char *, GError **),
                  YoinkRequest *r, const char *what, GError **error)
{
  guint n = json_array_get_length (a);
  for (guint i = 0; i < n; i++) {
    JsonNode *n_i = json_array_get_element (a, i);
    if (!JSON_NODE_HOLDS_VALUE (n_i) || json_node_get_value_type (n_i) != G_TYPE_STRING)
      return invalid (error, what, "(not a string)");
    if (!add (r, json_node_get_string (n_i), error))
      return FALSE;
  }
  return TRUE;
}

YoinkRequest *
yoink_request_from_json (JsonObject *o, GError **error)
{
  g_autoptr (YoinkRequest) r = yoink_request_new ();
  const char *s;
  JsonArray *a;

  if ((s = yoink_json_str (o, "url")) && !yoink_request_add_uri (r, s, error))
    return NULL;
  if ((a = yoink_json_arr (o, "urls")) && !add_string_array (a, yoink_request_add_uri, r, "link", error))
    return NULL;
  if ((a = yoink_json_arr (o, "headers")) && !add_string_array (a, yoink_request_add_header, r, "header", error))
    return NULL;
  if ((s = yoink_json_str (o, "referer")) && *s && !set_plain_string (&r->referer, "referer", s, error))
    return NULL;
  if ((s = yoink_json_str (o, "userAgent")) && *s && !set_plain_string (&r->user_agent, "user agent", s, error))
    return NULL;
  if ((s = yoink_json_str (o, "filename")) && *s && !yoink_request_set_out (r, s, error))
    return NULL;

  if (r->uris->len == 0) {
    g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT, "Request has no link");
    return NULL;
  }
  return g_steal_pointer (&r);
}

JsonObject *
yoink_request_to_options (const YoinkRequest *r)
{
  JsonObject *opts = json_object_new ();

  if (r->headers->len) {
    JsonArray *headers = json_array_new ();
    for (guint i = 0; i < r->headers->len; i++)
      json_array_add_string_element (headers, g_ptr_array_index (r->headers, i));
    json_object_set_array_member (opts, "header", headers);
  }
  if (r->referer)
    json_object_set_string_member (opts, "referer", r->referer);
  if (r->user_agent)
    json_object_set_string_member (opts, "user-agent", r->user_agent);
  /* one output name can't apply to several links */
  if (r->out && r->uris->len == 1)
    json_object_set_string_member (opts, "out", r->out);

  return opts;
}
