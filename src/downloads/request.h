#pragma once

#include <gio/gio.h>
#include <json-glib/json-glib.h>

/* What to download, with shared options: what the browser extension, the
 * command line and the window hand us. Every link and torrent becomes its own
 * aria2 download. */
typedef struct {
  GPtrArray *uris;        /* char* */
  GPtrArray *torrents;    /* GFile*, local .torrent files */
  GPtrArray *headers;     /* char*, "Name: value" */
  char      *referer;
  char      *user_agent;
  char      *out;         /* file name to save as (single link only) */
  char      *dir;         /* folder to save into; set by Yoink, never by the browser */
} YoinkRequest;

YoinkRequest *yoink_request_new             (void);
void          yoink_request_free            (YoinkRequest *r);
/* links + torrents */
guint         yoink_request_count           (const YoinkRequest *r);

/* Validating constructor for untrusted input (the browser extension):
 * { "url" | "urls", "headers", "referer", "userAgent", "filename" } */
YoinkRequest *yoink_request_from_json       (JsonObject *o, GError **error);

/* Validates and appends; FALSE (with error) if unsafe. */
gboolean      yoink_request_add_uri         (YoinkRequest *r, const char *uri, GError **error);
void          yoink_request_add_torrent     (YoinkRequest *r, GFile *file);
gboolean      yoink_request_add_header      (YoinkRequest *r, const char *header, GError **error);
gboolean      yoink_request_set_out         (YoinkRequest *r, const char *name, GError **error);
/* a bare file name: nothing that could point outside the download folder */
gboolean      yoink_request_out_is_valid    (const char *name);

/* aria2 per-download options for addUri / addTorrent; may be empty. */
JsonObject   *yoink_request_to_options      (const YoinkRequest *r);
JsonObject   *yoink_request_torrent_options (const YoinkRequest *r);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (YoinkRequest, yoink_request_free)
