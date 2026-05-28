#include "emu_manager.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

void path_join(char *out, size_t out_len, const char *a, const char *b, const char *c) {
    if (c && c[0]) {
        snprintf(out, out_len, "%s/%s/%s", a, b, c);
    } else if (b && b[0]) {
        snprintf(out, out_len, "%s/%s", a, b);
    } else {
        snprintf(out, out_len, "%s", a);
    }
}

static bool is_dir(const char *path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static bool is_file(const char *path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

static void sync_storage(void) {
    sync();
}

void manager_sync_storage(void) {
    sync_storage();
}

bool fs_move(const char *src, const char *dst, char *err, size_t err_len) {
    if (rename(src, dst) == 0) {
        sync_storage();
        return true;
    }
    int e = errno;
    snprintf(err, err_len, "移动失败: %s -> %s (%s)", src, dst, strerror(e));
    return false;
}

bool fs_ensure_dir(const char *path, char *err, size_t err_len) {
    struct stat st;
    if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            return true;
        }
        snprintf(err, err_len, "路径已存在且不是目录: %s", path);
        return false;
    }
    if (mkdir(path, 0755) == 0) {
        sync_storage();
        return true;
    }
    snprintf(err, err_len, "创建目录失败: %s (%s)", path, strerror(errno));
    return false;
}

bool group_dir_visible(GroupId g) {
    char path[MAX_PATH];
    path_join(path, sizeof path, SECTIONS_DIR, kGroups[g].dir_name, NULL);
    return is_dir(path);
}

bool fs_hide_group(GroupId g, char *err, size_t err_len) {
    char src[MAX_PATH], dst[MAX_PATH];
    path_join(src, sizeof src, SECTIONS_DIR, kGroups[g].dir_name, NULL);
    path_join(dst, sizeof dst, EMUBAK_DIR, kGroups[g].dir_name, NULL);
    if (!is_dir(src)) {
        if (is_dir(dst)) {
            return true;
        }
        snprintf(err, err_len, "分组目录不存在: %s", src);
        return false;
    }
    if (is_dir(dst)) {
        snprintf(err, err_len, "目标已存在: %s", dst);
        return false;
    }
  if (!fs_ensure_dir(EMUBAK_DIR, err, err_len)) {
        return false;
    }
    return fs_move(src, dst, err, err_len);
}

bool fs_show_group(GroupId g, char *err, size_t err_len) {
    char src[MAX_PATH], dst[MAX_PATH];
    path_join(src, sizeof src, EMUBAK_DIR, kGroups[g].dir_name, NULL);
    path_join(dst, sizeof dst, SECTIONS_DIR, kGroups[g].dir_name, NULL);
    if (!is_dir(src)) {
        if (is_dir(dst)) {
            return true;
        }
        if (!fs_ensure_dir(dst, err, err_len)) {
            return false;
        }
        return true;
    }
    if (is_dir(dst)) {
        snprintf(err, err_len, "目标已存在: %s", dst);
        return false;
    }
    if (!fs_ensure_dir(SECTIONS_DIR, err, err_len)) {
        return false;
    }
    return fs_move(src, dst, err, err_len);
}

int app_find_location(const char *id) {
    char path[MAX_PATH];
    for (int g = 0; g < GRP_COUNT; g++) {
        path_join(path, sizeof path, SECTIONS_DIR, kGroups[g].dir_name, id);
        if (is_file(path)) {
            return g;
        }
    }
    path_join(path, sizeof path, EMUBAK_DIR, id, NULL);
    if (is_file(path)) {
        return -1;
    }
    return -2;
}

bool fs_move_app(const char *id, int from_loc, int to_loc, char *err, size_t err_len) {
    char src[MAX_PATH], dst[MAX_PATH];

    if (from_loc >= 0) {
        path_join(src, sizeof src, SECTIONS_DIR, kGroups[from_loc].dir_name, id);
    } else if (from_loc == -1) {
        path_join(src, sizeof src, EMUBAK_DIR, id, NULL);
    } else {
        snprintf(err, err_len, "未知来源位置");
        return false;
    }

    if (to_loc >= 0) {
        char dir[MAX_PATH];
        path_join(dir, sizeof dir, SECTIONS_DIR, kGroups[to_loc].dir_name, NULL);
        if (!fs_ensure_dir(dir, err, err_len)) {
            return false;
        }
        path_join(dst, sizeof dst, SECTIONS_DIR, kGroups[to_loc].dir_name, id);
    } else if (to_loc == -1) {
        if (!fs_ensure_dir(EMUBAK_DIR, err, err_len)) {
            return false;
        }
        path_join(dst, sizeof dst, EMUBAK_DIR, id, NULL);
    } else {
        snprintf(err, err_len, "未知目标位置");
        return false;
    }

    if (strcmp(src, dst) == 0) {
        return true;
    }
    if (!is_file(src)) {
        snprintf(err, err_len, "源文件不存在: %s", src);
        return false;
    }
    if (is_file(dst)) {
        snprintf(err, err_len, "目标已存在: %s", dst);
        return false;
    }
    return fs_move(src, dst, err, err_len);
}
