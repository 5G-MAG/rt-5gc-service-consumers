/*****************************************************************************
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */
#include "ogs-core.h"

#include "macros.h"

#include "civic-address.h"
#include "priv_civic-address.h"

/* mb_smf_sc_civic_address Type functions */

MB_SMF_CLIENT_API mb_smf_sc_civic_address_t *mb_smf_sc_civic_address_new()
{
    return _civic_address_new();
}

MB_SMF_CLIENT_API void mb_smf_sc_civic_address_delete(mb_smf_sc_civic_address_t *address)
{
    _civic_address_free(address);
}

/* Library internal civic_address methods (protected) */
mb_smf_sc_civic_address_t *_civic_address_new()
{
    return (mb_smf_sc_civic_address_t*)ogs_calloc(1,sizeof(mb_smf_sc_civic_address_t));
}

void _civic_address_free(mb_smf_sc_civic_address_t *civic_address)
{
    if (!civic_address) return;
    _civic_address_clear(civic_address);
    ogs_free(civic_address);
}

void _civic_address_clear(mb_smf_sc_civic_address_t *civic_address)
{
    if (!civic_address) return;
    if (civic_address->country) { ogs_free(civic_address->country); civic_address->country = NULL; }
    if (civic_address->a[0]) { ogs_free(civic_address->a[0]); civic_address->a[0] = NULL; }
    if (civic_address->a[1]) { ogs_free(civic_address->a[1]); civic_address->a[1] = NULL; }
    if (civic_address->a[2]) { ogs_free(civic_address->a[2]); civic_address->a[2] = NULL; }
    if (civic_address->a[3]) { ogs_free(civic_address->a[3]); civic_address->a[3] = NULL; }
    if (civic_address->a[4]) { ogs_free(civic_address->a[4]); civic_address->a[4] = NULL; }
    if (civic_address->a[5]) { ogs_free(civic_address->a[5]); civic_address->a[5] = NULL; }
    if (civic_address->prd) { ogs_free(civic_address->prd); civic_address->prd = NULL; }
    if (civic_address->pod) { ogs_free(civic_address->pod); civic_address->pod = NULL; }
    if (civic_address->sts) { ogs_free(civic_address->sts); civic_address->sts = NULL; }
    if (civic_address->hno) { ogs_free(civic_address->hno); civic_address->hno = NULL; }
    if (civic_address->hns) { ogs_free(civic_address->hns); civic_address->hns = NULL; }
    if (civic_address->lmk) { ogs_free(civic_address->lmk); civic_address->lmk = NULL; }
    if (civic_address->loc) { ogs_free(civic_address->loc); civic_address->loc = NULL; }
    if (civic_address->nam) { ogs_free(civic_address->nam); civic_address->nam = NULL; }
    if (civic_address->pc) { ogs_free(civic_address->pc); civic_address->pc = NULL; }
    if (civic_address->bld) { ogs_free(civic_address->bld); civic_address->bld = NULL; }
    if (civic_address->unit) { ogs_free(civic_address->unit); civic_address->unit = NULL; }
    if (civic_address->flr) { ogs_free(civic_address->flr); civic_address->flr = NULL; }
    if (civic_address->room) { ogs_free(civic_address->room); civic_address->room = NULL; }
    if (civic_address->plc) { ogs_free(civic_address->plc); civic_address->plc = NULL; }
    if (civic_address->pcn) { ogs_free(civic_address->pcn); civic_address->pcn = NULL; }
    if (civic_address->pobox) { ogs_free(civic_address->pobox); civic_address->pobox = NULL; }
    if (civic_address->addcode) { ogs_free(civic_address->addcode); civic_address->addcode = NULL; }
    if (civic_address->seat) { ogs_free(civic_address->seat); civic_address->seat = NULL; }
    if (civic_address->rd) { ogs_free(civic_address->rd); civic_address->rd = NULL; }
    if (civic_address->rdsec) { ogs_free(civic_address->rdsec); civic_address->rdsec = NULL; }
    if (civic_address->rdbr) { ogs_free(civic_address->rdbr); civic_address->rdbr = NULL; }
    if (civic_address->rdsubbr) { ogs_free(civic_address->rdsubbr); civic_address->rdsubbr = NULL; }
    if (civic_address->prm) { ogs_free(civic_address->prm); civic_address->prm = NULL; }
    if (civic_address->pom) { ogs_free(civic_address->pom); civic_address->pom = NULL; }
    if (civic_address->usage_rules) { ogs_free(civic_address->usage_rules); civic_address->usage_rules = NULL; }
    if (civic_address->method) { ogs_free(civic_address->method); civic_address->method = NULL; }
    if (civic_address->provided_by) { ogs_free(civic_address->provided_by); civic_address->provided_by = NULL; }
}

void _civic_address_copy(mb_smf_sc_civic_address_t **dst, const mb_smf_sc_civic_address_t *src)
{
    if (!src) {
        if (*dst) {
            _civic_address_free(*dst);
            *dst = NULL;
        }
        return;
    }

    if (!*dst) {
        *dst = _civic_address_new();
    } else {
        _civic_address_clear(*dst);
    }

    if (src->country) (*dst)->country = ogs_strdup(src->country);
    if (src->a[0]) (*dst)->a[0] = ogs_strdup(src->a[0]);
    if (src->a[1]) (*dst)->a[1] = ogs_strdup(src->a[1]);
    if (src->a[2]) (*dst)->a[2] = ogs_strdup(src->a[2]);
    if (src->a[3]) (*dst)->a[3] = ogs_strdup(src->a[3]);
    if (src->a[4]) (*dst)->a[4] = ogs_strdup(src->a[4]);
    if (src->a[5]) (*dst)->a[5] = ogs_strdup(src->a[5]);
    if (src->prd) (*dst)->prd = ogs_strdup(src->prd);
    if (src->pod) (*dst)->pod = ogs_strdup(src->pod);
    if (src->sts) (*dst)->sts = ogs_strdup(src->sts);
    if (src->hno) (*dst)->hno = ogs_strdup(src->hno);
    if (src->hns) (*dst)->hns = ogs_strdup(src->hns);
    if (src->lmk) (*dst)->lmk = ogs_strdup(src->lmk);
    if (src->loc) (*dst)->loc = ogs_strdup(src->loc);
    if (src->nam) (*dst)->nam = ogs_strdup(src->nam);
    if (src->pc) (*dst)->pc = ogs_strdup(src->pc);
    if (src->bld) (*dst)->bld = ogs_strdup(src->bld);
    if (src->unit) (*dst)->unit = ogs_strdup(src->unit);
    if (src->flr) (*dst)->flr = ogs_strdup(src->flr);
    if (src->room) (*dst)->room = ogs_strdup(src->room);
    if (src->plc) (*dst)->plc = ogs_strdup(src->plc);
    if (src->pcn) (*dst)->pcn = ogs_strdup(src->pcn);
    if (src->pobox) (*dst)->pobox = ogs_strdup(src->pobox);
    if (src->addcode) (*dst)->addcode = ogs_strdup(src->addcode);
    if (src->seat) (*dst)->seat = ogs_strdup(src->seat);
    if (src->rd) (*dst)->rd = ogs_strdup(src->rd);
    if (src->rdsec) (*dst)->rdsec = ogs_strdup(src->rdsec);
    if (src->rdbr) (*dst)->rdbr = ogs_strdup(src->rdbr);
    if (src->rdsubbr) (*dst)->rdsubbr = ogs_strdup(src->rdsubbr);
    if (src->prm) (*dst)->prm = ogs_strdup(src->prm);
    if (src->pom) (*dst)->pom = ogs_strdup(src->pom);
    if (src->usage_rules) (*dst)->usage_rules = ogs_strdup(src->usage_rules);
    if (src->method) (*dst)->method = ogs_strdup(src->method);
    if (src->provided_by) (*dst)->provided_by = ogs_strdup(src->provided_by);
}

bool _civic_address_equal(const mb_smf_sc_civic_address_t *a, const mb_smf_sc_civic_address_t *b)
{
    if (a == b) return true;
    if (!a || !b) return false;

    /* check pointers set */
    if (!a->country != !b->country) return false;
    if (!a->a[0] != !b->a[0]) return false;
    if (!a->a[1] != !b->a[1]) return false;
    if (!a->a[2] != !b->a[2]) return false;
    if (!a->a[3] != !b->a[3]) return false;
    if (!a->a[4] != !b->a[4]) return false;
    if (!a->a[5] != !b->a[5]) return false;
    if (!a->prd != !b->prd) return false;
    if (!a->pod != !b->pod) return false;
    if (!a->sts != !b->sts) return false;
    if (!a->hno != !b->hno) return false;
    if (!a->hns != !b->hns) return false;
    if (!a->lmk != !b->lmk) return false;
    if (!a->loc != !b->loc) return false;
    if (!a->nam != !b->nam) return false;
    if (!a->pc != !b->pc) return false;
    if (!a->bld != !b->bld) return false;
    if (!a->unit != !b->unit) return false;
    if (!a->flr != !b->flr) return false;
    if (!a->room != !b->room) return false;
    if (!a->plc != !b->plc) return false;
    if (!a->pcn != !b->pcn) return false;
    if (!a->pobox != !b->pobox) return false;
    if (!a->addcode != !b->addcode) return false;
    if (!a->seat != !b->seat) return false;
    if (!a->rd != !b->rd) return false;
    if (!a->rdsec != !b->rdsec) return false;
    if (!a->rdbr != !b->rdbr) return false;
    if (!a->rdsubbr != !b->rdsubbr) return false;
    if (!a->prm != !b->prm) return false;
    if (!a->pom != !b->pom) return false;
    if (!a->usage_rules != !b->usage_rules) return false;
    if (!a->method != !b->method) return false;
    if (!a->provided_by != !b->provided_by) return false;

    /* compare strings */
    if (a->country && strcmp(a->country, b->country)) return false;
    if (a->a[0] && strcmp(a->a[0], b->a[0])) return false;
    if (a->a[1] && strcmp(a->a[1], b->a[1])) return false;
    if (a->a[2] && strcmp(a->a[2], b->a[2])) return false;
    if (a->a[3] && strcmp(a->a[3], b->a[3])) return false;
    if (a->a[4] && strcmp(a->a[4], b->a[4])) return false;
    if (a->a[5] && strcmp(a->a[5], b->a[5])) return false;
    if (a->prd && strcmp(a->prd, b->prd)) return false;
    if (a->pod && strcmp(a->pod, b->pod)) return false;
    if (a->sts && strcmp(a->sts, b->sts)) return false;
    if (a->hno && strcmp(a->hno, b->hno)) return false;
    if (a->hns && strcmp(a->hns, b->hns)) return false;
    if (a->lmk && strcmp(a->lmk, b->lmk)) return false;
    if (a->loc && strcmp(a->loc, b->loc)) return false;
    if (a->nam && strcmp(a->nam, b->nam)) return false;
    if (a->pc && strcmp(a->pc, b->pc)) return false;
    if (a->bld && strcmp(a->bld, b->bld)) return false;
    if (a->unit && strcmp(a->unit, b->unit)) return false;
    if (a->flr && strcmp(a->flr, b->flr)) return false;
    if (a->room && strcmp(a->room, b->room)) return false;
    if (a->plc && strcmp(a->plc, b->plc)) return false;
    if (a->pcn && strcmp(a->pcn, b->pcn)) return false;
    if (a->pobox && strcmp(a->pobox, b->pobox)) return false;
    if (a->addcode && strcmp(a->addcode, b->addcode)) return false;
    if (a->seat && strcmp(a->seat, b->seat)) return false;
    if (a->rd && strcmp(a->rd, b->rd)) return false;
    if (a->rdsec && strcmp(a->rdsec, b->rdsec)) return false;
    if (a->rdbr && strcmp(a->rdbr, b->rdbr)) return false;
    if (a->rdsubbr && strcmp(a->rdsubbr, b->rdsubbr)) return false;
    if (a->prm && strcmp(a->prm, b->prm)) return false;
    if (a->pom && strcmp(a->pom, b->pom)) return false;
    if (a->usage_rules && strcmp(a->usage_rules, b->usage_rules)) return false;
    if (a->method && strcmp(a->method, b->method)) return false;
    if (a->provided_by && strcmp(a->provided_by, b->provided_by)) return false;

    return true;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
