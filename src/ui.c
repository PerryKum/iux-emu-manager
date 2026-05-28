#include "emu_manager.h"
#include "ui.h"

#include <SDL.h>
#include <SDL_ttf.h>

#include <stdio.h>
#include <string.h>

#define WIN_W 640
#define WIN_H 480
#define ROW_H 34
#define TOP_H 52
#define FOOT_H 36
#define LIST_TOP (TOP_H + 4)
#define LIST_ROWS ((WIN_H - LIST_TOP - FOOT_H) / ROW_H)
#define PAGE_STEP 10

static const char *kFontPaths[] = {
    IUX_ROOT "/com/res/default.ttf",
    IUX_ROOT "/icewaymusic/resources/font.ttf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
    "/usr/share/fonts/TTF/DejaVuSans.ttf",
    NULL
};

static SDL_Color col_bg     = { 24,  28,  36,  255 };
static SDL_Color col_panel  = { 36,  42,  54,  255 };
static SDL_Color col_text   = { 230, 235, 245, 255 };
static SDL_Color col_dim    = { 140, 150, 165, 255 };
static SDL_Color col_accent = { 72,  140, 220, 255 };
static SDL_Color col_sel    = { 52,  88,  140, 255 };
static SDL_Color col_warn   = { 220, 120, 80,  255 };

static TTF_Font *load_font(int size) {
    for (int i = 0; kFontPaths[i]; i++) {
        TTF_Font *f = TTF_OpenFont(kFontPaths[i], size);
        if (f) {
            return f;
        }
    }
    return NULL;
}

typedef struct {
    Uint8 up, down, left, right;
    Uint8 a, b, x, y, select, start;
    Uint8 l1, r1;
} GpadMap;

static GpadMap g_gpad = { 11, 12, 13, 14, 0, 1, 3, 2, 4, 6, 9, 10 };
static SDL_GameController *g_controller;

static int gpad_json_int(const char *json, const char *key) {
    char needle[32];
    snprintf(needle, sizeof needle, "\"%s\"", key);
    const char *p = strstr(json, needle);
    if (!p) {
        return -1;
    }
    p = strchr(p, ':');
    if (!p) {
        return -1;
    }
    return (int)strtol(p + 1, NULL, 10);
}

static void load_gpad_map(void) {
    FILE *f = fopen(ICEWAY_GAMEPAD_CFG, "rb");
    if (!f) {
        return;
    }
    char buf[512];
    size_t n = fread(buf, 1, sizeof buf - 1, f);
    fclose(f);
    buf[n] = '\0';
    int v;
#define GP(k, fld) \
    do { \
        v = gpad_json_int(buf, k); \
        if (v >= 0 && v <= 255) { \
            g_gpad.fld = (Uint8)v; \
        } \
    } while (0)
    GP("up", up);
    GP("down", down);
    GP("left", left);
    GP("right", right);
    GP("a", a);
    GP("b", b);
    GP("x", x);
    GP("y", y);
    GP("select", select);
    GP("start", start);
    GP("l1", l1);
    GP("r1", r1);
#undef GP
}

static void open_game_controller(void) {
    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        if (!SDL_IsGameController(i)) {
            continue;
        }
        g_controller = SDL_GameControllerOpen(i);
        if (g_controller) {
            return;
        }
    }
}

bool ui_init(Ui *ui, char *err, size_t err_len) {
    memset(ui, 0, sizeof *ui);
    g_controller = NULL;
    SDL_SetHint("SDL_VIDEO_KMSDRM_REQUIRE_DRM_MASTER", "0");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) < 0) {
        snprintf(err, err_len, "SDL_Init: %s", SDL_GetError());
        return false;
    }
    if (TTF_Init() < 0) {
        snprintf(err, err_len, "TTF_Init: %s", TTF_GetError());
        return false;
    }
    ui->window = SDL_CreateWindow(
        "Emu Manager", 0, 0, WIN_W, WIN_H, SDL_WINDOW_FULLSCREEN);
    if (!ui->window) {
        snprintf(err, err_len, "CreateWindow: %s", SDL_GetError());
        return false;
    }
    ui->renderer = SDL_CreateRenderer(ui->window, -1, SDL_RENDERER_SOFTWARE | SDL_RENDERER_PRESENTVSYNC);
    if (!ui->renderer) {
        ui->renderer = SDL_CreateRenderer(ui->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    }
    if (!ui->renderer) {
        snprintf(err, err_len, "CreateRenderer: %s", SDL_GetError());
        return false;
    }
    ui->font = load_font(22);
    ui->font_small = load_font(16);
    if (!ui->font) {
        snprintf(err, err_len, "找不到中文字体 (TTF)");
        return false;
    }
    if (!ui->font_small) {
        ui->font_small = ui->font;
    }
    SDL_GameControllerEventState(SDL_ENABLE);
    load_gpad_map();
    open_game_controller();
    return true;
}

void ui_shutdown(Ui *ui) {
    if (g_controller) {
        SDL_GameControllerClose(g_controller);
        g_controller = NULL;
    }
    if (ui->font_small && ui->font_small != ui->font) {
        TTF_CloseFont(ui->font_small);
    }
    if (ui->font) {
        TTF_CloseFont(ui->font);
    }
    if (ui->renderer) {
        SDL_DestroyRenderer(ui->renderer);
    }
    if (ui->window) {
        SDL_DestroyWindow(ui->window);
    }
    TTF_Quit();
    SDL_Quit();
}

static void draw_text(Ui *ui, const char *text, int x, int y, SDL_Color c, TTF_Font *font) {
    if (!text || !text[0]) {
        return;
    }
    SDL_Surface *surf = TTF_RenderUTF8_Blended(font ? font : ui->font, text, c);
    if (!surf) {
        return;
    }
    SDL_Texture *tex = SDL_CreateTextureFromSurface(ui->renderer, surf);
    if (!tex) {
        SDL_FreeSurface(surf);
        return;
    }
    SDL_Rect dst = { x, y, surf->w, surf->h };
    SDL_RenderCopy(ui->renderer, tex, NULL, &dst);
    SDL_DestroyTexture(tex);
    SDL_FreeSurface(surf);
}

static void draw_rect(Ui *ui, SDL_Rect r, SDL_Color c) {
    SDL_SetRenderDrawColor(ui->renderer, c.r, c.g, c.b, c.a);
    SDL_RenderFillRect(ui->renderer, &r);
}

static void draw_border_rect(Ui *ui, SDL_Rect r, SDL_Color fill, SDL_Color border) {
    draw_rect(ui, r, fill);
    SDL_SetRenderDrawColor(ui->renderer, border.r, border.g, border.b, border.a);
    SDL_RenderDrawRect(ui->renderer, &r);
}

static int row_count(UiState *s) {
    return s->tab == TAB_GROUPS ? GRP_COUNT : s->st->app_count;
}

static void clamp_scroll(UiState *s) {
    int n = row_count(s);
    if (n <= LIST_ROWS) {
        s->scroll = 0;
    } else if (s->scroll > n - LIST_ROWS) {
        s->scroll = n - LIST_ROWS;
    }
    if (s->scroll < 0) {
        s->scroll = 0;
    }
    if (s->selected < 0) {
        s->selected = 0;
    }
    if (s->selected >= n && n > 0) {
        s->selected = n - 1;
    }
    if (s->selected < s->scroll) {
        s->scroll = s->selected;
    }
    if (s->selected >= s->scroll + LIST_ROWS) {
        s->scroll = s->selected - LIST_ROWS + 1;
    }
}

static void group_value_text(GroupId g, bool visible, char *buf, size_t len) {
    if (!kGroups[g].can_hide_group) {
        snprintf(buf, len, "始终显示");
        return;
    }
    snprintf(buf, len, visible ? "显示" : "隐藏");
}

static int app_cycle_next_location(ManagerState *st, int cur_loc, int dir) {
    int slots[GRP_COUNT + 1];
    int n = 0;
    slots[n++] = -1;
    for (int g = 0; g < GRP_COUNT; g++) {
        if (st->group_visible[g]) {
            slots[n++] = g;
        }
    }
    int idx = 0;
    for (int i = 0; i < n; i++) {
        if (slots[i] == cur_loc) {
            idx = i;
            break;
        }
    }
    if (dir > 0) {
        idx = (idx + 1) % n;
    } else if (dir < 0) {
        idx = (idx + n - 1) % n;
    }
    return slots[idx];
}

static void app_value_text(const AppEntry *a, char *buf, size_t len) {
    if (a->location < 0) {
        snprintf(buf, len, "隐藏");
        return;
    }
    snprintf(buf, len, "%s", kGroups[a->location].label);
}

static bool app_can_cycle(const AppEntry *a) {
    if (app_is_locked(a->id)) {
        return false;
    }
    if (a->location >= 0 && !kGroups[a->location].apps_can_hide) {
        return false;
    }
    return true;
}

void ui_show_toast(UiState *s, const char *msg) {
    snprintf(s->toast, sizeof s->toast, "%s", msg ? msg : "");
    s->toast_until = SDL_GetTicks() + 2800;
}

static void draw_tabs(UiState *s) {
    Ui *ui = s->ui;
    SDL_Rect bar = { 0, 0, WIN_W, TOP_H };
    draw_rect(ui, bar, col_panel);
    const char *tabs[] = { "分组", "应用" };
    int tw = WIN_W / TAB_COUNT;
    for (int i = 0; i < TAB_COUNT; i++) {
        SDL_Rect r = { i * tw + 8, 8, tw - 16, TOP_H - 16 };
        bool active = (int)s->tab == i;
        draw_border_rect(ui, r, active ? col_sel : col_bg, active ? col_accent : col_dim);
        draw_text(ui, tabs[i], r.x + 24, r.y + 10, active ? col_text : col_dim, ui->font);
    }
}

static void draw_row(UiState *s, int row_index, int y) {
    Ui *ui = s->ui;
    bool selected = (row_index == s->selected);
    SDL_Rect r = { 12, y, WIN_W - 24, ROW_H - 2 };
    draw_border_rect(ui, r, selected ? col_sel : col_bg, selected ? col_accent : col_panel);

    char left[256];
    char right[128];
    if (s->tab == TAB_GROUPS) {
        GroupId g = (GroupId)row_index;
        snprintf(left, sizeof left, "%s", kGroups[g].label);
        group_value_text(g, s->st->group_visible[g], right, sizeof right);
    } else {
        const AppEntry *a = &s->st->apps[row_index];
        snprintf(left, sizeof left, "%s", a->title[0] ? a->title : a->id);
        app_value_text(a, right, sizeof right);
        if (!app_can_cycle(a)) {
            strncat(right, " (固定)", sizeof right - strlen(right) - 1);
        }
    }

    draw_text(ui, left, r.x + 12, r.y + 6, col_text, ui->font);
    int rw = 0;
    TTF_SizeUTF8(ui->font, right, &rw, NULL);
    draw_text(ui, "<", r.x + r.w - rw - 56, r.y + 6, col_dim, ui->font_small);
    draw_text(ui, right, r.x + r.w - rw - 40, r.y + 6, selected ? col_accent : col_text, ui->font);
    draw_text(ui, ">", r.x + r.w - 28, r.y + 6, col_dim, ui->font_small);
}

static void draw_list(UiState *s) {
    int n = row_count(s);
    for (int i = 0; i < LIST_ROWS; i++) {
        int idx = s->scroll + i;
        if (idx >= n) {
            break;
        }
        draw_row(s, idx, LIST_TOP + i * ROW_H);
    }
    if (n == 0) {
        draw_text(s->ui, "(无项目)", 24, LIST_TOP + 20, col_dim, s->ui->font);
    }
}

static void draw_confirm_dialog(UiState *s) {
    Ui *ui = s->ui;
    SDL_SetRenderDrawBlendMode(ui->renderer, SDL_BLENDMODE_BLEND);
    SDL_Rect dim = { 0, 0, WIN_W, WIN_H };
    SDL_SetRenderDrawColor(ui->renderer, 0, 0, 0, 180);
    SDL_RenderFillRect(ui->renderer, &dim);

    SDL_Rect box = { 80, 140, WIN_W - 160, 200 };
    draw_border_rect(ui, box, col_panel, col_accent);

    const char *title = "保存并退出？";
    const char *line1 = "将同步数据到存储卡后退出";
    const char *line2 = "A 确定    B 取消";
    draw_text(ui, title, box.x + 24, box.y + 24, col_text, ui->font);
    draw_text(ui, line1, box.x + 24, box.y + 64, col_dim, ui->font_small);
    draw_text(ui, line2, box.x + 24, box.y + 120, col_accent, ui->font_small);
    SDL_SetRenderDrawBlendMode(ui->renderer, SDL_BLENDMODE_NONE);
}

static void draw_footer(UiState *s) {
    Ui *ui = s->ui;
    SDL_Rect r = { 0, WIN_H - FOOT_H, WIN_W, FOOT_H };
    draw_rect(ui, r, col_panel);
    if (s->confirm != CONFIRM_NONE) {
        draw_text(ui, "A 确定   B 取消", 12, WIN_H - FOOT_H + 8, col_dim, ui->font_small);
    } else {
        draw_text(ui, "START:保存退出  B:退出  X/Y:翻页  L1/R1:标签  方向:移动  左右:切换", 12, WIN_H - FOOT_H + 8, col_dim, ui->font_small);
    }
    if (s->toast[0] && SDL_GetTicks() < s->toast_until) {
        draw_text(ui, s->toast, 12, WIN_H - FOOT_H - 28, col_warn, ui->font_small);
    }
}

void ui_render(UiState *s) {
    if (!s || !s->ui || !s->ui->renderer) {
        return;
    }
    SDL_SetRenderDrawColor(s->ui->renderer, col_bg.r, col_bg.g, col_bg.b, 255);
    SDL_RenderClear(s->ui->renderer);
    draw_rect(s->ui, (SDL_Rect){ 0, 0, WIN_W, WIN_H }, col_bg);
    draw_tabs(s);
    draw_list(s);
    draw_footer(s);
    if (s->confirm != CONFIRM_NONE) {
        draw_confirm_dialog(s);
    }
    SDL_RenderPresent(s->ui->renderer);
}

static void change_group(UiState *s, GroupId g, int dir) {
    if (!kGroups[g].can_hide_group) {
        return;
    }
    bool vis = s->st->group_visible[g];
    bool target = vis;
    if (dir > 0) {
        target = false; /* 右：隐藏 */
    } else if (dir < 0) {
        target = true; /* 左：显示 */
    } else {
        target = !vis;
    }
    if (target == vis) {
        return;
    }
    char err[256];
    if (!manager_set_group_visible(s->st, g, target, err, sizeof err)) {
        ui_show_toast(s, err);
    }
}

static void change_app(UiState *s, int dir) {
    if (s->st->app_count == 0) {
        return;
    }
    const AppEntry *a = &s->st->apps[s->selected];
    if (!app_can_cycle(a)) {
        ui_show_toast(s, "此项不可隐藏或移动");
        return;
    }
    int new_loc = app_cycle_next_location(s->st, a->location, dir);
    if (new_loc == a->location) {
        return;
    }
    char err[256];
    if (!manager_set_app_location(s->st, a->id, new_loc, err, sizeof err)) {
        ui_show_toast(s, err);
    }
}

static void nav_up(UiState *s) {
    s->selected--;
    clamp_scroll(s);
}

static void nav_down(UiState *s) {
    s->selected++;
    clamp_scroll(s);
}

static void nav_page_up(UiState *s) {
    if (row_count(s) == 0) {
        return;
    }
    s->selected -= PAGE_STEP;
    if (s->selected < 0) {
        s->selected = 0;
    }
    clamp_scroll(s);
}

static void nav_page_down(UiState *s) {
    int n = row_count(s);
    if (n == 0) {
        return;
    }
    s->selected += PAGE_STEP;
    if (s->selected >= n) {
        s->selected = n - 1;
    }
    clamp_scroll(s);
}

static void nav_left(UiState *s) {
    if (s->tab == TAB_GROUPS) {
        change_group(s, (GroupId)s->selected, -1);
    } else {
        change_app(s, -1);
    }
}

static void nav_right(UiState *s) {
    if (s->tab == TAB_GROUPS) {
        change_group(s, (GroupId)s->selected, 1);
    } else {
        change_app(s, 1);
    }
}

static void tab_prev(UiState *s) {
    s->tab = (TabId)((s->tab + TAB_COUNT - 1) % TAB_COUNT);
    s->selected = 0;
    s->scroll = 0;
}

static void tab_next(UiState *s) {
    s->tab = (TabId)((s->tab + 1) % TAB_COUNT);
    s->selected = 0;
    s->scroll = 0;
}

static void handle_confirm_button(UiState *s, Uint8 btn, bool *quit, bool *sync_on_exit) {
    if (btn == g_gpad.a) {
        *sync_on_exit = true;
        *quit = true;
        s->confirm = CONFIRM_NONE;
    } else if (btn == g_gpad.b || btn == g_gpad.select) {
        s->confirm = CONFIRM_NONE;
    }
}

static void handle_pad_button(UiState *s, Uint8 btn, bool *quit, bool *sync_on_exit) {
    if (s->confirm != CONFIRM_NONE) {
        handle_confirm_button(s, btn, quit, sync_on_exit);
        return;
    }
    if (btn == g_gpad.up) {
        nav_up(s);
    } else if (btn == g_gpad.down) {
        nav_down(s);
    } else if (btn == g_gpad.left) {
        nav_left(s);
    } else if (btn == g_gpad.right) {
        nav_right(s);
    } else if (btn == g_gpad.x) {
        nav_page_down(s);
    } else if (btn == g_gpad.y) {
        nav_page_up(s);
    } else if (btn == g_gpad.l1) {
        tab_prev(s);
    } else if (btn == g_gpad.r1) {
        tab_next(s);
    } else if (btn == g_gpad.start) {
        s->confirm = CONFIRM_SAVE_EXIT;
    } else if (btn == g_gpad.b || btn == g_gpad.select) {
        *quit = true;
    }
}

static bool axis_repeat_ok(void) {
    static Uint32 last = 0;
    Uint32 now = SDL_GetTicks();
    if (now - last < 160) {
        return false;
    }
    last = now;
    return true;
}

static void handle_axis_nav(UiState *s, Uint8 axis, Sint16 val) {
    const Sint16 dead = 8000;
    if (axis == 0) {
        if (val < -dead) {
            nav_left(s);
        }
        if (val > dead) {
            nav_right(s);
        }
    } else if (axis == 1) {
        if (val < -dead) {
            nav_up(s);
        }
        if (val > dead) {
            nav_down(s);
        }
    }
}

void ui_handle_input(UiState *s, SDL_Event *ev, bool *quit, bool *sync_on_exit) {
    if (ev->type == SDL_QUIT) {
        *quit = true;
        return;
    }

    if (ev->type == SDL_CONTROLLERDEVICEADDED) {
        if (!g_controller) {
            g_controller = SDL_GameControllerOpen(ev->cdevice.which);
        }
        return;
    }

    if (ev->type == SDL_CONTROLLERBUTTONDOWN) {
        handle_pad_button(s, ev->cbutton.button, quit, sync_on_exit);
        return;
    }

    if (s->confirm != CONFIRM_NONE) {
        return;
    }

    if (ev->type == SDL_CONTROLLERAXISMOTION) {
        if (!axis_repeat_ok()) {
            return;
        }
        if (ev->caxis.axis == SDL_CONTROLLER_AXIS_LEFTX) {
            handle_axis_nav(s, 0, ev->caxis.value);
        } else if (ev->caxis.axis == SDL_CONTROLLER_AXIS_LEFTY) {
            handle_axis_nav(s, 1, ev->caxis.value);
        }
    }
}
