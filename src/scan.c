#include "emu_manager.h"

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

const GroupInfo kGroups[MAX_GROUPS] = {
    { "applications", "应用",     false, true  },
    { "emulators",    "模拟器",   true,  true  },
    { "handheld",     "掌机",     true,  true  },
    { "console",      "主机",     true,  true  },
    { "settings",     "设置",     false, true  },
    { "arcade",       "街机",     true,  true  },
};

static const char *kLockedAppIds[] = {
    "02usbunmount",
    "03usbmount",
    "emulationstation",
    "update",
    "ra",
    "ppsspp",
    "explor",
    "emumanager",
};

bool app_is_locked(const char *id) {
    if (!id || !id[0]) {
        return false;
    }
    for (size_t i = 0; i < sizeof kLockedAppIds / sizeof kLockedAppIds[0]; i++) {
        if (strcmp(id, kLockedAppIds[i]) == 0) {
            return true;
        }
    }
    return false;
}

static bool reserved_emubak_name(const char *name) {
    return strcmp(name, "arcade") == 0 || strcmp(name, "console") == 0;
}

bool is_regular_entry(const char *name) {
    if (!name || !name[0]) {
        return false;
    }
    if (name[0] == '.') {
        return false;
    }
    return true;
}

bool parse_entry_title(const char *path, char *title, size_t title_len) {
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        snprintf(title, title_len, "%s", path);
        return false;
    }
    char line[512];
    title[0] = '\0';
    while (fgets(line, sizeof line, fp)) {
        if (strncmp(line, "title=", 6) == 0) {
            char *val = line + 6;
            size_t n = strcspn(val, "\r\n");
            val[n] = '\0';
            snprintf(title, title_len, "%s", val);
            fclose(fp);
            return true;
        }
    }
    fclose(fp);
    return false;
}

static void add_app(ManagerState *st, const char *id, const char *path, int location) {
    if (st->app_count >= MAX_APPS) {
        return;
    }
    for (int i = 0; i < st->app_count; i++) {
        if (strcmp(st->apps[i].id, id) == 0) {
            return;
        }
    }
    AppEntry *a = &st->apps[st->app_count++];
    snprintf(a->id, sizeof a->id, "%s", id);
    a->location = location;
    a->cascade_hidden_group = -1;
    if (!parse_entry_title(path, a->title, sizeof a->title)) {
        snprintf(a->title, sizeof a->title, "%s", id);
    }
}

static void scan_group_dir(ManagerState *st, GroupId g) {
    char dir[MAX_PATH];
    path_join(dir, sizeof dir, SECTIONS_DIR, kGroups[g].dir_name, NULL);

    DIR *d = opendir(dir);
    if (!d) {
        return;
    }
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (!is_regular_entry(ent->d_name)) {
            continue;
        }
        char path[MAX_PATH];
        path_join(path, sizeof path, dir, ent->d_name, NULL);
        struct stat stbuf;
        if (stat(path, &stbuf) != 0 || !S_ISREG(stbuf.st_mode)) {
            continue;
        }
        add_app(st, ent->d_name, path, g);
    }
    closedir(d);
}

static void scan_emubak(ManagerState *st) {
    DIR *d = opendir(EMUBAK_DIR);
    if (!d) {
        return;
    }
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (!is_regular_entry(ent->d_name)) {
            continue;
        }
        if (reserved_emubak_name(ent->d_name)) {
            continue;
        }
        char path[MAX_PATH];
        path_join(path, sizeof path, EMUBAK_DIR, ent->d_name, NULL);
        struct stat stbuf;
        if (stat(path, &stbuf) != 0 || !S_ISREG(stbuf.st_mode)) {
            continue;
        }
        add_app(st, ent->d_name, path, -1);
    }
    closedir(d);
}

void manager_scan(ManagerState *st) {
    st->app_count = 0;
    for (int g = 0; g < GRP_COUNT; g++) {
        st->group_visible[g] = group_dir_visible((GroupId)g);
    }
    for (int g = 0; g < GRP_COUNT; g++) {
        scan_group_dir(st, (GroupId)g);
    }
    scan_emubak(st);

    for (int i = 0; i < st->app_count - 1; i++) {
        for (int j = i + 1; j < st->app_count; j++) {
            if (strcmp(st->apps[i].title, st->apps[j].title) > 0) {
                AppEntry tmp = st->apps[i];
                st->apps[i] = st->apps[j];
                st->apps[j] = tmp;
            }
        }
    }
}

static AppEntry *find_app_entry(ManagerState *st, const char *id) {
    for (int i = 0; i < st->app_count; i++) {
        if (strcmp(st->apps[i].id, id) == 0) {
            return &st->apps[i];
        }
    }
    return NULL;
}

static void hide_apps_in_group(ManagerState *st, GroupId g) {
    for (int i = 0; i < st->app_count; i++) {
        AppEntry *a = &st->apps[i];
        if (a->location != g) {
            continue;
        }
        if (app_is_locked(a->id)) {
            continue;
        }
        if (!kGroups[g].apps_can_hide) {
            continue;
        }
        a->cascade_hidden_group = g;
        a->location = -1;
    }
}

static void restore_apps_for_group(ManagerState *st, GroupId g) {
    for (int i = 0; i < st->app_count; i++) {
        AppEntry *a = &st->apps[i];
        if (a->location != -1) {
            continue;
        }
        if (a->cascade_hidden_group != g) {
            continue;
        }
        a->location = g;
        a->cascade_hidden_group = -1;
    }
}

static void coerce_apps_out_of_hidden_groups(ManagerState *st) {
    for (int i = 0; i < st->app_count; i++) {
        AppEntry *a = &st->apps[i];
        if (a->location < 0) {
            continue;
        }
        if (st->group_visible[a->location]) {
            continue;
        }
        if (app_is_locked(a->id)) {
            continue;
        }
        if (!kGroups[a->location].apps_can_hide) {
            continue;
        }
        a->location = -1;
    }
}

bool manager_set_group_visible(ManagerState *st, GroupId g, bool visible, char *err, size_t err_len) {
    if (!kGroups[g].can_hide_group) {
        snprintf(err, err_len, "该分组不允许隐藏");
        return false;
    }
    if (st->group_visible[g] == visible) {
        return true;
    }
    st->group_visible[g] = visible;
    if (visible) {
        restore_apps_for_group(st, g);
    } else {
        hide_apps_in_group(st, g);
    }
    return true;
}

bool manager_set_app_location(ManagerState *st, const char *id, int location, char *err, size_t err_len) {
    AppEntry *a = find_app_entry(st, id);
    if (!a) {
        snprintf(err, err_len, "找不到入口: %s", id);
        return false;
    }
    if (app_is_locked(id) && a->location != location) {
        snprintf(err, err_len, "此项固定，不可调整");
        return false;
    }
    if (a->location >= 0 && !kGroups[a->location].apps_can_hide && location == -1) {
        snprintf(err, err_len, "该入口不允许隐藏");
        return false;
    }
    if (location >= 0 && !st->group_visible[location]) {
        snprintf(err, err_len, "该分组已隐藏");
        return false;
    }
    a->location = location;
    a->cascade_hidden_group = -1;
    return true;
}

bool manager_commit(ManagerState *st, char *err, size_t err_len) {
    coerce_apps_out_of_hidden_groups(st);

    for (int i = 0; i < st->app_count; i++) {
        const AppEntry *a = &st->apps[i];
        int disk = app_find_location(a->id);
        if (disk == -2) {
            snprintf(err, err_len, "找不到入口: %s", a->id);
            return false;
        }
        if (disk == a->location) {
            continue;
        }
        if (app_is_locked(a->id)) {
            snprintf(err, err_len, "此项固定，不可调整: %s", a->id);
            return false;
        }
        if (disk >= 0 && !kGroups[disk].apps_can_hide && a->location == -1) {
            snprintf(err, err_len, "该入口不允许隐藏: %s", a->id);
            return false;
        }
        if (a->location >= 0 && !st->group_visible[a->location]) {
            snprintf(err, err_len, "目标分组已隐藏: %s", a->id);
            return false;
        }
        if (!fs_move_app(a->id, disk, a->location, err, err_len)) {
            return false;
        }
    }

    for (int g = 0; g < GRP_COUNT; g++) {
        bool on_disk = group_dir_visible((GroupId)g);
        bool want = st->group_visible[g];
        if (on_disk == want) {
            continue;
        }
        bool ok = want ? fs_show_group((GroupId)g, err, err_len)
                       : fs_hide_group((GroupId)g, err, err_len);
        if (!ok) {
            return false;
        }
    }

    return true;
}
