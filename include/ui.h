#ifndef UI_H
#define UI_H

#include "emu_manager.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <stdbool.h>

typedef struct Ui {
    SDL_Window   *window;
    SDL_Renderer *renderer;
    TTF_Font     *font;
    TTF_Font     *font_small;
} Ui;

bool ui_init(Ui *ui, char *err, size_t err_len);
void ui_shutdown(Ui *ui);

typedef enum {
    CONFIRM_NONE = 0,
    CONFIRM_SAVE_EXIT,
} ConfirmKind;

typedef struct {
    Ui *ui;
    ManagerState *st;
    TabId tab;
    int selected;
    int scroll;
    char toast[256];
    Uint32 toast_until;
    ConfirmKind confirm;
} UiState;

void ui_show_toast(UiState *s, const char *msg);
void ui_render(UiState *s);
void ui_handle_input(UiState *s, SDL_Event *ev, bool *quit, bool *sync_on_exit);

#endif
