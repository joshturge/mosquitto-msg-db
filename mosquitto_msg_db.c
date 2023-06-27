/*
 * Copyright (c) 2023 Joshua Turgeon <jturgeon@x1.turgeon.au>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <db.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <mosquitto.h>
#include <mosquitto_broker.h>
#include <mosquitto_plugin.h>

#include "config.h"

static mosquitto_plugin_id_t *mosq_pid = NULL;
static DB *db_con = NULL;

static int
callback_message(int event, void *event_data, void *userdata)
{
	struct mosquitto_evt_message *msg;
	DBT dbk, dbd;
	time_t ts;
	size_t ksize;
	char *kbuf;
	int ret;

	msg = event_data;

	ts = time(NULL);
	ksize = snprintf(NULL, 0, "%ld", (long)ts);
	ksize += strlen(msg->topic);
	// NOTE: 1 byte for '@', another for '\0'
	ksize += 2; 

	mosquitto_log_printf(MOSQ_LOG_DEBUG, "%d: key size", ksize);
	
	kbuf = mosquitto_calloc(ksize, sizeof(char));
	if (kbuf == NULL)
		return MOSQ_ERR_NOMEM;

	ret = snprintf(kbuf, ksize, "%s@%ld", msg->topic, (long)ts);
	if (ret < 0) {
		mosquitto_log_printf(MOSQ_LOG_ERR, "%s@%ld: sprintf error: %d", msg->topic, ts, ret);
		return MOSQ_ERR_INVAL;
	}

	memset(&dbk, 0, sizeof(dbk));
	dbk.size = ksize;
	dbk.data = kbuf;
	memset(&dbd, 0, sizeof(dbd));
	dbd.size = msg->payloadlen;
	dbd.data = msg->payload;

	ret = db_con->put(db_con, &dbk, &dbd, R_NOOVERWRITE);
	if (ret == -1) {
		mosquitto_log_printf(MOSQ_LOG_ERR, "%s: dbput: %s", kbuf, strerror(errno));
		return MOSQ_ERR_ERRNO;
	}

	mosquitto_log_printf(MOSQ_LOG_DEBUG, "%s: inserted", kbuf);

	ret = db_con->sync(db_con, 0);
	if (ret == -1) {
		mosquitto_log_printf(MOSQ_LOG_ERR, "%s: dbsync: %s", kbuf, strerror(errno));
		return MOSQ_ERR_ERRNO;
	}

	return 0;
}

int
mosquitto_plugin_version(int supported_version_count, 
	const int *supported_versions)
{
		int i;

		for (i=0; i < supported_version_count; i++) {
			if (supported_versions[i] == 5) {
				mosquitto_log_printf(MOSQ_LOG_DEBUG, "5: plugin version supported");
				return 5;
			}
		}
		
		mosquitto_log_printf(MOSQ_LOG_ERR, "5: plugin version not supported");

		return -1;
}

int
mosquitto_plugin_init(mosquitto_plugin_id_t *identifier, void **userdata,
	struct mosquitto_opt *options, int option_count)
{
	int ret;
	mosq_pid = identifier;
	
	db_con = dbopen(db_pathname, O_CREAT | O_RDWR | O_SHLOCK,
	    db_permissions, db_type, NULL);
	if (db_con == NULL)
		mosquitto_log_printf(MOSQ_LOG_ERR, "%s: %s", db_pathname, strerror(errno));

	mosquitto_log_printf(MOSQ_LOG_DEBUG, "%s: opened database", db_pathname);

	ret = mosquitto_callback_register(mosq_pid, MOSQ_EVT_MESSAGE, callback_message, NULL, NULL);

	mosquitto_log_printf(MOSQ_LOG_DEBUG, "%d: registered callback", ret);

	return ret;
}

int
mosquitto_plugin_cleanup(void *userdata, struct mosquitto_opt *options,
	int option_count)
{
	int ret;

	ret = mosquitto_callback_unregister(mosq_pid, MOSQ_EVT_MESSAGE, callback_message, NULL);

	mosquitto_log_printf(MOSQ_LOG_DEBUG, "%d: unregistered callback", ret);

	ret = db_con->close(db_con);
	if (ret == -1)
		mosquitto_log_printf(MOSQ_LOG_ERR, "%s: %s", db_pathname, strerror(errno));

	return ret;
}
