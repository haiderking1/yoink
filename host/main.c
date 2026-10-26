/* dy-host: Chrome native messaging bridge between the DY extension and DY.
 * Chrome starts it, talks over stdin/stdout, and stops it when done. */

#include <gio/gunixinputstream.h>
#include <gio/gunixoutputstream.h>

#include "handler.h"
#include "protocol.h"

int
main (int argc, char **argv)
{
  g_autoptr (GInputStream) in = g_unix_input_stream_new (STDIN_FILENO, FALSE);
  g_autoptr (GOutputStream) out = g_unix_output_stream_new (STDOUT_FILENO, FALSE);

  for (;;) {
    g_autoptr (GError) error = NULL;
    g_autoptr (JsonNode) message = dy_host_read_message (in, &error);

    if (!message) {
      if (error) {
        g_printerr ("dy-host: %s\n", error->message);
        return 1;
      }
      return 0;  /* browser closed the pipe */
    }

    g_autoptr (JsonNode) answer = dy_host_handle (message);
    if (!dy_host_write_message (out, answer, &error)) {
      g_printerr ("dy-host: %s\n", error->message);
      return 1;
    }
  }
}
