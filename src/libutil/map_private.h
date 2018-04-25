/*-
 * Copyright 2016 Vsevolod Stakhov
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef SRC_LIBUTIL_MAP_PRIVATE_H_
#define SRC_LIBUTIL_MAP_PRIVATE_H_

#include "config.h"
#include "mem_pool.h"
#include "keypair.h"
#include "unix-std.h"
#include "map.h"
#include "ref.h"

typedef void (*rspamd_map_dtor) (gpointer p);
extern guint rspamd_map_log_id;
#define msg_err_map(...) rspamd_default_log_function (G_LOG_LEVEL_CRITICAL, \
		"map", map->tag, \
        G_STRFUNC, \
        __VA_ARGS__)
#define msg_warn_map(...)   rspamd_default_log_function (G_LOG_LEVEL_WARNING, \
		"map", map->tag, \
        G_STRFUNC, \
        __VA_ARGS__)
#define msg_info_map(...)   rspamd_default_log_function (G_LOG_LEVEL_INFO, \
		"map", map->tag, \
        G_STRFUNC, \
        __VA_ARGS__)
#define msg_debug_map(...)  rspamd_conditional_debug_fast (NULL, NULL, \
        rspamd_map_log_id, "map", map->tag, \
        G_STRFUNC, \
        __VA_ARGS__)

enum fetch_proto {
	MAP_PROTO_FILE,
	MAP_PROTO_HTTP,
	MAP_PROTO_HTTPS,
	MAP_PROTO_STATIC
};

/**
 * Data specific to file maps
 */
struct file_map_data {
	gchar *filename;
	struct stat st;
};

/**
 * Data specific to HTTP maps
 */
struct http_map_data {
	gchar *path;
	gchar *host;
	gchar *last_signature;
	rspamd_fstring_t *etag;
	time_t last_modified;
	time_t last_checked;
	gboolean request_sent;
	guint64 gen;
	guint16 port;
};

struct static_map_data {
	guchar *data;
	gsize len;
	gboolean processed;
};

union rspamd_map_backend_data {
	struct file_map_data *fd;
	struct http_map_data *hd;
	struct static_map_data *sd;
};

struct rspamd_map_backend {
	enum fetch_proto protocol;
	gboolean is_signed;
	gboolean is_compressed;
	guint32 id;
	struct rspamd_cryptobox_pubkey *trusted_pubkey;
	union rspamd_map_backend_data data;
	gchar *uri;
	ref_entry_t ref;
};

struct rspamd_map_cachepoint {
	gint available;
	gsize len;
	time_t last_modified;
	gchar shmem_name[256];
};

struct rspamd_map {
	struct rspamd_dns_resolver *r;
	struct rspamd_config *cfg;
	GPtrArray *backends;
	map_cb_t read_callback;
	map_fin_cb_t fin_callback;
	void **user_data;
	struct event_base *ev_base;
	gchar *description;
	gchar *name;
	guint32 id;
	gboolean scheduled_check;
	rspamd_map_dtor dtor;
	gpointer dtor_data;
	rspamd_map_traverse_function traverse_function;
	/* Should we check HTTP or just load cached data */
	struct timeval tv;
	gdouble poll_timeout;
	time_t next_check;
	gboolean active_http;
	/* Shared lock for temporary disabling of map reading (e.g. when this map is written by UI) */
	gint *locked;
	/* Shared cache data */
	struct rspamd_map_cachepoint *cache;
	gchar tag[MEMPOOL_UID_LEN];
};

enum rspamd_map_http_stage {
	map_resolve_host2 = 0, /* 2 requests sent */
	map_resolve_host1, /* 1 requests sent */
	map_load_file,
	map_load_pubkey,
	map_load_signature,
	map_finished
};

struct map_periodic_cbdata {
	struct rspamd_map *map;
	struct map_cb_data cbdata;
	struct event ev;
	gboolean need_modify;
	gboolean errored;
	gboolean locked;
	guint cur_backend;
	ref_entry_t ref;
};

struct http_callback_data {
	struct event_base *ev_base;
	struct rspamd_http_connection *conn;
	rspamd_inet_addr_t *addr;
	struct rspamd_map *map;
	struct rspamd_map_backend *bk;
	struct http_map_data *data;
	struct map_periodic_cbdata *periodic;
	struct rspamd_cryptobox_pubkey *pk;
	gboolean check;
	struct rspamd_storage_shmem *shmem_data;
	struct rspamd_storage_shmem *shmem_sig;
	struct rspamd_storage_shmem *shmem_pubkey;
	gsize data_len;
	gsize sig_len;
	gsize pubkey_len;

	enum rspamd_map_http_stage stage;
	gint fd;
	struct timeval tv;

	ref_entry_t ref;
};

#endif /* SRC_LIBUTIL_MAP_PRIVATE_H_ */
