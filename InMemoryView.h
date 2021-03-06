/* Copyright 2012-present Facebook, Inc.
 * Licensed under the Apache License, Version 2.0 */
#pragma once
#include <memory>
#include <unordered_map>
#include "watchman_string.h"

struct watchman_file;
struct watchman_dir;
struct Watcher;

namespace watchman {

/** Keeps track of the state of the filesystem in-memory. */
struct InMemoryView {
  std::unique_ptr<watchman_dir> root_dir;
  w_string root_path;
  Watcher* watcher;
  /** Record the most recent tick value seen during either markFileChanged
   * (or for triggers, is bumped when a trigger is registered).
   * This allows subscribers to know how far back they need to query. */
  uint32_t pending_trigger_tick{0};
  uint32_t pending_sub_tick{0};

  uint32_t last_age_out_tick{0};
  time_t last_age_out_timestamp{0};

  /* the most recently changed file */
  struct watchman_file* latest_file{0};

  /* Holds the list head for files of a given suffix */
  struct file_list_head {
    watchman_file* head{nullptr};
  };

  /* Holds the list heads for all known suffixes */
  std::unordered_map<w_string, std::unique_ptr<file_list_head>> suffixes;

  explicit InMemoryView(const w_string& root_path);

  /** Updates the otime for the file and bubbles it to the front of recency
   * index */
  void markFileChanged(
      watchman_file* file,
      const struct timeval& now,
      uint32_t tick);

  /** Mark a directory as being removed from the view.
   * Marks the contained set of files as deleted.
   * If recursive is true, is recursively invoked on child dirs. */
  void markDirDeleted(
      struct watchman_dir* dir,
      const struct timeval& now,
      uint32_t tick,
      bool recursive);

  watchman_dir* resolveDir(const w_string& dirname, bool create);
  const watchman_dir* resolveDir(const w_string& dirname) const;

  /** Returns the direct child file named name if it already
   * exists, else creates that entry and returns it */
  watchman_file* getOrCreateChildFile(
      watchman_dir* dir,
      const w_string& file_name,
      const struct timeval& now,
      uint32_t tick);
};
}
