#pragma once

#include <json-glib/json-glib.h>

/* A request to download one or more links with shared options: what the
 * browser extension and the command line hand us. */
typedef struct {
  GPtrArray *uris;      /* char*, each becomes its own download */
  GPtrArray *headers;   /* char*, "Name: value" */
  char      *referer;
  char      *user_agent;
  char      *out;       /* file name to save as */
} YoinkRequest;

YoinkRequest  *yoink_request_new        (void);
void        yoink_request_free       (YoinkRequest *r);

/* Validating constructor for untrusted input (the browser extension):
 * { "url" | "urls", "headers", "referer", "userAgent", "filename" } */
YoinkRequest  *yoink_request_from_json  (JsonObject *o, GError **error);

/* Validates and appends; FALSE (with error) if unsafe. */
gboolean    yoink_request_add_uri    (YoinkRequest *r, const char *uri, GError **error);
gboolean    yoink_request_add_header (YoinkRequest *r, const char *header, GError **error);
gboolean    yoink_request_set_out    (YoinkRequest *r, const char *name, GError **error);

/* aria2 per-download options for addUri; may be empty. */
JsonObject *yoink_request_to_options (const YoinkRequest *r);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (YoinkRequest, yoink_request_free)
