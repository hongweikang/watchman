/* Copyright 2012-present Facebook, Inc.
 * Licensed under the Apache License, Version 2.0 */

#include "watchman.h"

// POSIX says open with O_NOFOLLOW should set errno to ELOOP if the path is a
// symlink. However, FreeBSD (which ironically originated O_NOFOLLOW) sets it to
// EMLINK.
#ifdef __FreeBSD__
#define ENOFOLLOWSYMLINK EMLINK
#else
#define ENOFOLLOWSYMLINK ELOOP
#endif

void handle_open_errno(struct write_locked_watchman_root *lock,
                       struct watchman_dir *dir, struct timeval now,
                       const char *syscall, int err, const char *reason) {
  auto dir_name = dir->getFullPath();
  bool log_warning = true;
  bool transient = false;

  if (err == ENOENT || err == ENOTDIR || err == ENOFOLLOWSYMLINK) {
    log_warning = false;
    transient = false;
  } else if (err == EACCES || err == EPERM) {
    log_warning = true;
    transient = false;
  } else if (err == ENFILE || err == EMFILE) {
    set_poison_state(dir_name, now, syscall, err, strerror(err));
    return;
  } else {
    log_warning = true;
    transient = true;
  }

  if (w_string_equal(dir_name, lock->root->root_path)) {
    if (!transient) {
      w_log(
          W_LOG_ERR,
          "%s(%s) -> %s. Root was deleted; cancelling watch\n",
          syscall,
          dir_name.c_str(),
          reason ? reason : strerror(err));
      w_root_cancel(lock->root);
      return;
    }
  }

  auto warn = w_string::printf(
      "%s(%s) -> %s. Marking this portion of the tree deleted",
      syscall,
      dir_name.c_str(),
      reason ? reason : strerror(err));

  w_log(err == ENOENT ? W_LOG_DBG : W_LOG_ERR, "%s\n", warn.c_str());
  if (log_warning) {
    w_root_set_warning(lock, warn);
  }

  stop_watching_dir(lock, dir);
  lock->root->inner.view.markDirDeleted(
      dir, now, lock->root->inner.ticks, true);
}

void w_root_set_warning(
    struct write_locked_watchman_root* lock,
    const w_string& str) {
  lock->root->warning = str;
}


/* vim:ts=2:sw=2:et:
 */
