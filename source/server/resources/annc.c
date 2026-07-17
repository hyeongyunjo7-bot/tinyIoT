#include <stdlib.h>
#include <string.h>
#include "../onem2m.h"
#include "../logger.h"
#include "../util.h"
#include "../dbmanager.h"
#include "../config.h"

extern ResourceTree *rt;
extern cJSON *ATTRIBUTES;

static bool is_standard_fcnta_attribute(const char *attr)
{
    if (!attr)
        return true;

    const char *fixed[] = {
        "rn", "ri", "pi", "ct", "lt", "ty", "acpi", "lbl", "loc", "et", "memberOf",
        "lnk", "cnd", "nl", "mni", "mbs", "mia", "ast", "daci", "custom_attrs",
        NULL
    };

    for (int i = 0; fixed[i]; i++)
    {
        if (strcmp(attr, fixed[i]) == 0)
            return true;
    }

    return false;
}

static cJSON *extract_fcnta_custom_attributes(cJSON *fcnta)
{
    if (!fcnta)
        return NULL;

    cJSON *custom_attrs = cJSON_CreateObject();
    cJSON *item = NULL;
    cJSON_ArrayForEach(item, fcnta)
    {
        if (item->string && !is_standard_fcnta_attribute(item->string))
        {
            cJSON_AddItemToObject(custom_attrs, item->string, cJSON_Duplicate(item, true));
        }
    }

    if (cJSON_GetArraySize(custom_attrs) == 0)
    {
        cJSON_Delete(custom_attrs);
        return NULL;
    }

    return custom_attrs;
}

static void merge_fcnta_custom_attributes(cJSON *target, cJSON *custom_attrs)
{
    if (!target || !custom_attrs)
        return;

    cJSON *item = NULL;
    cJSON_ArrayForEach(item, custom_attrs)
    {
        if (!item->string)
            continue;

        cJSON_DeleteItemFromObject(target, item->string);
        if (!cJSON_IsNull(item))
        {
            cJSON_AddItemToObject(target, item->string, cJSON_Duplicate(item, true));
        }
    }
}

static cJSON *merged_fcnta_custom_attributes(cJSON *target, cJSON *updates)
{
    cJSON *merged = NULL;
    cJSON *ri = cJSON_GetObjectItem(target, "ri");
    if (ri && cJSON_IsString(ri))
    {
        merged = db_get_fcnta_custom_attributes(ri->valuestring);
    }
    if (!merged)
    {
        merged = extract_fcnta_custom_attributes(target);
    }
    if (!merged)
    {
        merged = cJSON_CreateObject();
    }

    cJSON *item = NULL;
    cJSON_ArrayForEach(item, updates)
    {
        if (!item->string)
            continue;

        cJSON_DeleteItemFromObject(merged, item->string);
        if (!cJSON_IsNull(item))
        {
            cJSON_AddItemToObject(merged, item->string, cJSON_Duplicate(item, true));
        }
    }

    return merged;
}

static int validate_fcnta_create(oneM2MPrimitive *o2pt, cJSON *root)
{
    if (!o2pt->request_pc || !cJSON_IsObject(o2pt->request_pc))
    {
        return handle_error(o2pt, RSC_BAD_REQUEST, "invalid announced resource payload");
    }

    if (!root || !cJSON_IsObject(root))
    {
        return handle_error(o2pt, RSC_BAD_REQUEST, "invalid announced resource payload");
    }

    cJSON *resource = cJSON_GetObjectItemCaseSensitive(root, "m2m:fcntA");
    if (!resource)
    {
        return handle_error(o2pt, RSC_BAD_REQUEST, "invalid announced resource payload");
    }

    if (!cJSON_IsObject(resource))
    {
        return handle_error(o2pt, RSC_BAD_REQUEST, "invalid announced resource payload");
    }

    if (parse_object_type_cjson(root) != RT_FCNTA)
    {
        return handle_error(o2pt, RSC_BAD_REQUEST, "resource type mismatch");
    }

    cJSON *lnk = cJSON_GetObjectItem(resource, "lnk");
    if (!lnk || !cJSON_IsString(lnk) || is_blank_string(lnk->valuestring))
    {
        return handle_error(o2pt, RSC_BAD_REQUEST, "lnk is empty");
    }

    cJSON *rn = cJSON_GetObjectItem(resource, "rn");
    if (rn && (!cJSON_IsString(rn) || is_blank_string(rn->valuestring)))
    {
        return handle_error(o2pt, RSC_BAD_REQUEST, "attribute `rn` is invalid");
    }

    return RSC_OK;
}

int create_annc(oneM2MPrimitive *o2pt, RTNode *parent_rtnode)
{
    cJSON *custom_attrs = NULL;

    // type specific validation
    switch (o2pt->ty)
    {
    case RT_ACPA:
        break;
    case RT_AEA:
        check_aei_invalid(o2pt);
        break;
    case RT_CBA:
        break;
    case RT_CNTA:
        break;
    case RT_GRPA:
        break;
    case RT_CINA:
        break;
    case RT_FCNTA:
    {
        int rsc = validate_fcnta_create(o2pt, o2pt->request_pc);
        if (rsc != RSC_OK)
            return rsc;
    }
    break;
    }

    cJSON *root = cJSON_Duplicate(o2pt->request_pc, 1);
    cJSON *resource = cJSON_GetObjectItem(root, get_resource_key(o2pt->ty));
    if (!resource || !cJSON_IsObject(resource))
    {
        cJSON_Delete(root);
        return handle_error(o2pt, RSC_BAD_REQUEST, "invalid announced resource payload");
    }

    if (o2pt->ty == RT_FCNTA)
    {
        custom_attrs = extract_fcnta_custom_attributes(resource);
    }

    add_general_attribute(resource, parent_rtnode, o2pt->ty);

    int rsc = RSC_OK;
    if (rsc != RSC_OK)
    {
        cJSON_Delete(root);
        return rsc;
    }

    // Add uri attribute
    char *ptr = malloc(1024);
    cJSON *rn = cJSON_GetObjectItem(resource, "rn");
    if (!ptr)
    {
        cJSON_Delete(root);
        return handle_error(o2pt, RSC_INTERNAL_SERVER_ERROR, "Memory allocation failed");
    }
    if (!rn || !rn->valuestring)
    {
        free(ptr);
        cJSON_Delete(root);
        return handle_error(o2pt, RSC_BAD_REQUEST, "attribute `rn` is invalid");
    }
    sprintf(ptr, "%s/%s", get_uri_rtnode(parent_rtnode), rn->valuestring);
    // Save to DB
    int result = db_store_resource(resource, ptr);

    if (result != 1)
    {
        if (custom_attrs)
            cJSON_Delete(custom_attrs);
        handle_error(o2pt, RSC_INTERNAL_SERVER_ERROR, "DB store fail");
        cJSON_Delete(root);

        free(ptr);
        ptr = NULL;
        return RSC_INTERNAL_SERVER_ERROR;
    }

    if (custom_attrs)
    {
        cJSON *ri = cJSON_GetObjectItem(resource, "ri");
        if (ri && cJSON_IsString(ri))
        {
            db_store_fcnta_custom_attributes(ri->valuestring, custom_attrs);
        }
    }

    free(ptr);
    ptr = NULL;

    // Add to resource tree
    RTNode *child_rtnode = create_rtnode(resource, o2pt->ty);
    add_child_resource_tree(parent_rtnode, child_rtnode);

    make_response_body(o2pt, child_rtnode);
    cJSON_DetachItemFromObject(root, get_resource_key(o2pt->ty));
    cJSON_Delete(root);
    if (custom_attrs)
        cJSON_Delete(custom_attrs);

    return o2pt->rsc = RSC_CREATED;
}

int update_annc(oneM2MPrimitive *o2pt, RTNode *target_rtnode)
{
    int rsc;
    char invalid_key[][8] = {"ty", "pi", "ri", "rn", "ct"};
    cJSON *custom_attrs = NULL;
    if (!o2pt->request_pc || !cJSON_IsObject(o2pt->request_pc))
    {
        return handle_error(o2pt, RSC_BAD_REQUEST, "invalid announced resource payload");
    }
    cJSON *req_src = NULL;
    if (o2pt->ty == RT_FCNTA)
    {
        req_src = cJSON_GetObjectItemCaseSensitive(o2pt->request_pc, get_resource_key(o2pt->ty));
    }
    else
    {
        req_src = cJSON_GetObjectItem(o2pt->request_pc, get_resource_key(o2pt->ty));
    }
	if (!req_src)
	{
		return handle_error(o2pt, RSC_BAD_REQUEST, "invalid announced resource payload");
	}
    if (!cJSON_IsObject(req_src))
    {
        return handle_error(o2pt, RSC_BAD_REQUEST, "invalid announced resource payload");
    }
    if (o2pt->ty == RT_FCNTA && parse_object_type_cjson(o2pt->request_pc) != o2pt->ty)
    {
        return handle_error(o2pt, RSC_BAD_REQUEST, "resource type mismatch");
    }
    int invalid_key_size = sizeof(invalid_key) / (8 * sizeof(char));
    for (int i = 0; i < invalid_key_size; i++)
    {
        if (cJSON_GetObjectItem(req_src, invalid_key[i]))
        {
            return handle_error(o2pt, RSC_BAD_REQUEST, "unsupported attribute on update");
        }
    }
	cJSON *lnk_item = cJSON_GetObjectItem(target_rtnode->obj, "lnk");
	if (!lnk_item || !cJSON_IsString(lnk_item) || is_blank_string(lnk_item->valuestring))
	{
		return handle_error(o2pt, RSC_BAD_REQUEST, "lnk is empty");
	}
	char *lnk = lnk_item->valuestring;
	logger("UTIL", LOG_LEVEL_DEBUG, "lnk : %s, fr : %s", lnk, o2pt->fr);
    if (strncmp(o2pt->fr, lnk, strlen(o2pt->fr)) == 0 && lnk[strlen(o2pt->fr)] == '/')
    {
        logger("UTIL", LOG_LEVEL_DEBUG, "update from originator");
    }
    else
    {
        cJSON *ast = cJSON_GetObjectItem(target_rtnode->obj, "ast");
        if (ast)
        {
            if (ast->valueint != AST_BI_DIRECTIONAL)
            {
                return handle_error(o2pt, RSC_BAD_REQUEST, "resource is uni-directional");
            }
            else
            {
                if (req_src->child == NULL){
                    logger("UTIL", LOG_LEVEL_DEBUG, "Empty update payload, skipping forwarding");
                }
                else {

                    oneM2MPrimitive *req = calloc(1, sizeof(oneM2MPrimitive));
                    o2ptcpy(&req, o2pt);
                    req->to = strdup(lnk);
                    cJSON_Delete(req->request_pc);
                    req->request_pc = cJSON_CreateObject();
                    cJSON_AddItemReferenceToObject(req->request_pc, get_resource_key(o2pt->ty - 10000), req_src);

                    rsc = forwarding_onem2m_resource(req, find_csr_rtnode_by_uri(lnk));
                    if (rsc != RSC_UPDATED)
                    {
                        return handle_error(o2pt, RSC_BAD_REQUEST, "failed to update original resource");
                    }
                }
            }
        }
        else
        {
            return handle_error(o2pt, RSC_BAD_REQUEST, "resource is uni-directional");
        }
    }

    int result = 0;

    cJSON *aea = target_rtnode->obj;
    cJSON *pjson = NULL;

    if (o2pt->ty == RT_FCNTA)
    {
        custom_attrs = extract_fcnta_custom_attributes(req_src);
    }

    update_resource(target_rtnode->obj, req_src);

    result = db_update_resource(req_src, cJSON_GetObjectItem(target_rtnode->obj, "ri")->valuestring, target_rtnode->ty);

    if (o2pt->ty == RT_FCNTA && custom_attrs)
    {
        cJSON *ri = cJSON_GetObjectItem(target_rtnode->obj, "ri");
        cJSON *merged = merged_fcnta_custom_attributes(target_rtnode->obj, custom_attrs);
        merge_fcnta_custom_attributes(target_rtnode->obj, custom_attrs);
        if (ri && cJSON_IsString(ri) && merged)
        {
            db_update_fcnta_custom_attributes(ri->valuestring, merged);
        }
        if (merged)
            cJSON_Delete(merged);
        cJSON_Delete(custom_attrs);
    }

    make_response_body(o2pt, target_rtnode);

    o2pt->rsc = RSC_UPDATED;
    return RSC_UPDATED;
}
