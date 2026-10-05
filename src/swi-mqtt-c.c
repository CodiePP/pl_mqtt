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
#include <string.h>

#include <SWI-Prolog.h>

#include "mqtt_core.h"

#define TEXT_FLAGS (CVT_ATOM|CVT_STRING|CVT_LIST|CVT_INTEGER|CVT_FLOAT|REP_UTF8)

/* declarations */
foreign_t swi_mqtt_version(term_t v);
foreign_t swi_mqtt_conf_new(term_t cid);
foreign_t swi_mqtt_conf_destroy(term_t cid);
foreign_t swi_mqtt_conf_set(term_t cid, term_t k, term_t v);
foreign_t swi_mqtt_conf_dump(term_t cid, term_t list);
foreign_t swi_mqtt_client_new(term_t cid, term_t client);
foreign_t swi_mqtt_connect(term_t client, term_t timeout);
foreign_t swi_mqtt_is_connected(term_t client);
foreign_t swi_mqtt_disconnect(term_t client);
foreign_t swi_mqtt_destroy(term_t client);
foreign_t swi_mqtt_publish(term_t client, term_t topic, term_t payload, term_t qos, term_t retain);
foreign_t swi_mqtt_flush(term_t client, term_t timeout);
foreign_t swi_mqtt_subscribe(term_t client, term_t pattern, term_t qos);
foreign_t swi_mqtt_unsubscribe(term_t client, term_t pattern);
foreign_t swi_mqtt_poll(term_t client, term_t timeout, term_t message, term_t meta);
foreign_t swi_mqtt_dropped(term_t client, term_t count);

/* install predicates */
install_t install()
{
  PL_register_foreign("pl_mqtt_version", 1, swi_mqtt_version, 0);
  PL_register_foreign("pl_mqtt_conf_new", 1, swi_mqtt_conf_new, 0);
  PL_register_foreign("pl_mqtt_conf_destroy", 1, swi_mqtt_conf_destroy, 0);
  PL_register_foreign("pl_mqtt_conf_set", 3, swi_mqtt_conf_set, 0);
  PL_register_foreign("pl_mqtt_conf_dump", 2, swi_mqtt_conf_dump, 0);
  PL_register_foreign("pl_mqtt_client_new", 2, swi_mqtt_client_new, 0);
  PL_register_foreign("pl_mqtt_connect", 2, swi_mqtt_connect, 0);
  PL_register_foreign("pl_mqtt_is_connected", 1, swi_mqtt_is_connected, 0);
  PL_register_foreign("pl_mqtt_disconnect", 1, swi_mqtt_disconnect, 0);
  PL_register_foreign("pl_mqtt_destroy", 1, swi_mqtt_destroy, 0);
  PL_register_foreign("pl_mqtt_publish", 5, swi_mqtt_publish, 0);
  PL_register_foreign("pl_mqtt_flush", 2, swi_mqtt_flush, 0);
  PL_register_foreign("pl_mqtt_subscribe", 3, swi_mqtt_subscribe, 0);
  PL_register_foreign("pl_mqtt_unsubscribe", 2, swi_mqtt_unsubscribe, 0);
  PL_register_foreign("pl_mqtt_poll", 4, swi_mqtt_poll, 0);
  PL_register_foreign("pl_mqtt_dropped", 2, swi_mqtt_dropped, 0);
}

/* helpers */

static int get_ptr(term_t t, void **p)
{
  if (PL_is_variable(t)) { return 0; }
  return PL_get_pointer(t, p);
}

/* 0 for false, 1 for true; accepts the atoms true/false and the integers 0/1 */
static int get_bool(term_t t, int *out)
{
  int i;
  if (PL_get_bool(t, &i)) { *out = i ? 1 : 0; return 1; }
  if (PL_get_integer(t, &i) && (i == 0 || i == 1)) { *out = i; return 1; }
  return 0;
}

/* definitions */

foreign_t swi_mqtt_version(term_t out_v)
{
  return PL_unify_atom_chars(out_v, pl_mqtt_version_str());
}

foreign_t swi_mqtt_conf_new(term_t out_cid)
{
  if (!PL_is_variable(out_cid)) { PL_fail; }
  pl_mqtt_conf *kc = pl_mqtt_conf_new();
  if (!kc) { PL_fail; }
  return PL_unify_pointer(out_cid, kc);
}

foreign_t swi_mqtt_conf_destroy(term_t in_cid)
{
  pl_mqtt_conf *kc;
  if (!get_ptr(in_cid, (void**)&kc)) { PL_fail; }
  pl_mqtt_conf_destroy(kc);
  PL_succeed;
}

foreign_t swi_mqtt_conf_set(term_t in_cid, term_t in_k, term_t in_v)
{
  pl_mqtt_conf *kc;
  if (!get_ptr(in_cid, (void**)&kc)) { PL_fail; }

  char *k_key, *k_val;
  if (!PL_get_chars(in_k, &k_key, CVT_ATOM|CVT_STRING|REP_UTF8)) { PL_fail; }
  if (!PL_get_chars(in_v, &k_val, CVT_ATOM|CVT_STRING|CVT_INTEGER|CVT_LIST|REP_UTF8)) { PL_fail; }

  char errstr[128];
  if (pl_mqtt_conf_set(kc, k_key, k_val, errstr, sizeof(errstr)) != 0)
  {
    fprintf(stderr, "ERROR - mqtt conf set %s: %s\n", k_key, errstr);
    PL_fail;
  }
  PL_succeed;
}

foreign_t swi_mqtt_conf_dump(term_t in_cid, term_t out_list)
{
  pl_mqtt_conf *kc;
  if (!get_ptr(in_cid, (void**)&kc)) { PL_fail; }
  if (!PL_is_variable(out_list)) { PL_fail; }

  term_t ele = PL_new_term_ref();
  term_t lst = PL_copy_term_ref(out_list);

  const char *key;
  char val[256];
  for (int i = 0; pl_mqtt_conf_entry(kc, i, &key, val, sizeof(val)) == 0; i++)
  {
    functor_t fct = PL_new_functor(PL_new_atom(key), 1);
    if (!PL_unify_list(lst, ele, lst)) { PL_fail; }
    if (!PL_unify_term(ele, PL_FUNCTOR, fct,
                            PL_UTF8_STRING, val)) { PL_fail; }
  }
  return PL_unify_nil(lst);
}

foreign_t swi_mqtt_client_new(term_t in_cid, term_t out_client)
{
  pl_mqtt_conf *kc;
  if (!get_ptr(in_cid, (void**)&kc)) { PL_fail; }
  if (!PL_is_variable(out_client)) { PL_fail; }

  char errstr[128];
  pl_mqtt_client *c = pl_mqtt_client_new(kc, errstr, sizeof(errstr));
  if (!c)
  {
    fprintf(stderr, "ERROR - mqtt client new: %s\n", errstr);
    PL_fail;
  }
  return PL_unify_pointer(out_client, c);
}

foreign_t swi_mqtt_connect(term_t in_client, term_t in_timeout)
{
  pl_mqtt_client *c;
  if (!get_ptr(in_client, (void**)&c)) { PL_fail; }
  int timeout;
  if (!PL_get_integer(in_timeout, &timeout)) { PL_fail; }

  int rc = pl_mqtt_connect(c, timeout);
  if (rc != 0)
  {
    fprintf(stderr, "ERROR - mqtt connect: %s\n", pl_mqtt_strerror(rc));
    PL_fail;
  }
  PL_succeed;
}

foreign_t swi_mqtt_is_connected(term_t in_client)
{
  pl_mqtt_client *c;
  if (!get_ptr(in_client, (void**)&c)) { PL_fail; }
  return pl_mqtt_is_connected(c) ? TRUE : FALSE;
}

foreign_t swi_mqtt_disconnect(term_t in_client)
{
  pl_mqtt_client *c;
  if (!get_ptr(in_client, (void**)&c)) { PL_fail; }
  return (pl_mqtt_disconnect(c) == 0) ? TRUE : FALSE;
}

foreign_t swi_mqtt_destroy(term_t in_client)
{
  pl_mqtt_client *c;
  if (!get_ptr(in_client, (void**)&c)) { PL_fail; }
  pl_mqtt_client_destroy(c);
  PL_succeed;
}

foreign_t swi_mqtt_publish(term_t in_client, term_t in_topic, term_t in_payload,
                           term_t in_qos, term_t in_retain)
{
  pl_mqtt_client *c;
  if (!get_ptr(in_client, (void**)&c)) { PL_fail; }

  char *topic;
  if (!PL_get_chars(in_topic, &topic, CVT_ATOM|CVT_STRING|REP_UTF8)) { PL_fail; }

  char *payload;
  size_t len;
  if (!PL_get_nchars(in_payload, &len, &payload, TEXT_FLAGS)) { PL_fail; }

  int qos, retain;
  if (!PL_get_integer(in_qos, &qos)) { PL_fail; }
  if (!get_bool(in_retain, &retain)) { PL_fail; }

  int rc = pl_mqtt_publish(c, topic, payload, len, qos, retain);
  if (rc != 0)
  {
    fprintf(stderr, "ERROR - mqtt publish: %s\n", pl_mqtt_strerror(rc));
    PL_fail;
  }
  PL_succeed;
}

foreign_t swi_mqtt_flush(term_t in_client, term_t in_timeout)
{
  pl_mqtt_client *c;
  if (!get_ptr(in_client, (void**)&c)) { PL_fail; }
  int timeout;
  if (!PL_get_integer(in_timeout, &timeout)) { PL_fail; }
  return (pl_mqtt_flush(c, timeout) == 0) ? TRUE : FALSE;
}

foreign_t swi_mqtt_subscribe(term_t in_client, term_t in_pattern, term_t in_qos)
{
  pl_mqtt_client *c;
  if (!get_ptr(in_client, (void**)&c)) { PL_fail; }
  char *pattern;
  if (!PL_get_chars(in_pattern, &pattern, CVT_ATOM|CVT_STRING|REP_UTF8)) { PL_fail; }
  int qos;
  if (!PL_get_integer(in_qos, &qos)) { PL_fail; }

  int rc = pl_mqtt_subscribe(c, pattern, qos);
  if (rc != 0)
  {
    fprintf(stderr, "ERROR - mqtt subscribe: %s\n", pl_mqtt_strerror(rc));
    PL_fail;
  }
  PL_succeed;
}

foreign_t swi_mqtt_unsubscribe(term_t in_client, term_t in_pattern)
{
  pl_mqtt_client *c;
  if (!get_ptr(in_client, (void**)&c)) { PL_fail; }
  char *pattern;
  if (!PL_get_chars(in_pattern, &pattern, CVT_ATOM|CVT_STRING|REP_UTF8)) { PL_fail; }
  return (pl_mqtt_unsubscribe(c, pattern) == 0) ? TRUE : FALSE;
}

/* meta: [topic(T), qos(Q), retain(true|false), mid(M)] */
static int swi_unify_mqtt_message(const pl_mqtt_message *msg, term_t out_msg, term_t out_meta)
{
  if (!PL_is_variable(out_msg)) { return -2; }
  if (!PL_is_variable(out_meta)) { return -3; }

  /* payloads that are not valid UTF-8 are passed on byte-for-byte as Latin-1 */
  int enc = pl_mqtt_is_utf8(msg->payload, msg->payload_len) ? REP_UTF8 : REP_ISO_LATIN_1;
  if (!PL_unify_chars(out_msg, PL_ATOM|enc, msg->payload_len, (char*)msg->payload)) { return 1; }

  term_t ele = PL_new_term_ref();
  term_t lst = PL_copy_term_ref(out_meta);

  functor_t fct = PL_new_functor(PL_new_atom("topic"), 1);
  if (!PL_unify_list(lst, ele, lst)) { return 2; }
  if (!PL_unify_term(ele, PL_FUNCTOR, fct,
                          PL_UTF8_CHARS, msg->topic)) { return 3; }
  fct = PL_new_functor(PL_new_atom("qos"), 1);
  if (!PL_unify_list(lst, ele, lst)) { return 4; }
  if (!PL_unify_term(ele, PL_FUNCTOR, fct,
                          PL_INT, msg->qos)) { return 5; }
  fct = PL_new_functor(PL_new_atom("retain"), 1);
  if (!PL_unify_list(lst, ele, lst)) { return 6; }
  if (!PL_unify_term(ele, PL_FUNCTOR, fct,
                          PL_BOOL, msg->retain)) { return 7; }
  fct = PL_new_functor(PL_new_atom("mid"), 1);
  if (!PL_unify_list(lst, ele, lst)) { return 8; }
  if (!PL_unify_term(ele, PL_FUNCTOR, fct,
                          PL_INT, msg->mid)) { return 9; }

  if (PL_unify_nil(lst) != TRUE) { return 99; }
  return 0;
}

foreign_t swi_mqtt_poll(term_t in_client, term_t in_timeout, term_t out_message, term_t out_meta)
{
  pl_mqtt_client *c;
  if (!get_ptr(in_client, (void**)&c)) { PL_fail; }
  int timeout;
  if (!PL_get_integer(in_timeout, &timeout)) { PL_fail; }
  if (!PL_is_variable(out_message)) { PL_fail; }
  if (!PL_is_variable(out_meta)) { PL_fail; }

  pl_mqtt_message *msg = pl_mqtt_poll(c, timeout);
  if (!msg) { PL_fail; }   /* timed out */

  int res = swi_unify_mqtt_message(msg, out_message, out_meta);
  pl_mqtt_message_free(msg);

  if (res != 0)
  {
    fprintf(stderr, "ERROR: poll message unification returned: %d\n", res);
    PL_fail;
  }
  PL_succeed;
}

foreign_t swi_mqtt_dropped(term_t in_client, term_t out_count)
{
  pl_mqtt_client *c;
  if (!get_ptr(in_client, (void**)&c)) { PL_fail; }
  return PL_unify_int64(out_count, (int64_t)pl_mqtt_dropped(c));
}
