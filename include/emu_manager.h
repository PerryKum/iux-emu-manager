#ifndef EMU_MANAGER_H
#define EMU_MANAGER_H

#include <stdbool.h>
#include <stddef.h>

#define STORAGE_ROOT "/storage"
#define IUX_ROOT     STORAGE_ROOT "/iux"
#define ICEWAY_GAMEPAD_CFG IUX_ROOT "/icewaymusic/config/gamepad.json"
#define SECTIONS_DIR IUX_ROOT "/sections"
/* 放在 sections 内，不在六个首页分组名里，不会单独出现在首页 */
#define EMUBAK_DIR   SECTIONS_DIR "/emubak"

#define MAX_GROUPS     6
#define MAX_APPS       256
#define MAX_NAME       128
#define MAX_PATH       512
#define MAX_TITLE      256

typedef enum {
    TAB_GROUPS = 0,
    TAB_APPS   = 1,
    TAB_COUNT  = 2
} TabId;

typedef enum {
    GRP_APPLICATIONS = 0,
    GRP_EMULATORS,
    GRP_HANDHELD,
    GRP_CONSOLE,
    GRP_SETTINGS,
    GRP_ARCADE,
    GRP_COUNT
} GroupId;

typedef struct {
    const char *dir_name;
    const char *label;
    bool can_hide_group;
    bool apps_can_hide;
} GroupInfo;

typedef struct {
    char id[MAX_NAME];
    char title[MAX_TITLE];
    int  location; /* GroupId, or -1 = hidden in emubak */
    /* 因分组被隐藏而连带标为隐藏时记录来源分组；手动改位置后清零 */
    int  cascade_hidden_group; /* GroupId, or -1 */
} AppEntry;

typedef struct {
    AppEntry  apps[MAX_APPS];
    int       app_count;
    bool      group_visible[MAX_GROUPS];
} ManagerState;

extern const GroupInfo kGroups[MAX_GROUPS];

bool group_dir_visible(GroupId g);
bool fs_hide_group(GroupId g, char *err, size_t err_len);
bool fs_show_group(GroupId g, char *err, size_t err_len);
bool fs_move_app(const char *id, int from_loc, int to_loc, char *err, size_t err_len);
int app_find_location(const char *id);

void manager_scan(ManagerState *st);
bool manager_set_group_visible(ManagerState *st, GroupId g, bool visible, char *err, size_t err_len);
bool manager_set_app_location(ManagerState *st, const char *id, int location, char *err, size_t err_len);
bool manager_commit(ManagerState *st, char *err, size_t err_len);

bool app_is_locked(const char *id);
bool parse_entry_title(const char *path, char *title, size_t title_len);
bool is_regular_entry(const char *name);
void path_join(char *out, size_t out_len, const char *a, const char *b, const char *c);
void manager_sync_storage(void);

#endif
