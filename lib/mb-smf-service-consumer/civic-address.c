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
#include "ogs-sbi.h"

#include "macros.h"
#include "json-patch.h"

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
void _civic_addresses_free(ogs_list_t *civic_addresses)
{
    if (!civic_addresses) return;
    _civic_addresses_clear(civic_addresses);
    ogs_free(civic_addresses);
}

void _civic_addresses_clear(ogs_list_t *civic_addresses)
{
    if (!civic_addresses) return;

    mb_smf_sc_civic_address_t *next, *addr;
    ogs_list_for_each_safe(civic_addresses, next, addr) {
        ogs_list_remove(civic_addresses, addr);
        _civic_address_free(addr);
    }
}

void _civic_addresses_copy_values(ogs_list_t *dst, const ogs_list_t *src)
{
    if (src && ogs_list_count(src) == 0) src = NULL;

    if (!dst) return;

    _civic_addresses_clear(dst);

    if (!src) return;

    mb_smf_sc_civic_address_t *addr;
    ogs_list_for_each(src, addr) {
        mb_smf_sc_civic_address_t *new_addr = NULL;
        _civic_address_copy(&new_addr, addr);
        ogs_list_add(dst, new_addr);
    }

}

void _civic_addresses_copy(ogs_list_t **dst, const ogs_list_t *src)
{
    if (src && ogs_list_count(src) == 0) src = NULL;
    if (!src) {
        if (*dst) {
            _civic_addresses_free(*dst);
            *dst = NULL;
        }
        return;
    }
    
    if (!*dst) {
        *dst = (typeof(*dst))ogs_calloc(1, sizeof(**dst));
    }
    _civic_addresses_copy_values(*dst, src);
}

bool _civic_addresses_equal(const ogs_list_t *a, const ogs_list_t *b)
{
    if (a && ogs_list_count(a) == 0) a = NULL;
    if (b && ogs_list_count(b) == 0) b = NULL;
    if (a == b) return true;
    if (!a || !b) return false;
    if (ogs_list_count(a) != ogs_list_count(b)) return false;

    ogs_list_t *b_copy = NULL;
    _civic_addresses_copy(&b_copy, b);

    mb_smf_sc_civic_address_t *a_addr;
    ogs_list_for_each(a, a_addr) {
        mb_smf_sc_civic_address_t *next, *b_addr;
        bool found = false;
        ogs_list_for_each_safe(b_copy, next, b_addr) {
            if (_civic_address_equal(a_addr, b_addr)) {
                ogs_list_remove(b_copy, b_addr);
                _civic_address_free(b_addr);
                found = true;
                break;
            }
        }
        if (!found) {
            _civic_addresses_free(b_copy);
            return false;
        }
    }

    _civic_addresses_free(b_copy);
    return true;
}

ogs_list_t *_civic_addresses_patch_list(const ogs_list_t *a, const ogs_list_t *b)
{
    ogs_list_t *patches = NULL;

    if (a && ogs_list_count(a) == 0) a = NULL;
    if (b && ogs_list_count(b) == 0) b = NULL;

    if (a != b) {
        if (!a) {
            _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation_add, "/", _civic_addresses_to_json(b));
            patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
            ogs_list_add(patches, patch);
        } else if (!b) {
            _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation__remove, "/", NULL);
            patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
            ogs_list_add(patches, patch);
        } else {
            int idx = 0;
            mb_smf_sc_civic_address_t *a_addr = (mb_smf_sc_civic_address_t*)ogs_list_first(a);
            mb_smf_sc_civic_address_t *b_addr = (mb_smf_sc_civic_address_t*)ogs_list_first(b);
            while (a_addr || b_addr) {
                _json_patch_t *patch = NULL;
                if (!a_addr) {
                    patch = _json_patch_new(OpenAPI_patch_operation_add, "/-", _civic_address_to_json(b_addr));
                } else if (!b_addr) {
                    char *path = ogs_msprintf("/%i", idx);
                    patch = _json_patch_new(OpenAPI_patch_operation__remove, path, NULL);
                    ogs_free(path);
                    idx--;
                } else if (!_civic_address_equal(a_addr, b_addr)) {
                    char *path = ogs_msprintf("/%i", idx);
                    patch = _json_patch_new(OpenAPI_patch_operation_replace, path, _civic_address_to_json(b_addr));
                    ogs_free(path);
                }
                if (patch) {
                    if (!patches) patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
                    ogs_list_add(patches, patch);
                }
                idx++;   
                if (a_addr) a_addr = (mb_smf_sc_civic_address_t*)ogs_list_next(a_addr);
                if (b_addr) b_addr = (mb_smf_sc_civic_address_t*)ogs_list_next(b_addr);
            }
        }
    }

    return patches;
}

OpenAPI_list_t *_civic_addresses_to_openapi(const ogs_list_t *civic_addresses)
{
    if (!civic_addresses || ogs_list_count(civic_addresses) == 0) return NULL;

    OpenAPI_list_t *list = OpenAPI_list_create();

    mb_smf_sc_civic_address_t *addr;
    ogs_list_for_each(civic_addresses, addr) {
        OpenAPI_list_add(list, _civic_address_to_openapi(addr));
    }

    return list;
}

cJSON *_civic_addresses_to_json(const ogs_list_t *civic_addresses)
{
    if (!civic_addresses) return NULL;

    cJSON *json = cJSON_CreateArray();

    mb_smf_sc_civic_address_t *addr;
    ogs_list_for_each(civic_addresses, addr) {
        cJSON_AddItemToArray(json, _civic_address_to_json(addr));
    }

    return json;
}

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

ogs_list_t *_civic_address_patch_list(const mb_smf_sc_civic_address_t *a, const mb_smf_sc_civic_address_t *b)
{
    ogs_list_t *patches = NULL;

    if (a != b) {
        if (!a) {
            _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation_add, "/", _civic_address_to_json(b));
            patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
            ogs_list_add(patches, patch);
        } else if (!b) {
            _json_patch_t *patch = _json_patch_new(OpenAPI_patch_operation__remove, "/", NULL);
            patches = (typeof(patches))ogs_calloc(1, sizeof(*patches));
            ogs_list_add(patches, patch);
        } else {
#define PATCH_FIELD(field, name) \
            do { \
                if (a->field != b->field) { \
                    _json_patch_t *patch = NULL; \
                    if (!a->field) { \
                        patch = _json_patch_new(OpenAPI_patch_operation_add, "/" name, cJSON_CreateString(b->field)); \
                    } else if (!b->field) { \
                        patch = _json_patch_new(OpenAPI_patch_operation__remove, "/" name, NULL); \
                    } else if (strcmp(a->field, b->field)) { \
                        patch = _json_patch_new(OpenAPI_patch_operation_replace, "/" name, cJSON_CreateString(b->field)); \
                    } \
                    if (patch) { \
                        if (!patches) patches = (typeof(patches))ogs_calloc(1, sizeof(*patches)); \
                        ogs_list_add(patches, patch); \
                    } \
                } \
            } while (0)

            PATCH_FIELD(country, "country");
            PATCH_FIELD(a[0], "A1");
            PATCH_FIELD(a[1], "A2");
            PATCH_FIELD(a[2], "A3");
            PATCH_FIELD(a[3], "A4");
            PATCH_FIELD(a[4], "A5");
            PATCH_FIELD(a[5], "A6");
            PATCH_FIELD(prd, "PRD");
            PATCH_FIELD(pod, "POD");
            PATCH_FIELD(sts, "STS");
            PATCH_FIELD(hno, "HNO");
            PATCH_FIELD(hns, "HNS");
            PATCH_FIELD(lmk, "LMK");
            PATCH_FIELD(loc, "LOC");
            PATCH_FIELD(nam, "NAM");
            PATCH_FIELD(pc, "PC");
            PATCH_FIELD(bld, "BLD");
            PATCH_FIELD(unit, "UNIT");
            PATCH_FIELD(flr, "FLR");
            PATCH_FIELD(room, "ROOM");
            PATCH_FIELD(plc, "PLC");
            PATCH_FIELD(pcn, "PCN");
            PATCH_FIELD(pobox, "POBOX");
            PATCH_FIELD(addcode, "ADDCODE");
            PATCH_FIELD(seat, "SEAT");
            PATCH_FIELD(rd, "RD");
            PATCH_FIELD(rdsec, "RDSEC");
            PATCH_FIELD(rdbr, "RDBR");
            PATCH_FIELD(rdsubbr, "RDSUBBR");
            PATCH_FIELD(prm, "PRM");
            PATCH_FIELD(pom, "POM");
            PATCH_FIELD(usage_rules, "usageRules");
            PATCH_FIELD(method, "method");
            PATCH_FIELD(provided_by, "providedBy");

#undef PATCH_FIELD
        }
    }

    return patches;
}

OpenAPI_civic_address_t *_civic_address_to_openapi(const mb_smf_sc_civic_address_t *civic_address)
{
    if (!civic_address) return NULL;
    return OpenAPI_civic_address_create(
            ogs_strdup(civic_address->country),
            civic_address->a[0]?ogs_strdup(civic_address->a[0]):NULL,
            civic_address->a[1]?ogs_strdup(civic_address->a[1]):NULL,
            civic_address->a[2]?ogs_strdup(civic_address->a[2]):NULL,
            civic_address->a[3]?ogs_strdup(civic_address->a[3]):NULL,
            civic_address->a[4]?ogs_strdup(civic_address->a[4]):NULL,
            civic_address->a[5]?ogs_strdup(civic_address->a[5]):NULL,
            civic_address->prd?ogs_strdup(civic_address->prd):NULL,
            civic_address->pod?ogs_strdup(civic_address->pod):NULL,
            civic_address->sts?ogs_strdup(civic_address->sts):NULL,
            civic_address->hno?ogs_strdup(civic_address->hno):NULL,
            civic_address->hns?ogs_strdup(civic_address->hns):NULL,
            civic_address->lmk?ogs_strdup(civic_address->lmk):NULL,
            civic_address->loc?ogs_strdup(civic_address->loc):NULL,
            civic_address->nam?ogs_strdup(civic_address->nam):NULL,
            civic_address->pc?ogs_strdup(civic_address->pc):NULL,
            civic_address->bld?ogs_strdup(civic_address->bld):NULL,
            civic_address->unit?ogs_strdup(civic_address->unit):NULL,
            civic_address->flr?ogs_strdup(civic_address->flr):NULL,
            civic_address->room?ogs_strdup(civic_address->room):NULL,
            civic_address->plc?ogs_strdup(civic_address->plc):NULL,
            civic_address->pcn?ogs_strdup(civic_address->pcn):NULL,
            civic_address->pobox?ogs_strdup(civic_address->pobox):NULL,
            civic_address->addcode?ogs_strdup(civic_address->addcode):NULL,
            civic_address->seat?ogs_strdup(civic_address->seat):NULL,
            civic_address->rd?ogs_strdup(civic_address->rd):NULL,
            civic_address->rdsec?ogs_strdup(civic_address->rdsec):NULL,
            civic_address->rdbr?ogs_strdup(civic_address->rdbr):NULL,
            civic_address->rdsubbr?ogs_strdup(civic_address->rdsubbr):NULL,
            civic_address->prm?ogs_strdup(civic_address->prm):NULL,
            civic_address->pom?ogs_strdup(civic_address->pom):NULL,
            civic_address->usage_rules?ogs_strdup(civic_address->usage_rules):NULL,
            civic_address->method?ogs_strdup(civic_address->method):NULL,
            civic_address->provided_by?ogs_strdup(civic_address->provided_by):NULL
        );
}

cJSON *_civic_address_to_json(const mb_smf_sc_civic_address_t *civic_address)
{
    if (!civic_address) return NULL;

    OpenAPI_civic_address_t *api_addr = _civic_address_to_openapi(civic_address);
    if (!api_addr) return NULL;

    cJSON *json = OpenAPI_civic_address_convertToJSON(api_addr);
    OpenAPI_civic_address_free(api_addr);

    return json;
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
