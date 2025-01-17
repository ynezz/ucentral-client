/* SPDX-License-Identifier: BSD-3-Clause */

#include "ucentral.h"

#include <libubus.h>

static struct ubus_auto_conn conn;
static struct blob_buf u;

static int ubus_status_cb(struct ubus_context *ctx,
			  struct ubus_object *obj,
			  struct ubus_request_data *req,
			  const char *method, struct blob_attr *msg)
{
	time_t delta = time(NULL) - conn_time;

	blob_buf_init(&u, 0);
	blobmsg_add_u32(&u, websocket ? "connected" : "disconnected", delta);
	blobmsg_add_u32(&u, "latest", uuid_latest);
	blobmsg_add_u32(&u, "active", uuid_active);
	ubus_send_reply(ctx, req, u.head);

	return UBUS_STATUS_OK;
}

static int ubus_send_cb(struct ubus_context *ctx,
			struct ubus_object *obj,
			struct ubus_request_data *req,
			const char *method, struct blob_attr *msg)
{
	if (!msg)
		return UBUS_STATUS_INVALID_ARGUMENT;

	raw_send(msg);

	return UBUS_STATUS_OK;
}

static int ubus_log_cb(struct ubus_context *ctx,
		       struct ubus_object *obj,
		       struct ubus_request_data *req,
		       const char *method, struct blob_attr *msg)
{
	enum {
		LOG_MSG,
		__LOG_MAX,
	};

	static const struct blobmsg_policy log_policy[__LOG_MAX] = {
		[LOG_MSG] = { .name = "msg", .type = BLOBMSG_TYPE_STRING },
	};

	struct blob_attr *tb[__LOG_MAX] = {};

	blobmsg_parse(log_policy, __LOG_MAX, tb, blobmsg_data(msg), blobmsg_data_len(msg));
	if (!tb[LOG_MSG])
		return UBUS_STATUS_INVALID_ARGUMENT;

	log_send(blobmsg_get_string(tb[LOG_MSG]));

	return UBUS_STATUS_OK;
}

static int ubus_health_cb(struct ubus_context *ctx,
			  struct ubus_object *obj,
			  struct ubus_request_data *req,
			  const char *method, struct blob_attr *msg)
{
	enum {
		HEALTH_SANITY,
		HEALTH_DATA,
		__HEALTH_MAX,
	};

	static const struct blobmsg_policy health_policy[__HEALTH_MAX] = {
		[HEALTH_SANITY] = { .name = "sanity", .type = BLOBMSG_TYPE_INT32 },
		[HEALTH_DATA] = { .name = "data", .type = BLOBMSG_TYPE_TABLE },
	};

	struct blob_attr *tb[__HEALTH_MAX] = {};

	blobmsg_parse(health_policy, __HEALTH_MAX, tb, blobmsg_data(msg), blobmsg_data_len(msg));
	if (!tb[HEALTH_SANITY] || !tb[HEALTH_DATA])
		return UBUS_STATUS_INVALID_ARGUMENT;

	health_send(blobmsg_get_u32(tb[HEALTH_SANITY]), tb[HEALTH_DATA]);

	return UBUS_STATUS_OK;
}

static int ubus_simulate_cb(struct ubus_context *ctx,
			    struct ubus_object *obj,
			    struct ubus_request_data *req,
			    const char *method, struct blob_attr *msg)
{
	proto_handle_simulate(msg);

	return UBUS_STATUS_OK;
}

static int ubus_result_cb(struct ubus_context *ctx,
			  struct ubus_object *obj,
			  struct ubus_request_data *req,
			  const char *method, struct blob_attr *msg)
{
	enum {
		RESULT_STATUS,
		RESULT_ID,
		__RESULT_MAX,
	};

	static const struct blobmsg_policy result_policy[__RESULT_MAX] = {
		[RESULT_STATUS] = { .name = "status", .type = BLOBMSG_TYPE_TABLE },
		[RESULT_ID] = { .name = "id", .type = BLOBMSG_TYPE_INT32 },
	};

	struct blob_attr *tb[__RESULT_MAX] = {};

	blobmsg_parse(result_policy, __RESULT_MAX, tb, blobmsg_data(msg), blobmsg_data_len(msg));
	if (!tb[RESULT_STATUS] || !tb[RESULT_ID])
		return UBUS_STATUS_INVALID_ARGUMENT;

	result_send(blobmsg_get_u32(tb[RESULT_ID]), tb[RESULT_STATUS]);

	return UBUS_STATUS_OK;
}

static int ubus_stats_cb(struct ubus_context *ctx,
			 struct ubus_object *obj,
			 struct ubus_request_data *req,
			 const char *method, struct blob_attr *msg)
{
	stats_send(msg);

	return UBUS_STATUS_OK;
}



static const struct ubus_method ucentral_methods[] = {
	UBUS_METHOD_NOARG("status", ubus_status_cb),
	UBUS_METHOD_NOARG("health", ubus_health_cb),
	UBUS_METHOD_NOARG("stats", ubus_stats_cb),
	UBUS_METHOD_NOARG("send", ubus_send_cb),
	UBUS_METHOD_NOARG("result", ubus_result_cb),
	UBUS_METHOD_NOARG("log", ubus_log_cb),
	UBUS_METHOD_NOARG("simulate", ubus_simulate_cb),
};

static struct ubus_object_type ubus_object_type =
	UBUS_OBJECT_TYPE("ucentral", ucentral_methods);

struct ubus_object ubus_object = {
	.name = "ucentral",
	.type = &ubus_object_type,
	.methods = ucentral_methods,
	.n_methods = ARRAY_SIZE(ucentral_methods),
};

static void ubus_connect_handler(struct ubus_context *ctx)
{
	ubus_add_object(ctx, &ubus_object);
}

void ubus_init(void)
{
	conn.cb = ubus_connect_handler;
	ubus_auto_connect(&conn);
}

void ubus_deinit(void)
{
	ubus_auto_shutdown(&conn);
}
