/*   Prolog Interface to MQTT (libmosquitto)
 *   Copyright (C) 2026  Alexander Diemand
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PL_MQTT_CORE_H
#define PL_MQTT_CORE_H

/* the actual libmosquitto work, shared between the GNU Prolog and
 * SWI-Prolog bridges. this file has no dependency on either Prolog C
 * interface.
 *
 * a client runs its own network thread around mosquitto_loop_forever,
 * restarted with a backoff whenever libmosquitto gives up on a connection,
 * so it keeps reconnecting until pl_mqtt_disconnect or
 * pl_mqtt_client_destroy. incoming messages are copied onto a queue that
 * the Prolog side drains with pl_mqtt_poll. */

#include <stdint.h>
#include <stddef.h>

#define PL_MQTT_ETIMEOUT (-100)   /* no CONNACK / nothing happened in time */
#define PL_MQTT_EREFUSED (-101)   /* broker answered CONNACK with an error */
#define PL_MQTT_EARG     (-102)   /* bad argument */

typedef struct pl_mqtt_conf pl_mqtt_conf;
typedef struct pl_mqtt_client pl_mqtt_client;

/* configuration (the analog of rd_kafka_conf_t) */
pl_mqtt_conf *pl_mqtt_conf_new(void);
void pl_mqtt_conf_destroy(pl_mqtt_conf *conf);

/* returns 0 on success, -1 on unknown key / invalid value (errstr is filled) */
int pl_mqtt_conf_set(pl_mqtt_conf *conf, const char *key, const char *val,
                     char *errstr, size_t errstr_size);

/* enumerates the settings: idx = 0,1,2,.. ; returns 0 and fills key/val,
 * or -1 once idx is past the last entry */
int pl_mqtt_conf_entry(const pl_mqtt_conf *conf, int idx,
                       const char **key, char *val, size_t val_size);

/* clients */
const char *pl_mqtt_version_str(void);
const char *pl_mqtt_strerror(int rc);

/* the configuration is copied; the caller still owns conf */
pl_mqtt_client *pl_mqtt_client_new(const pl_mqtt_conf *conf, char *errstr, size_t errstr_size);

/* connects and starts the network thread; waits up to timeout_ms for the CONNACK.
 * returns 0 on success, a MOSQ_ERR_* code or one of PL_MQTT_E*. */
int pl_mqtt_connect(pl_mqtt_client *c, int timeout_ms);
int pl_mqtt_is_connected(pl_mqtt_client *c);
int pl_mqtt_disconnect(pl_mqtt_client *c);
void pl_mqtt_client_destroy(pl_mqtt_client *c);

/* returns 0 on success, otherwise a MOSQ_ERR_* code */
int pl_mqtt_publish(pl_mqtt_client *c, const char *topic,
                    const void *payload, size_t payload_len,
                    int qos, int retain);

/* waits until all published messages were handed over to / acknowledged by the broker.
 * returns 0 on success, PL_MQTT_ETIMEOUT otherwise */
int pl_mqtt_flush(pl_mqtt_client *c, int timeout_ms);

/* subscriptions are remembered and renewed after an automatic reconnect */
int pl_mqtt_subscribe(pl_mqtt_client *c, const char *pattern, int qos);
int pl_mqtt_unsubscribe(pl_mqtt_client *c, const char *pattern);

/* a received message; free with pl_mqtt_message_free */
typedef struct {
  char *topic;
  void *payload;      /* always NUL terminated (not counted in payload_len) */
  size_t payload_len;
  int qos;
  int retain;
  int mid;
} pl_mqtt_message;

/* waits up to timeout_ms for a message. returns NULL on timeout */
pl_mqtt_message *pl_mqtt_poll(pl_mqtt_client *c, int timeout_ms);
void pl_mqtt_message_free(pl_mqtt_message *msg);

/* number of messages dropped because the receive queue was full */
unsigned long pl_mqtt_dropped(pl_mqtt_client *c);

/* nonzero if the bytes are well-formed UTF-8 */
int pl_mqtt_is_utf8(const void *data, size_t len);

#endif /* PL_MQTT_CORE_H */
