/* SPDX-License-Identifier: LGPL-2.0-or-later */
#include "fake-input.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wayland-client.h>
static struct org_kde_kwin_fake_input *input;
static struct wl_display *display;
static void global(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version)
{
    if (!strcmp(interface, "org_kde_kwin_fake_input"))
        input = wl_registry_bind(registry, name, &org_kde_kwin_fake_input_interface, version < 6 ? version : 6);
}
static void removed(void *data, struct wl_registry *registry, uint32_t name)
{
}
static void sync_input(void)
{
    wl_display_roundtrip(display);
    usleep(50000);
}
static void move(double x, double y)
{
    org_kde_kwin_fake_input_pointer_motion_absolute(input, wl_fixed_from_double(x), wl_fixed_from_double(y));
    sync_input();
}
static void button(int pressed)
{
    org_kde_kwin_fake_input_button(input, 272, pressed);
    sync_input();
}
int main(int argc, char **argv)
{
    display = wl_display_connect(NULL);
    if (!display)
        return 2;
    struct wl_registry *registry = wl_display_get_registry(display);
    static const struct wl_registry_listener listener = {global, removed};
    wl_registry_add_listener(registry, &listener, NULL);
    wl_display_roundtrip(display);
    if (!input) {
        fprintf(stderr, "fake input protocol unavailable\n");
        return 3;
    }
    org_kde_kwin_fake_input_authenticate(input, "Spectacle isolated tests", "Regression test in a private KWin session");
    sync_input();
    if (argc == 6 && !strcmp(argv[1], "drag")) {
        double x = atof(argv[2]), y = atof(argv[3]), dx = atof(argv[4]) - x, dy = atof(argv[5]) - y;
        move(x, y);
        button(1);
        for (int i = 1; i <= 10; i++)
            move(x + dx * i / 10, y + dy * i / 10);
        button(0);
    } else if (argc == 4 && (!strcmp(argv[1], "click") || !strcmp(argv[1], "doubleclick"))) {
        move(atof(argv[2]), atof(argv[3]));
        button(1);
        button(0);
        if (!strcmp(argv[1], "doubleclick")) {
            button(1);
            button(0);
        }
    } else if (argc == 3 && !strcmp(argv[1], "key")) {
        org_kde_kwin_fake_input_keyboard_key(input, atoi(argv[2]), 1);
        sync_input();
        org_kde_kwin_fake_input_keyboard_key(input, atoi(argv[2]), 0);
        sync_input();
    } else
        return 4;
    org_kde_kwin_fake_input_destroy(input);
    wl_display_flush(display);
    wl_display_disconnect(display);
    return 0;
}
