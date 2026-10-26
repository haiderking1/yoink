#pragma once

#include <gtk/gtk.h>

#include "downloads/request.h"

/* Asks where to save every incoming request, one popup at a time, starting
 * from the folder used last time. The folder picked is remembered. */
typedef struct _YoinkSavePrompt YoinkSavePrompt;

/* Gets each accepted request, with ->dir set; takes ownership. */
typedef void (*YoinkSavePromptAccept) (YoinkRequest *request, gpointer user_data);

YoinkSavePrompt *yoink_save_prompt_new  (GtkApplication *app, YoinkSavePromptAccept accept, gpointer user_data);
/* takes ownership of request */
void             yoink_save_prompt_push (YoinkSavePrompt *self, YoinkRequest *request);
void             yoink_save_prompt_free (YoinkSavePrompt *self);
