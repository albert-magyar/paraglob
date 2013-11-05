#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <regex.h>

#include "paraglob.h"

typedef struct entry {
  char *pattern;
  regex_t dfa;
  void *value;
} entry_t;

typedef struct regex_trie_node {
  char key;
  regex_t *patterns;
  void **values;
  int n_patterns;
  struct regex_trie_node *next, *children;
} regex_trie_node_t;

struct __paraglob {
  regex_trie_node_t *prefix_root;
  regex_trie_node_t *suffix_root;
  paraglob_match_callback *callback;
  char *test_message;
};

enum __rt_mode { RT_PREFIX, RT_SUFFIX };

regex_trie_node_t *_find_child(regex_trie_node_t *parent, char key) {
  regex_trie_node_t *current = parent->children;
  while (current != NULL && current->key != key) {
    current = current->next;
  }
  return current;
}

regex_trie_node_t *_find_or_create_child(regex_trie_node_t *parent, char key) {
  regex_trie_node_t *match = _find_child(parent, key);
  if (match == NULL) {
    match = calloc(1, sizeof(regex_trie_node_t));
    match->key = key;
    match->next = parent->children;
    parent->children = match;
  }
  return match;
}

uint64_t _get_prefix(const char *pattern, char **prefix) {
  char *prefix_end = strchr(pattern, '*');
  uint64_t prefix_len;
  if (prefix_end) {
    prefix_len = prefix_end - pattern;
  } else {
    prefix_len = strlen(pattern);
  }
  *prefix = calloc(prefix_len + 1, sizeof(char));
  strncpy(*prefix, pattern, prefix_len);
  return prefix_len;
}

uint64_t _get_suffix(const char *pattern, char **suffix) {
  char *suffix_start = strrchr(pattern, '*');
  uint64_t suffix_len;
  if (suffix_start) {
    suffix_len = strlen(pattern) - (suffix_start - pattern) - 1;
    suffix_start++;
  } else {
    suffix_len = strlen(pattern);
    suffix_start = pattern;
  }
  *suffix = calloc(suffix_len + 1, sizeof(char));
  strncpy(*suffix, suffix_start, suffix_len);
  return suffix_len;
}

paraglob_t paraglob_create(enum paraglob_encoding encoding, paraglob_match_callback *callback) {
  paraglob_t pg = malloc(sizeof(struct __paraglob));
  pg->prefix_root = calloc(1, sizeof(regex_trie_node_t));
  pg->suffix_root = calloc(1, sizeof(regex_trie_node_t));
  pg->callback = callback;
  pg->test_message = malloc(5);
  strcpy(pg->test_message, "test");
  return pg;
}

int _prefix_insert(regex_trie_node_t *root, char *prefix, const char *pattern, void *cookie) {
  regex_trie_node_t *current = root;
  int i = 0;
  while (prefix[i]) {
    current = _find_or_create_child(current, prefix[i]);
    i++;
  }
  current->n_patterns++;
  current->patterns = (regex_t *) realloc(current->patterns, current->n_patterns * sizeof(regex_t));
  current->values = (void **) realloc(current->values, current->n_patterns * sizeof(void *));
  current->values[current->n_patterns-1] = cookie;
  return regcomp(current->patterns + current->n_patterns - 1, pattern, 0);
}

int _suffix_insert(regex_trie_node_t *root, char *suffix, const char *pattern, void *cookie) {
  regex_trie_node_t *current = root;
  int i = 0;
  int i_max = strlen(suffix);
  while (i < i_max) {
    int idx = (i_max - i) - 1;
    current = _find_or_create_child(current, suffix[idx]);
    i++;
  }
  current->n_patterns++;
  current->patterns = (regex_t *) realloc(current->patterns, current->n_patterns * sizeof(regex_t));
  current->values = (void **) realloc(current->values, current->n_patterns * sizeof(void *));
  current->values[current->n_patterns-1] = cookie;
  return regcomp(current->patterns + current->n_patterns - 1, pattern, 0);
}

void correct_pattern(char *bre_pattern, const char *pattern) {
  int src_idx = 0;
  int dst_idx = 0;
  while (pattern[src_idx]) {
    if (pattern[src_idx] == '*') {
      bre_pattern[dst_idx++] = '.';
      bre_pattern[dst_idx++] = '*';
    } else {
      bre_pattern[dst_idx++] = pattern[src_idx];
    }
    src_idx++;
  }
}

int paraglob_insert(paraglob_t pg, uint64_t len, const char *pattern, void *cookie) {
  int result;
  char *prefix, *suffix;
  uint64_t prefix_len = _get_prefix(pattern, &prefix);
  uint64_t suffix_len = _get_suffix(pattern, &suffix);
  char *bre_pattern = calloc(len * 2, sizeof(char));
  correct_pattern(bre_pattern, pattern);
  if (prefix_len > suffix_len) {
    result = _prefix_insert(pg->prefix_root, prefix, bre_pattern, cookie);
  } else {
    result = _suffix_insert(pg->suffix_root, suffix, bre_pattern, cookie);
  }
  free(prefix);
  free(suffix);
  free(bre_pattern);
  return (result == 0) ? 1 : 0;
}

int paraglob_compile(paraglob_t pg) {
  return 1;
}

uint64_t _rt_matches(paraglob_t pg, uint64_t len, const char *needle, enum __rt_mode mode) {
  regex_trie_node_t *current = (mode == RT_PREFIX) ? pg->prefix_root : pg->suffix_root;
  int i = 0, j = 0;
  int matches = 0;
  while (i < len) {
    int idx = (mode == RT_PREFIX) ? i : (len - i) - 1;
    current = _find_child(current, needle[idx]);
    if (current) {
      for (j = 0; j < current->n_patterns; j++) {
	if (regexec(current->patterns + j, needle, 0, NULL, 0) == 0) {
	  if (pg->callback) {
	    pg->callback(4, pg->test_message, current->values[j]);
	  }
	  matches++;
	}
      }
    } else {
      break;
    }
    i++;
  }
  return matches;
}

uint64_t paraglob_match(paraglob_t pg, uint64_t len, const char *needle) {
  int prefix_matches = _rt_matches(pg, len, needle, RT_PREFIX);
  int suffix_matches = _rt_matches(pg, len, needle, RT_SUFFIX);
  return prefix_matches + suffix_matches;
}

void paraglob_delete(paraglob_t pg) {

}

void paraglob_enable_debug(paraglob_t pg, FILE *debug) {

}

void paraglob_dump_debug(paraglob_t pg) {

}

void paraglob_stats(paraglob_t pg, uint64_t *fnmatches) {

}

const char *paraglob_strerror(paraglob_t pg) {
  return "no error";
}

