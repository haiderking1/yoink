#include "app/application.h"

int
main (int argc, char **argv)
{
  /* The CPU renderer uses roughly half the memory of the GPU ones for an app
   * this simple. Run with GSK_RENDERER=vulkan (or ngl) to override. */
  g_setenv ("GSK_RENDERER", "cairo", FALSE);

  g_autoptr (DyApplication) app = dy_application_new ();
  return g_application_run (G_APPLICATION (app), argc, argv);
}
