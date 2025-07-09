/*****************************************************************************
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */
#include <stdbool.h>

#include "ogs-core.h"

#include "macros.h"
#include "priv_mbs-session.h"

#include "context.h"

typedef struct _context_s {
    ogs_list_t mbs_sessions; /* item type is _priv_mbs_session_t */
} _context_t;

static _context_t *__self = NULL;

_context_t *_context_new()
{
    __self = (_context_t*)ogs_calloc(1, sizeof(_context_t));
    return __self;
}

void _context_destroy()
{
    _priv_mbs_session_t *sess, *next;

    if (!__self) return;

    ogs_list_for_each_safe(&__self->mbs_sessions, next, sess) {
        _context_remove_mbs_session(sess);
    }

    ogs_free(__self);
    __self = NULL;
}

bool _context_add_mbs_session(_priv_mbs_session_t *session)
{
    if (!__self || !session) return false;

    ogs_list_add(&__self->mbs_sessions, session);

    return true;
}

bool _context_remove_mbs_session(_priv_mbs_session_t *session)
{
    if (!__self || !session) return false;

    ogs_list_remove(&__self->mbs_sessions, session);
    _mbs_session_delete(session);

    return true;
}

ogs_list_t *_context_mbs_sessions()
{
    if (!__self) return NULL;
    return &__self->mbs_sessions;
}

bool _context_active_sessions_exists(_priv_mbs_session_t *session)
{
    _priv_mbs_session_t *sess;

    if (!__self) return false;
    if (!session) return false;

    ogs_list_for_each(&__self->mbs_sessions, sess) {
        if (sess == session) return true;
    }

    return false;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
