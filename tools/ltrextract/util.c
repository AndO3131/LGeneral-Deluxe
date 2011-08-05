/*
    Data-structures for extraction.
    Copyright (C) 2005 Leo Savernik  All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
    PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR
    CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
    EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
    PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
    PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
    OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "util.h"

#include "intl/hash-string.h"
#include "util/hashtable.h"
#include "util/hashtable_itr.h"

#include "list.h"
#include "parser.h"

#include <stdlib.h>
#include <string.h>

/* === Translation Stuff ============================================= */

struct Translateables {
  struct hashtable *ht;
  char *domain;
};

struct Comment {
  char *s;		/* string */
  unsigned len;		/* length of string (w/o trailing 0) */
  int full:1;		/* 1 if no more comment may be added */
};

static void comment_delete(void *cmt) {
  struct Comment * const comment = cmt;
  if (!comment) return;
  free(comment->s);
  free(comment);
}

/**
 * Checks whether the given string is already contained at
 * the end of the comment.
 */
static int comment_ends_with(struct Comment *cmt, const char *s, unsigned len) {
  if (cmt->len < len) return 0;
  return memcmp(cmt->s + cmt->len - len, s, len) == 0;
}

static void comment_add(struct Comment *cmt, const char *s, unsigned len) {
  if (cmt->full) return;
  /* FIXME: inefficient */
  cmt->s = realloc(cmt->s, cmt->len + len + 1);
  memcpy(cmt->s + cmt->len, s, len + 1);
  cmt->len += len;
}

struct Translateables *translateables_create(void) {
  struct Translateables *xl = malloc(sizeof(struct Translateables));
  xl->ht = create_hashtable(16384, (unsigned int (*) (void*))hash_string,
                            (int (*)(void *, void *))strcmp, comment_delete);
  xl->domain = 0; /* heeeheee! will hideously crash on strcmp */
  return xl;
}

void translateables_delete(struct Translateables *xl) {
  if (xl) {
    hashtable_destroy(xl->ht, 1);
    free(xl->domain);
    free(xl);
  }
}

void translateables_set_domain(struct Translateables *xl, const char *domain) {
  xl->domain = strdup(domain);
}

const char *translateables_get_domain(struct Translateables *xl) {
  return xl->domain;
}

const char *translateables_get(struct Translateables *xl, const char *key, unsigned *len) {
  struct Comment *cmt = hashtable_search(xl->ht, (void *)key);
  if (!cmt) return 0;
  if (len) *len = cmt->len;
  return cmt->s;
}

void translateables_add(struct Translateables *xl, const char *key, const char *comment) {
  unsigned len = strlen(comment);
  struct Comment *cmt = hashtable_search(xl->ht, (void *)key);
  /* if already contained, add comment to old comment */
  if (cmt) {
    comment_add(cmt, comment, len);
    return;
  }
  
  /* otherwise, create new entry */
  cmt = calloc(1, sizeof(struct Comment));
  cmt->len = len;
  cmt->s = strdup(comment);
  
  hashtable_insert(xl->ht, strdup(key), cmt);
}

struct TranslateablesIterator *translateables_iterator(struct Translateables *xl) {
  return (struct TranslateablesIterator *)hashtable_iterator(xl->ht);
}

const char *translateables_iterator_key(struct TranslateablesIterator *it) {
  return hashtable_iterator_key((struct hashtable_itr *)it);
}

const char *translateables_iterator_comment(struct TranslateablesIterator *it) {
  return ((struct Comment *)hashtable_iterator_value((struct hashtable_itr *)it))->s;
}

int translateables_iterator_advance(struct TranslateablesIterator *it) {
  return hashtable_iterator_advance((struct hashtable_itr *)it);
}

void translateables_iterator_delete(struct TranslateablesIterator *it) {
  free(it);
}

void translateables_add_pdata(struct Translateables *xl, struct PData *pd) {
  const int comment_limit = 100;
  char context[1024];
  unsigned context_len;
  const char *tr_str;

  /* gather context information */
  snprintf(context, sizeof context, " %s:%d",
           parser_get_filename(pd), parser_get_linenumber(pd));
  context_len = strlen(context);

  if (!pd->values) return;
  
  list_reset(pd->values);
  for (; (tr_str = list_next(pd->values)); ) {
    struct Comment *cmt;
    
    /* don't translate empty strings */
    if (!*tr_str) continue;
    
    cmt = hashtable_search(xl->ht, (void *)tr_str);

    /* If not yet contained, insert new one */
    if (!cmt) {
      cmt = calloc(1, sizeof(struct Comment));
      hashtable_insert(xl->ht, strdup(tr_str), cmt);
    }
    
    /* Only add comment if not yet inserted */
    if (!comment_ends_with(cmt, context, context_len)) {
      comment_add(cmt, context, context_len);
      
      if (cmt->len + context_len >= comment_limit) {
        static const char more[] = " ...";
        comment_add(cmt, more, sizeof(more) - 1);
        cmt->full = 1;
      }
    }
  }
}

/* === End Translation Stuff ========================================= */


int resource_matches_domain(struct PData *pd, struct Translateables *xl) {
  char *str;
  if (!parser_get_value(pd, "domain", &str, 0)) return 0;
  return strcmp(translateables_get_domain(xl), str) == 0;
}

/* kate: tab-indents on; space-indent on; replace-tabs off; indent-width 2; dynamic-word-wrap off; inden(t)-mode cstyle */
