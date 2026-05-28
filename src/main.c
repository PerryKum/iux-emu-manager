#include "emu_manager.h"
#include "ui.h"

#include <SDL.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    Ui ui;
    UiState us;
    ManagerState mgr;
    memset(&ui, 0, sizeof ui);
    memset(&us, 0, sizeof us);
    memset(&mgr, 0, sizeof mgr);

    char err[256];
    if (!ui_init(&ui, err, sizeof err)) {
        fprintf(stderr, "emu_manager: %s\n", err);
        sleep(3);
        return 1;
    }

    us.ui = &ui;
    us.st = &mgr;
    manager_scan(&mgr);

    bool quit = false;
    bool sync_on_exit = false;
    while (!quit) {
        ui_render(&us);
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            ui_handle_input(&us, &ev, &quit, &sync_on_exit);
        }
        SDL_Delay(16);
    }

    if (sync_on_exit) {
        if (!manager_commit(&mgr, err, sizeof err)) {
            fprintf(stderr, "emu_manager: %s\n", err);
            sleep(3);
            return 1;
        }
        manager_sync_storage();
    }

    ui_shutdown(&ui);
    return 0;
}
