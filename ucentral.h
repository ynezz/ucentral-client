/* SPDX-License-Identifier: BSD-3-Clause */

#include <sys/types.h>
#include <sys/stat.h>

#include <stdlib.h>
#include <unistd.h>
#include <glob.h>
#include <libgen.h>
#include <zlib.h>

#include <libwebsockets.h>

#include <libubox/ulog.h>
#include <libubox/utils.h>
#include <libubox/blobmsg.h>
#include <libubox/runqueue.h>
#include <libubox/blobmsg_json.h>

#define ULOG_DBG(fmt, ...) ulog(LOG_DEBUG, fmt, ## __VA_ARGS__)

#define USYNC_CERT	"/etc/ucentral/cert.pem"
#define USYNC_CONFIG	"/etc/ucentral/"
#define USYNC_STATE	"/tmp/ucentral.state"
#define USYNC_TMP	"/tmp/ucentral.cfg"
#define USYNC_LATEST	"/etc/ucentral/ucentral.active"


struct client_config {
	const char *server;
	int port;
	const char *path;
	const char *serial;
	const char *firmware;
	int debug;
};
extern struct client_config client;

struct ucentral_task;
struct task {
	int run_time;
	int delay;
	int periodic;
	void (*run)(time_t uuid);
	void (*complete)(struct task *t, time_t uuid, uint32_t id, int ret);
	int pending;
	struct ucentral_task *t;
};

extern struct runqueue runqueue;
extern struct lws *websocket;
extern time_t conn_time;

extern time_t uuid_latest;
extern time_t uuid_active;
extern time_t uuid_applied;

void config_init(int apply, uint32_t id);
int config_verify(struct blob_attr *attr, uint32_t id);

int cmd_run(struct blob_attr *tb, uint32_t id);

void connect_send(void);
void ping_send(void);
void raw_send(struct blob_attr *a);
void log_send(char *message);
void health_send(uint32_t sanity, struct blob_attr *a);
void result_send(uint32_t id, struct blob_attr *a);
void result_send_error(uint32_t error, char *text, uint32_t retcode, uint32_t id);
void stats_send(struct blob_attr *a);

void proto_handle(char *cmd);
void proto_handle_simulate(struct blob_attr *a);
void proto_free(void);

void configure_reply(uint32_t error, char *text, time_t uuid, uint32_t id);

void config_deinit(void);

void ubus_init(void);
void ubus_deinit(void);

void blink_run(uint32_t duration);

void health_run(uint32_t id);
void health_deinit(void);

void apply_run(uint32_t id);
extern int apply_pending;

void verify_run(uint32_t id);

void failsafe_init(void);

void task_run(struct task *task, time_t uuid, uint32_t id);
void task_stop(struct task *task);
