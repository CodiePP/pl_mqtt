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

#include "gprolog.h"
#include "mqtt_core.h"

/* opaque handles are passed between Prolog calls as plain integers
 * holding the pointer value, mirroring PL_unify_pointer/PL_get_pointer
 * on the SWI side. */
static void *rd_pointer_check(PlTerm t)
{
  return (void *)(intptr_t)Pl_Rd_Integer_Check(t);
}

/* payloads are (pointer,len) pairs that are not necessarily NUL-terminated;
 * GNU Prolog's Mk_String only takes a C string, so copy through a NUL-terminated
 * scratch buffer. payloads containing NUL bytes are cut at the first one. */
static PlTerm mk_string_n(const void *data, size_t len)
{
  char *buf = (char *)malloc(len + 1);
  PlTerm t;
  if (!buf) { return Pl_Mk_String(""); }
  if (data && len > 0) { memcpy(buf, data, len); }
  buf[len] = '\0';
  t = Pl_Mk_String(buf);
  free(buf);
  return t;
}

static PlBool un_string_n(const void *data, size_t len, PlTerm term)
{
  char *buf = (char *)malloc(len + 1);
  PlBool ok;
  if (!buf) { return PL_FALSE; }
  if (data && len > 0) { memcpy(buf, data, len); }
  buf[len] = '\0';
  ok = Pl_Un_String_Check(buf, term);
  free(buf);
  return ok;
}

PlBool gp_mqtt_version(PlTerm out_v)
{
  return Pl_Un_String_Check(pl_mqtt_version_str(), out_v);
}

PlBool gp_mqtt_conf_new(PlTerm out_cid)
{
  if (Pl_Builtin_Non_Var(out_cid)) { return PL_FALSE; }
  pl_mqtt_conf *kc = pl_mqtt_conf_new();
  if (!kc) { return PL_FALSE; }
  return Pl_Un_Integer_Check((PlLong)(intptr_t)kc, out_cid);
}

PlBool gp_mqtt_conf_destroy(PlTerm in_cid)
{
  if (Pl_Builtin_Var(in_cid)) { return PL_FALSE; }
  pl_mqtt_conf_destroy((pl_mqtt_conf *)rd_pointer_check(in_cid));
  return PL_TRUE;
}

PlBool gp_mqtt_conf_set(PlTerm in_cid, PlTerm in_k, PlTerm in_v)
{
  if (Pl_Builtin_Var(in_cid)) { return PL_FALSE; }
  pl_mqtt_conf *kc = (pl_mqtt_conf *)rd_pointer_check(in_cid);

  char *k_key = Pl_Rd_String_Check(in_k);
  char *k_val = Pl_Rd_String_Check(in_v);

  char errstr[128];
  if (pl_mqtt_conf_set(kc, k_key, k_val, errstr, sizeof(errstr)) != 0) {
    fprintf(stderr, "ERROR - mqtt conf set %s: %s\n", k_key, errstr);
    return PL_FALSE;
  }
  return PL_TRUE;
}

PlBool gp_mqtt_conf_dump(PlTerm in_cid, PlTerm out_list)
{
  if (Pl_Builtin_Var(in_cid)) { return PL_FALSE; }
  pl_mqtt_conf *kc = (pl_mqtt_conf *)rd_pointer_check(in_cid);
  if (Pl_Builtin_Non_Var(out_list)) { return PL_FALSE; }

  PlTerm elems[64];
  int n = 0;
  const char *key;
  char val[256];
  while (n < 64 && pl_mqtt_conf_entry(kc, n, &key, val, sizeof(val)) == 0) {
    PlTerm arg[1];
    arg[0] = Pl_Mk_String(val);
    elems[n] = Pl_Mk_Compound(Pl_Create_Atom((char *)key), 1, arg);
    n++;
  }
  return Pl_Un_Proper_List_Check(n, elems, out_list);
}

PlBool gp_mqtt_client_new(PlTerm in_cid, PlTerm out_client)
{
  if (Pl_Builtin_Var(in_cid)) { return PL_FALSE; }
  pl_mqtt_conf *kc = (pl_mqtt_conf *)rd_pointer_check(in_cid);
  if (Pl_Builtin_Non_Var(out_client)) { return PL_FALSE; }

  char errstr[128];
  pl_mqtt_client *c = pl_mqtt_client_new(kc, errstr, sizeof(errstr));
  if (!c) {
    fprintf(stderr, "ERROR - mqtt client new: %s\n", errstr);
    return PL_FALSE;
  }
  return Pl_Un_Integer_Check((PlLong)(intptr_t)c, out_client);
}

PlBool gp_mqtt_connect(PlTerm in_client, PlTerm in_timeout)
{
  if (Pl_Builtin_Var(in_client)) { return PL_FALSE; }
  pl_mqtt_client *c = (pl_mqtt_client *)rd_pointer_check(in_client);
  int timeout = (int)Pl_Rd_Integer_Check(in_timeout);

  int rc = pl_mqtt_connect(c, timeout);
  if (rc != 0) {
    fprintf(stderr, "ERROR - mqtt connect: %s\n", pl_mqtt_strerror(rc));
    return PL_FALSE;
  }
  return PL_TRUE;
}

PlBool gp_mqtt_is_connected(PlTerm in_client)
{
  if (Pl_Builtin_Var(in_client)) { return PL_FALSE; }
  return pl_mqtt_is_connected((pl_mqtt_client *)rd_pointer_check(in_client)) ? PL_TRUE : PL_FALSE;
}

PlBool gp_mqtt_disconnect(PlTerm in_client)
{
  if (Pl_Builtin_Var(in_client)) { return PL_FALSE; }
  return (pl_mqtt_disconnect((pl_mqtt_client *)rd_pointer_check(in_client)) == 0) ? PL_TRUE : PL_FALSE;
}

PlBool gp_mqtt_destroy(PlTerm in_client)
{
  if (Pl_Builtin_Var(in_client)) { return PL_FALSE; }
  pl_mqtt_client_destroy((pl_mqtt_client *)rd_pointer_check(in_client));
  return PL_TRUE;
}

PlBool gp_mqtt_publish(PlTerm in_client, PlTerm in_topic, PlTerm in_payload,
                       PlTerm in_qos, PlTerm in_retain)
{
  if (Pl_Builtin_Var(in_client)) { return PL_FALSE; }
  pl_mqtt_client *c = (pl_mqtt_client *)rd_pointer_check(in_client);

  char *topic = Pl_Rd_String_Check(in_topic);
  char *payload = Pl_Rd_String_Check(in_payload);
  int qos = (int)Pl_Rd_Integer_Check(in_qos);
  int retain = (strcmp(Pl_Rd_String_Check(in_retain), "true") == 0);

  int rc = pl_mqtt_publish(c, topic, payload, strlen(payload), qos, retain);
  if (rc != 0) {
    fprintf(stderr, "ERROR - mqtt publish: %s\n", pl_mqtt_strerror(rc));
    return PL_FALSE;
  }
  return PL_TRUE;
}

PlBool gp_mqtt_flush(PlTerm in_client, PlTerm in_timeout)
{
  if (Pl_Builtin_Var(in_client)) { return PL_FALSE; }
  pl_mqtt_client *c = (pl_mqtt_client *)rd_pointer_check(in_client);
  int timeout = (int)Pl_Rd_Integer_Check(in_timeout);
  return (pl_mqtt_flush(c, timeout) == 0) ? PL_TRUE : PL_FALSE;
}

PlBool gp_mqtt_subscribe(PlTerm in_client, PlTerm in_pattern, PlTerm in_qos)
{
  if (Pl_Builtin_Var(in_client)) { return PL_FALSE; }
  pl_mqtt_client *c = (pl_mqtt_client *)rd_pointer_check(in_client);
  char *pattern = Pl_Rd_String_Check(in_pattern);
  int qos = (int)Pl_Rd_Integer_Check(in_qos);

  int rc = pl_mqtt_subscribe(c, pattern, qos);
  if (rc != 0) {
    fprintf(stderr, "ERROR - mqtt subscribe: %s\n", pl_mqtt_strerror(rc));
    return PL_FALSE;
  }
  return PL_TRUE;
}

PlBool gp_mqtt_unsubscribe(PlTerm in_client, PlTerm in_pattern)
{
  if (Pl_Builtin_Var(in_client)) { return PL_FALSE; }
  pl_mqtt_client *c = (pl_mqtt_client *)rd_pointer_check(in_client);
  char *pattern = Pl_Rd_String_Check(in_pattern);
  return (pl_mqtt_unsubscribe(c, pattern) == 0) ? PL_TRUE : PL_FALSE;
}

/* meta: [topic(T), qos(Q), retain(true|false), mid(M)] */
static PlBool gp_unify_mqtt_message(const pl_mqtt_message *msg, PlTerm out_msg, PlTerm out_meta)
{
  if (Pl_Builtin_Non_Var(out_msg)) { return PL_FALSE; }
  if (Pl_Builtin_Non_Var(out_meta)) { return PL_FALSE; }

  if (!un_string_n(msg->payload, msg->payload_len, out_msg)) { return PL_FALSE; }

  PlTerm meta[4];
  PlTerm arg[1];

  arg[0] = Pl_Mk_String(msg->topic);
  meta[0] = Pl_Mk_Compound(Pl_Create_Atom("topic"), 1, arg);

  arg[0] = Pl_Mk_Integer((PlLong)msg->qos);
  meta[1] = Pl_Mk_Compound(Pl_Create_Atom("qos"), 1, arg);

  arg[0] = Pl_Mk_Atom(Pl_Create_Atom(msg->retain ? "true" : "false"));
  meta[2] = Pl_Mk_Compound(Pl_Create_Atom("retain"), 1, arg);

  arg[0] = Pl_Mk_Integer((PlLong)msg->mid);
  meta[3] = Pl_Mk_Compound(Pl_Create_Atom("mid"), 1, arg);

  return Pl_Un_Proper_List_Check(4, meta, out_meta);
}

PlBool gp_mqtt_poll(PlTerm in_client, PlTerm in_timeout, PlTerm out_message, PlTerm out_meta)
{
  if (Pl_Builtin_Var(in_client)) { return PL_FALSE; }
  pl_mqtt_client *c = (pl_mqtt_client *)rd_pointer_check(in_client);
  int timeout = (int)Pl_Rd_Integer_Check(in_timeout);

  if (Pl_Builtin_Non_Var(out_message)) { return PL_FALSE; }
  if (Pl_Builtin_Non_Var(out_meta)) { return PL_FALSE; }

  pl_mqtt_message *msg = pl_mqtt_poll(c, timeout);
  if (!msg) { return PL_FALSE; }   /* timed out */

  PlBool ok = gp_unify_mqtt_message(msg, out_message, out_meta);
  pl_mqtt_message_free(msg);

  if (!ok) {
    fprintf(stderr, "ERROR: poll message unification failed\n");
    return PL_FALSE;
  }
  return PL_TRUE;
}

PlBool gp_mqtt_dropped(PlTerm in_client, PlTerm out_count)
{
  if (Pl_Builtin_Var(in_client)) { return PL_FALSE; }
  pl_mqtt_client *c = (pl_mqtt_client *)rd_pointer_check(in_client);
  return Pl_Un_Integer_Check((PlLong)pl_mqtt_dropped(c), out_count);
}
