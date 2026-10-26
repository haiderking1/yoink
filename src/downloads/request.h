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
} DyRequest;

DyRequest  *dy_request_new        (void);
void        dy_request_free       (DyRequest *r);

/* Validating constructor for untrusted input (the browser extension):
 * { "url" | "urls", "headers", "referer", "userAgent", "filename" } */
DyRequest  *dy_request_from_json  (JsonObject *o, GError **error);

/* Validates and appends; FALSE (with error) if unsafe. */
gboolean    dy_request_add_uri    (DyRequest *r, const char *uri, GError **error);
gboolean    dy_request_add_header (DyRequest *r, const char *header, GError **error);
gboolean    dy_request_set_out    (DyRequest *r, const char *name, GError **error);

/* aria2 per-download options for addUri; may be empty. */
JsonObject *dy_request_to_options (const DyRequest *r);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (DyRequest, dy_request_free)
