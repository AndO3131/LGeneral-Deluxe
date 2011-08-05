/*
    Translation utilities.
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

#include "localize.h"
#include "hashtable.h"
#include "paths.h"

#include "intl/hash-string.h"

#include <assert.h>
#include <locale.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/** internal language code. LC_C means default as well as unknown. */
typedef enum { LC_C, LC_DE, LC_EN } LanguageCode;

/** info about a loaded domain */
struct DomainInfo {
  int dummy; /* none yet */
};

static LanguageCode lang_code;
static struct hashtable *domain_map;

/* NOTE: Keep in sync with LanguageCode enum.
 * It serves as an index into this table
 */
/** language data for all supported languages */
static const struct {
  const char *lang;	/* iso language-specification */
  const char *charset;	/* charset for output in lgeneral */
}
lang_data[] = {
  { "C",  "ISO-8859-1" },
  { "de", "ISO-8859-1" },
  { "en", "ISO-8859-1" },
};

/** deletes a domain-info-entry */
static void domain_info_delete(void *d) {
  free(d);
}

/** returns the domain-map */
static inline struct hashtable *domain_map_instance() {
  if (!domain_map) {
    domain_map = create_hashtable(10, (unsigned int (*) (void*))hash_string,
                              (int (*)(void *, void *))strcmp,
                              domain_info_delete);
  }
  return domain_map;
}

/** inserts a new entry into the domain-map */
static void domain_map_insert(const char *domain, struct DomainInfo *info) {
  struct hashtable *dm = domain_map_instance();
  hashtable_insert(dm, (void *)domain, info);
}

#ifdef __NOTUSED
/** removes an entry from the domain-map */
static void domain_map_remove(const char *domain) {
  struct hashtable *dm = domain_map_instance();
  hashtable_remove(dm, (void *)domain);
}
#endif

#if 0
const char *tr(const char *s) {
  const char *r = gettext(s);
  fprintf(stderr, "'%s' -> '%s'\n", s, r);
  return r;
}
#endif

/**
 * Looks up an entry in the domain-map.
 * Returns \c DomainInfo or 0 if not found.
 */
static struct DomainInfo *domain_map_get(const char *domain) {
  struct hashtable *dm = domain_map_instance();
  return hashtable_search(dm, (void *)domain);
}

/**
 * Determines from the given language-string a suitable
 * language-code, or LC_C if no match.
 */
static LanguageCode locale_determine_internal_code(const char *str) {
  unsigned len = strlen(str);
  char *buf = alloca(len + 1);
  char *pos;

  /* make a copy we can manipulate at will */
  strcpy(buf, str);

  /* format can be of ll_TT.ENCODING. Strip .ENCODING */
  pos = strchr(buf, '.');
  len = pos ? pos - buf : len;
  buf[len] = 0;

  do {
    /* In the first pass, try to match ll_TT.
     * If this fails, strip _TT and match again.
     */
    int i;
    for (i = 0; i < sizeof lang_data/sizeof lang_data[0]; i++) {
      if (strcmp(lang_data[i].lang, buf) == 0)
        return (LanguageCode)i;
    }

    /* strip _TT */
    pos = strchr(buf, '_');
    len = pos ? pos - buf : len;
    buf[len] = 0;
  } while(pos);

  return LC_C;
}

int locale_load_domain(const char *domain, const char *translations) {
  struct DomainInfo *info;
  /* TODO: In the future resources may specify a package containing
   * all translations for a certain domain.
   * Currently, we rely on all translations being preinstalled.
   */
  assert(translations == 0);

  /* don't add any domain twice */
  info = domain_map_get(domain);
  if (info) return 1;

#ifdef ENABLE_NLS
  {
    char *path;
    const char suffix[] = "/share/locale";
    unsigned len = strlen(paths_prefix());
    path = malloc(len + sizeof suffix);
    strcpy(path, paths_prefix());
    strcpy(path + len, suffix);
    bindtextdomain(domain, path);
  }
#endif
  info = malloc(sizeof(struct DomainInfo));
  domain_map_insert(domain, info);
  return 1;
}

void locale_write_ordinal_number_lang(char *buf, unsigned n, LanguageCode code, int number) {

  switch (code) {
    case LC_DE:
      /* Ah! Is that easy :-) */
      snprintf( buf, n, "%d.", number);
      break;
    case LC_C:
    case LC_EN:
    default: {
      const int num100 = number % 100;
      switch ( num100 < 10 || num100 > 19 ? number % 10 : 4 ) {
        case 1: snprintf( buf, n, "%dst", number ); break;
        case 2: snprintf( buf, n, "%dnd", number ); break;
        case 3: snprintf( buf, n, "%drd", number ); break;
        default: snprintf( buf, n, "%dth", number ); break;
      }
      break;
    }
  }
}

void locale_write_ordinal_number(char *buf, unsigned n, int number) {
  locale_write_ordinal_number_lang(buf, n, lang_code, number);
}

void locale_init(const char *lang) {
  static const char *vars[] = { "LANGUAGE", "LC_ALL", "LC_MESSAGES", "LANG" };
  int i;

  if (lang) {
    setenv("LANGUAGE", lang, 1);
  }

  /* traverse envvars in same order like gettext and use
   * the first one found.
   */
  for (i = 0; i < sizeof vars/sizeof vars[0]; i++) {
    const char *lang = getenv(vars[i]);
    if (lang) {
      lang_code = locale_determine_internal_code(lang);
      break;
    }
  }

#if 0
  fprintf(stderr, "lc: %d, lang: %s\n", lang_code, lang_data[lang_code].str);
#endif

  setlocale(LC_MESSAGES, "");
  
  /* for glibc, force the output charset */
  setenv("OUTPUT_CHARSET", lang_data[lang_code].charset, 1);
  
  locale_load_domain(PACKAGE, 0);
#ifdef ENABLE_NLS
  textdomain(PACKAGE);
#endif
}

/* kate: tab-indents on; space-indent on; replace-tabs off; indent-width 2; dynamic-word-wrap off; inden(t)-mode cstyle */
