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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <pthread.h>
#include <mosquitto.h>

#include "mqtt_core.h"

#define QUEUE_DEFAULT_MAX 100000

/* ---- configuration ---------------------------------------------------- */

struct pl_mqtt_conf {
  char *client_id;        /* NULL: random id chosen by the library */
  char *host;
  int port;
  int keepalive;
  int clean_session;
  int protocol_version;   /* 3 (3.1), 4 (3.1.1) or 5 */
  char *username;
  char *password;
  char *cafile;
  char *capath;
  char *certfile;
  char *keyfile;
  int tls_insecure;
  char *will_topic;
  char *will_payload;
  int will_qos;
  int will_retain;
  int queue_max;
};

static char *dup_or_null(const char *s)
{
  return (s && *s) ? strdup(s) : NULL;
}

static void set_str(char **dst, const char *val)
{
  free(*dst);
  *dst = dup_or_null(val);
}

pl_mqtt_conf *pl_mqtt_conf_new(void)
{
  pl_mqtt_conf *c = calloc(1, sizeof(*c));
  if (!c) { return NULL; }
  c->host = strdup("localhost");
  c->port = 1883;
  c->keepalive = 60;
  c->clean_session = 1;
  c->protocol_version = 4;
  c->queue_max = QUEUE_DEFAULT_MAX;
  return c;
}

void pl_mqtt_conf_destroy(pl_mqtt_conf *c)
{
  if (!c) { return; }
  free(c->client_id); free(c->host); free(c->username); free(c->password);
  free(c->cafile); free(c->capath); free(c->certfile); free(c->keyfile);
  free(c->will_topic); free(c->will_payload);
  free(c);
}

static pl_mqtt_conf *conf_copy(const pl_mqtt_conf *s)
{
  pl_mqtt_conf *c = pl_mqtt_conf_new();
  if (!c) { return NULL; }
  c->port = s->port; c->keepalive = s->keepalive;
  c->clean_session = s->clean_session; c->protocol_version = s->protocol_version;
  c->tls_insecure = s->tls_insecure; c->will_qos = s->will_qos;
  c->will_retain = s->will_retain; c->queue_max = s->queue_max;
  set_str(&c->client_id, s->client_id); set_str(&c->host, s->host);
  set_str(&c->username, s->username); set_str(&c->password, s->password);
  set_str(&c->cafile, s->cafile); set_str(&c->capath, s->capath);
  set_str(&c->certfile, s->certfile); set_str(&c->keyfile, s->keyfile);
  set_str(&c->will_topic, s->will_topic); set_str(&c->will_payload, s->will_payload);
  return c;
}

static void seterr(char *errstr, size_t n, const char *msg)
{
  if (errstr && n > 0) { snprintf(errstr, n, "%s", msg); }
}

static int parse_int(const char *val, int lo, int hi, int *out)
{
  char *end;
  long v = strtol(val, &end, 10);
  if (end == val || *end != '\0' || v < lo || v > hi) { return -1; }
  *out = (int)v;
  return 0;
}

static int parse_bool(const char *val, int *out)
{
  if (!strcasecmp(val, "true") || !strcmp(val, "1") || !strcasecmp(val, "yes")) { *out = 1; return 0; }
  if (!strcasecmp(val, "false") || !strcmp(val, "0") || !strcasecmp(val, "no")) { *out = 0; return 0; }
  return -1;
}

int pl_mqtt_conf_set(pl_mqtt_conf *c, const char *key, const char *val,
                     char *errstr, size_t errstr_size)
{
  if (!c || !key || !val) { seterr(errstr, errstr_size, "null argument"); return -1; }

  if (!strcmp(key, "client.id")) { set_str(&c->client_id, val); return 0; }
  if (!strcmp(key, "host")) {
    if (!*val) { seterr(errstr, errstr_size, "host must not be empty"); return -1; }
    set_str(&c->host, val); return 0;
  }
  if (!strcmp(key, "username")) { set_str(&c->username, val); return 0; }
  if (!strcmp(key, "password")) { set_str(&c->password, val); return 0; }
  if (!strcmp(key, "tls.cafile")) { set_str(&c->cafile, val); return 0; }
  if (!strcmp(key, "tls.capath")) { set_str(&c->capath, val); return 0; }
  if (!strcmp(key, "tls.certfile")) { set_str(&c->certfile, val); return 0; }
  if (!strcmp(key, "tls.keyfile")) { set_str(&c->keyfile, val); return 0; }
  if (!strcmp(key, "will.topic")) { set_str(&c->will_topic, val); return 0; }
  if (!strcmp(key, "will.payload")) { set_str(&c->will_payload, val); return 0; }

  int rc = -1;
  if (!strcmp(key, "port")) { rc = parse_int(val, 1, 65535, &c->port); }
  else if (!strcmp(key, "keepalive")) { rc = parse_int(val, 5, 65535, &c->keepalive); }
  else if (!strcmp(key, "queue.max")) { rc = parse_int(val, 1, 100000000, &c->queue_max); }
  else if (!strcmp(key, "will.qos")) { rc = parse_int(val, 0, 2, &c->will_qos); }
  else if (!strcmp(key, "clean.session")) { rc = parse_bool(val, &c->clean_session); }
  else if (!strcmp(key, "tls.insecure")) { rc = parse_bool(val, &c->tls_insecure); }
  else if (!strcmp(key, "will.retain")) { rc = parse_bool(val, &c->will_retain); }
  else if (!strcmp(key, "protocol.version")) {
    if (!strcmp(val, "3.1")) { c->protocol_version = 3; rc = 0; }
    else if (!strcmp(val, "3.1.1")) { c->protocol_version = 4; rc = 0; }
    else if (!strcmp(val, "5")) { c->protocol_version = 5; rc = 0; }
  }
  else {
    seterr(errstr, errstr_size, "unknown configuration key");
    return -1;
  }
  if (rc != 0) { seterr(errstr, errstr_size, "invalid value"); }
  return rc;
}

int pl_mqtt_conf_entry(const pl_mqtt_conf *c, int idx, const char **key, char *val, size_t n)
{
  if (!c || idx < 0) { return -1; }
  const char *str = NULL;
  int num = 0;
  enum { T_STR, T_INT, T_BOOL } type = T_STR;

  switch (idx) {
    case 0:  *key = "client.id";        str = c->client_id; break;
    case 1:  *key = "host";             str = c->host; break;
    case 2:  *key = "port";             type = T_INT;  num = c->port; break;
    case 3:  *key = "keepalive";        type = T_INT;  num = c->keepalive; break;
    case 4:  *key = "clean.session";    type = T_BOOL; num = c->clean_session; break;
    case 5:  *key = "protocol.version";
             str = (c->protocol_version == 3) ? "3.1" : (c->protocol_version == 5) ? "5" : "3.1.1"; break;
    case 6:  *key = "username";         str = c->username; break;
    case 7:  *key = "password";         str = c->password ? "********" : NULL; break;
    case 8:  *key = "tls.cafile";       str = c->cafile; break;
    case 9:  *key = "tls.capath";       str = c->capath; break;
    case 10: *key = "tls.certfile";     str = c->certfile; break;
    case 11: *key = "tls.keyfile";      str = c->keyfile; break;
    case 12: *key = "tls.insecure";     type = T_BOOL; num = c->tls_insecure; break;
    case 13: *key = "will.topic";       str = c->will_topic; break;
    case 14: *key = "will.payload";     str = c->will_payload; break;
    case 15: *key = "will.qos";         type = T_INT;  num = c->will_qos; break;
    case 16: *key = "will.retain";      type = T_BOOL; num = c->will_retain; break;
    case 17: *key = "queue.max";        type = T_INT;  num = c->queue_max; break;
    default: return -1;
  }
  switch (type) {
    case T_STR:  snprintf(val, n, "%s", str ? str : ""); break;
    case T_INT:  snprintf(val, n, "%d", num); break;
    case T_BOOL: snprintf(val, n, "%s", num ? "true" : "false"); break;
  }
  return 0;
}

/* ---- client ----------------------------------------------------------- */

typedef struct qnode {
  pl_mqtt_message msg;
  struct qnode *next;
} qnode;

typedef struct sub {
  char *pattern;
  int qos;
  struct sub *next;
} sub;

struct pl_mqtt_client {
  struct mosquitto *mosq;
  pl_mqtt_conf *conf;

  pthread_mutex_t lock;
  pthread_cond_t cond;      /* signalled on any state change below */

  int connected;
  int connack_rc;
  int started;              /* network thread running */
  int connect_called;

  qnode *head, *tail;
  int queued;
  unsigned long dropped;

  unsigned long published;  /* publish calls accepted by libmosquitto */
  unsigned long confirmed;  /* on_publish callbacks seen */

  sub *subs;
};

static pthread_once_t lib_once = PTHREAD_ONCE_INIT;
static void lib_init(void) { mosquitto_lib_init(); }

const char *pl_mqtt_version_str(void)
{
  static char buf[32];
  int major, minor, rev;
  mosquitto_lib_version(&major, &minor, &rev);
  snprintf(buf, sizeof(buf), "%d.%d.%d", major, minor, rev);
  return buf;
}

const char *pl_mqtt_strerror(int rc)
{
  switch (rc) {
    case PL_MQTT_ETIMEOUT: return "timed out";
    case PL_MQTT_EREFUSED: return "connection refused by broker";
    case PL_MQTT_EARG: return "invalid argument";
    default: return mosquitto_strerror(rc);
  }
}

static void message_free_fields(pl_mqtt_message *m)
{
  free(m->topic);
  free(m->payload);
}

void pl_mqtt_message_free(pl_mqtt_message *m)
{
  if (!m) { return; }
  message_free_fields(m);
  free(m);
}

static void cb_connect(struct mosquitto *mosq, void *obj, int rc)
{
  pl_mqtt_client *c = obj;
  pthread_mutex_lock(&c->lock);
  c->connack_rc = rc;
  c->connected = (rc == 0);
  if (rc == 0) {
    /* (re-)establish remembered subscriptions, e.g. after a reconnect */
    for (sub *s = c->subs; s; s = s->next) {
      mosquitto_subscribe(mosq, NULL, s->pattern, s->qos);
    }
  }
  pthread_cond_broadcast(&c->cond);
  pthread_mutex_unlock(&c->lock);
}

static void cb_disconnect(struct mosquitto *mosq, void *obj, int rc)
{
  (void)mosq; (void)rc;
  pl_mqtt_client *c = obj;
  pthread_mutex_lock(&c->lock);
  c->connected = 0;
  pthread_cond_broadcast(&c->cond);
  pthread_mutex_unlock(&c->lock);
}

static void cb_publish(struct mosquitto *mosq, void *obj, int mid)
{
  (void)mosq; (void)mid;
  pl_mqtt_client *c = obj;
  pthread_mutex_lock(&c->lock);
  c->confirmed++;
  pthread_cond_broadcast(&c->cond);
  pthread_mutex_unlock(&c->lock);
}

static void cb_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *m)
{
  (void)mosq;
  pl_mqtt_client *c = obj;

  qnode *n = calloc(1, sizeof(*n));
  if (!n) { return; }
  n->msg.topic = strdup(m->topic ? m->topic : "");
  n->msg.payload = malloc((size_t)m->payloadlen + 1);
  if (!n->msg.topic || !n->msg.payload) { message_free_fields(&n->msg); free(n); return; }
  if (m->payloadlen > 0) { memcpy(n->msg.payload, m->payload, (size_t)m->payloadlen); }
  ((char *)n->msg.payload)[m->payloadlen] = '\0';
  n->msg.payload_len = (size_t)m->payloadlen;
  n->msg.qos = m->qos;
  n->msg.retain = m->retain ? 1 : 0;
  n->msg.mid = m->mid;

  pthread_mutex_lock(&c->lock);
  if (c->queued >= c->conf->queue_max) {
    c->dropped++;
    pthread_mutex_unlock(&c->lock);
    message_free_fields(&n->msg);
    free(n);
    return;
  }
  if (c->tail) { c->tail->next = n; } else { c->head = n; }
  c->tail = n;
  c->queued++;
  pthread_cond_broadcast(&c->cond);
  pthread_mutex_unlock(&c->lock);
}

/* absolute CLOCK_REALTIME deadline for pthread_cond_timedwait */
static void deadline_in(struct timespec *ts, int timeout_ms)
{
  clock_gettime(CLOCK_REALTIME, ts);
  ts->tv_sec += timeout_ms / 1000;
  ts->tv_nsec += (long)(timeout_ms % 1000) * 1000000L;
  if (ts->tv_nsec >= 1000000000L) { ts->tv_sec++; ts->tv_nsec -= 1000000000L; }
}

pl_mqtt_client *pl_mqtt_client_new(const pl_mqtt_conf *conf, char *errstr, size_t errstr_size)
{
  pthread_once(&lib_once, lib_init);

  pl_mqtt_client *c = calloc(1, sizeof(*c));
  if (!c) { seterr(errstr, errstr_size, "out of memory"); return NULL; }
  c->conf = conf_copy(conf);
  if (!c->conf) { free(c); seterr(errstr, errstr_size, "out of memory"); return NULL; }
  pthread_mutex_init(&c->lock, NULL);
  pthread_cond_init(&c->cond, NULL);

  const pl_mqtt_conf *cf = c->conf;
  /* without a client id the broker requires a clean session */
  if (!cf->client_id && !cf->clean_session) {
    seterr(errstr, errstr_size, "clean.session=false requires a client.id");
    goto fail;
  }

  c->mosq = mosquitto_new(cf->client_id, cf->clean_session ? true : false, c);
  if (!c->mosq) { seterr(errstr, errstr_size, "mosquitto_new failed"); goto fail; }

  int rc;
  if (cf->protocol_version != 4) {
    rc = mosquitto_int_option(c->mosq, MOSQ_OPT_PROTOCOL_VERSION,
                              cf->protocol_version == 3 ? MQTT_PROTOCOL_V31 : MQTT_PROTOCOL_V5);
    if (rc != MOSQ_ERR_SUCCESS) { seterr(errstr, errstr_size, mosquitto_strerror(rc)); goto fail; }
  }
  if (cf->username) {
    rc = mosquitto_username_pw_set(c->mosq, cf->username, cf->password);
    if (rc != MOSQ_ERR_SUCCESS) { seterr(errstr, errstr_size, mosquitto_strerror(rc)); goto fail; }
  }
  if (cf->cafile || cf->capath) {
    rc = mosquitto_tls_set(c->mosq, cf->cafile, cf->capath, cf->certfile, cf->keyfile, NULL);
    if (rc != MOSQ_ERR_SUCCESS) { seterr(errstr, errstr_size, mosquitto_strerror(rc)); goto fail; }
    if (cf->tls_insecure) { mosquitto_tls_insecure_set(c->mosq, true); }
  }
  if (cf->will_topic) {
    size_t wl = cf->will_payload ? strlen(cf->will_payload) : 0;
    rc = mosquitto_will_set(c->mosq, cf->will_topic, (int)wl, cf->will_payload,
                            cf->will_qos, cf->will_retain ? true : false);
    if (rc != MOSQ_ERR_SUCCESS) { seterr(errstr, errstr_size, mosquitto_strerror(rc)); goto fail; }
  }

  mosquitto_connect_callback_set(c->mosq, cb_connect);
  mosquitto_disconnect_callback_set(c->mosq, cb_disconnect);
  mosquitto_publish_callback_set(c->mosq, cb_publish);
  mosquitto_message_callback_set(c->mosq, cb_message);
  return c;

fail:
  pl_mqtt_client_destroy(c);
  return NULL;
}

int pl_mqtt_connect(pl_mqtt_client *c, int timeout_ms)
{
  if (!c) { return PL_MQTT_EARG; }
  if (timeout_ms < 0) { timeout_ms = 0; }

  int rc;
  pthread_mutex_lock(&c->lock);
  int first = !c->connect_called;
  c->connect_called = 1;
  c->connack_rc = -1;
  pthread_mutex_unlock(&c->lock);

  /* do not hold the lock: callbacks may run as soon as the thread starts */
  if (first) {
    rc = mosquitto_connect(c->mosq, c->conf->host, c->conf->port, c->conf->keepalive);
  } else {
    rc = mosquitto_reconnect(c->mosq);
  }
  if (rc != MOSQ_ERR_SUCCESS) { return rc; }

  pthread_mutex_lock(&c->lock);
  int need_start = !c->started;
  c->started = 1;
  pthread_mutex_unlock(&c->lock);
  if (need_start) {
    rc = mosquitto_loop_start(c->mosq);
    if (rc != MOSQ_ERR_SUCCESS) {
      pthread_mutex_lock(&c->lock);
      c->started = 0;
      pthread_mutex_unlock(&c->lock);
      return rc;
    }
  }

  struct timespec ts;
  deadline_in(&ts, timeout_ms);
  pthread_mutex_lock(&c->lock);
  while (c->connack_rc < 0) {
    if (pthread_cond_timedwait(&c->cond, &c->lock, &ts) != 0) { break; }
  }
  rc = (c->connack_rc == 0) ? 0 : (c->connack_rc < 0 ? PL_MQTT_ETIMEOUT : PL_MQTT_EREFUSED);
  pthread_mutex_unlock(&c->lock);
  return rc;
}

int pl_mqtt_is_connected(pl_mqtt_client *c)
{
  if (!c) { return 0; }
  pthread_mutex_lock(&c->lock);
  int r = c->connected;
  pthread_mutex_unlock(&c->lock);
  return r;
}

int pl_mqtt_disconnect(pl_mqtt_client *c)
{
  if (!c) { return PL_MQTT_EARG; }
  int rc = mosquitto_disconnect(c->mosq);
  return rc;
}

void pl_mqtt_client_destroy(pl_mqtt_client *c)
{
  if (!c) { return; }
  if (c->mosq) {
    if (c->started) {
      mosquitto_disconnect(c->mosq);
      mosquitto_loop_stop(c->mosq, false);   /* joins the network thread */
    }
    mosquitto_destroy(c->mosq);
  }
  while (c->head) {
    qnode *n = c->head;
    c->head = n->next;
    message_free_fields(&n->msg);
    free(n);
  }
  while (c->subs) {
    sub *s = c->subs;
    c->subs = s->next;
    free(s->pattern);
    free(s);
  }
  pl_mqtt_conf_destroy(c->conf);
  pthread_cond_destroy(&c->cond);
  pthread_mutex_destroy(&c->lock);
  free(c);
}

int pl_mqtt_publish(pl_mqtt_client *c, const char *topic,
                    const void *payload, size_t payload_len,
                    int qos, int retain)
{
  if (!c || !topic || qos < 0 || qos > 2) { return PL_MQTT_EARG; }
  int mid = 0;
  int rc = mosquitto_publish(c->mosq, &mid, topic, (int)payload_len, payload, qos, retain ? true : false);
  if (rc == MOSQ_ERR_SUCCESS) {
    pthread_mutex_lock(&c->lock);
    c->published++;
    pthread_mutex_unlock(&c->lock);
  }
  return rc;
}

int pl_mqtt_flush(pl_mqtt_client *c, int timeout_ms)
{
  if (!c) { return PL_MQTT_EARG; }
  struct timespec ts;
  deadline_in(&ts, timeout_ms < 0 ? 0 : timeout_ms);
  int rc = 0;
  pthread_mutex_lock(&c->lock);
  while (c->confirmed < c->published) {
    if (pthread_cond_timedwait(&c->cond, &c->lock, &ts) != 0) { rc = PL_MQTT_ETIMEOUT; break; }
  }
  pthread_mutex_unlock(&c->lock);
  return rc;
}

int pl_mqtt_subscribe(pl_mqtt_client *c, const char *pattern, int qos)
{
  if (!c || !pattern || qos < 0 || qos > 2) { return PL_MQTT_EARG; }
  int rc = mosquitto_subscribe(c->mosq, NULL, pattern, qos);
  if (rc != MOSQ_ERR_SUCCESS) { return rc; }

  pthread_mutex_lock(&c->lock);
  sub *s;
  for (s = c->subs; s; s = s->next) {
    if (!strcmp(s->pattern, pattern)) { s->qos = qos; break; }
  }
  if (!s && (s = calloc(1, sizeof(*s))) != NULL) {
    s->pattern = strdup(pattern);
    s->qos = qos;
    s->next = c->subs;
    c->subs = s;
  }
  pthread_mutex_unlock(&c->lock);
  return MOSQ_ERR_SUCCESS;
}

int pl_mqtt_unsubscribe(pl_mqtt_client *c, const char *pattern)
{
  if (!c || !pattern) { return PL_MQTT_EARG; }
  int rc = mosquitto_unsubscribe(c->mosq, NULL, pattern);
  if (rc != MOSQ_ERR_SUCCESS) { return rc; }

  pthread_mutex_lock(&c->lock);
  for (sub **p = &c->subs; *p; p = &(*p)->next) {
    if (!strcmp((*p)->pattern, pattern)) {
      sub *s = *p;
      *p = s->next;
      free(s->pattern);
      free(s);
      break;
    }
  }
  pthread_mutex_unlock(&c->lock);
  return MOSQ_ERR_SUCCESS;
}

pl_mqtt_message *pl_mqtt_poll(pl_mqtt_client *c, int timeout_ms)
{
  if (!c) { return NULL; }
  struct timespec ts;
  deadline_in(&ts, timeout_ms < 0 ? 0 : timeout_ms);

  pthread_mutex_lock(&c->lock);
  while (!c->head) {
    if (pthread_cond_timedwait(&c->cond, &c->lock, &ts) != 0) { break; }
  }
  qnode *n = c->head;
  if (n) {
    c->head = n->next;
    if (!c->head) { c->tail = NULL; }
    c->queued--;
  }
  pthread_mutex_unlock(&c->lock);
  if (!n) { return NULL; }

  pl_mqtt_message *m = malloc(sizeof(*m));
  if (m) { *m = n->msg; } else { message_free_fields(&n->msg); }
  free(n);
  return m;
}

unsigned long pl_mqtt_dropped(pl_mqtt_client *c)
{
  if (!c) { return 0; }
  pthread_mutex_lock(&c->lock);
  unsigned long d = c->dropped;
  pthread_mutex_unlock(&c->lock);
  return d;
}

int pl_mqtt_is_utf8(const void *data, size_t len)
{
  const unsigned char *s = data;
  size_t i = 0;
  while (i < len) {
    unsigned char b = s[i];
    size_t extra;
    unsigned int cp;
    if (b < 0x80) { i++; continue; }
    else if ((b & 0xE0) == 0xC0) { extra = 1; cp = b & 0x1F; }
    else if ((b & 0xF0) == 0xE0) { extra = 2; cp = b & 0x0F; }
    else if ((b & 0xF8) == 0xF0) { extra = 3; cp = b & 0x07; }
    else { return 0; }
    if (i + extra >= len) { return 0; }
    for (size_t k = 1; k <= extra; k++) {
      if ((s[i + k] & 0xC0) != 0x80) { return 0; }
      cp = (cp << 6) | (s[i + k] & 0x3F);
    }
    if ((extra == 1 && cp < 0x80) || (extra == 2 && cp < 0x800) || (extra == 3 && cp < 0x10000)) { return 0; }
    if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) { return 0; }
    i += extra + 1;
  }
  return 1;
}
