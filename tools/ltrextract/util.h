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

#ifndef LTREXTRACT_UTIL_H
#define LTREXTRACT_UTIL_H

struct Translateables;
struct TranslateablesIterator;
struct PData;

/** Creates a new set of translateables */
struct Translateables *translateables_create(void);

/** Deletes set of translateables */
void translateables_delete(struct Translateables *);

/**
 * Returns the comment if the given translateable already is contained.
 * @param xl set of translateables
 * @param key translateable to look up
 * @param len if nonzero, will be written to the length of the returned
 * string (without trailing null). Will not be initialised if key not found.
 *
 * Note: If no comment has ever been added, the function will return the
 * empty string if contained. If and only if the translateable is not
 * contained, 0 is returned.
 */
const char *translateables_get(struct Translateables *xl, const char *key, unsigned *len);

/** 
 * Adds a translateable to the set of translateables.
 *
 * @param xl set of translateables
 * @param key translateable string which will serve as a key for gettext
 * @param comment any comment describing \c key. Will be appended in verbatim
 * if \c key already exists. May be 0 for no comment.
 */
void translateables_add(struct Translateables *xl, const char *key, const char *comment);

/**
 * Adds a translateable from the given pdata-value.
 *
 * This is a convenience function. If the translateable is not yet contained,
 * it is added. Context information is added to the translateable
 * as a C-comment.
 */
void translateables_add_pdata(struct Translateables *xl, struct PData *pd);

/**
 * Sets the domain for all translateables.
 *
 * Only resources which belong to this domain are considered for extraction.
 */
void translateables_set_domain(struct Translateables *xl, const char *domain);

/**
 * Returns the domain for all translateables.
 */
const char *translateables_get_domain(struct Translateables *xl);

/**
 * Returns an iterator onto the given translateables set.
 */
struct TranslateablesIterator *translateables_iterator(struct Translateables *xl);

/**
 * Returns the current key.
 */
const char *translateables_iterator_key(struct TranslateablesIterator *it);

/**
 * Returns the current comment.
 */
const char *translateables_iterator_comment(struct TranslateablesIterator *it);

/**
 * Advances to the next entry. Returns 0 if end of set reached.
 */
int translateables_iterator_advance(struct TranslateablesIterator *it);

/**
 * Frees the iterator.
 */
void translateables_iterator_delete(struct TranslateablesIterator *it);

/**
 * Returns whether the resource matches the translatables' domain
 */
int resource_matches_domain(struct PData *pd, struct Translateables *xl);

#endif /*LTREXTRACT_UTIL_H*/

/* kate: tab-indents on; space-indent on; replace-tabs off; indent-width 2; dynamic-word-wrap off; inden(t)-mode cstyle */
