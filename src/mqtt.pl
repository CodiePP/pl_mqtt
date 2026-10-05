/*-------------------------------------------------------------------------*/
/* Prolog Interface to MQTT                                                */
/*                                                                         */
/* File  : mqtt.pl                                                         */
/* Author: Alexander Diemand                                               */
/*                                                                         */
/* Copyright (C) 2026 Alexander Diemand                                    */
/*                                                                         */
/*   This program is free software: you can redistribute it and/or modify  */
/*   it under the terms of the GNU General Public License as published by  */
/*   the Free Software Foundation, either version 3 of the License, or     */
/*   (at your option) any later version.                                   */
/*                                                                         */
/*   This program is distributed in the hope that it will be useful,       */
/*   but WITHOUT ANY WARRANTY; without even the implied warranty of        */
/*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         */
/*   GNU General Public License for more details.                          */
/*                                                                         */
/*   You should have received a copy of the GNU General Public License     */
/*   along with this program.  If not, see <http://www.gnu.org/licenses/>. */
/*-------------------------------------------------------------------------*/

:- module(mqtt, [ mqtt_version/1
                , mqtt_conf_new/1
                , mqtt_conf_set/3
                , mqtt_conf_destroy/1
                , mqtt_conf_dump/2
                , mqtt_client_new/2
                , mqtt_connect/1
                , mqtt_connect/2
                , mqtt_is_connected/1
                , mqtt_disconnect/1
                , mqtt_destroy/1
                , mqtt_publish/3
                , mqtt_publish/4
                , mqtt_publish/5
                , mqtt_flush/2
                , mqtt_subscribe/2
                , mqtt_subscribe/3
                , mqtt_unsubscribe/2
                , mqtt_poll/4
                , mqtt_dropped/2
                ]).

% mqtt_version(-Atom)
% version of the libmosquitto this module is linked against
mqtt_version(V) :-
  pl_mqtt_version(V).

% mqtt_conf_new(-Cid)
mqtt_conf_new(Cid) :-
  var(Cid),
  pl_mqtt_conf_new(Cid).

% mqtt_conf_set(+Cid, +Key, +Value)
% keys: client.id host port keepalive clean.session protocol.version
%       username password tls.cafile tls.capath tls.certfile tls.keyfile
%       tls.insecure will.topic will.payload will.qos will.retain queue.max
mqtt_conf_set(Cid, K, V) :-
  nonvar(Cid), atom(K), nonvar(V),
  mqtt_text(V, VA),
  pl_mqtt_conf_set(Cid, K, VA).

% mqtt_text(+Text, -Atom)
% atoms, numbers, strings and code lists are all accepted as text
mqtt_text(T, A) :- atom(T), !, A = T.
mqtt_text(T, A) :- integer(T), !, number_codes(T, Cs), atom_codes(A, Cs).
mqtt_text(T, A) :- atom_codes(A, T).

% mqtt_conf_destroy(+Cid)
mqtt_conf_destroy(Cid) :-
  nonvar(Cid),
  pl_mqtt_conf_destroy(Cid).

% mqtt_conf_dump(+Cid, -ConfPairs)
mqtt_conf_dump(Cid, ConfPairs) :-
  nonvar(Cid), var(ConfPairs),
  pl_mqtt_conf_dump(Cid, ConfPairs).

% mqtt_client_new(+Cid, -Client)
% creates a client from the configuration; does not connect yet
mqtt_client_new(Cid, Client) :-
  nonvar(Cid), var(Client),
  pl_mqtt_client_new(Cid, Client).

% mqtt_connect(+Client)
% connects to the broker, waiting up to 5 seconds for its acknowledgement
mqtt_connect(Client) :-
  mqtt_connect_(Client, 5000).

% mqtt_connect(+Client, +Timeout)
mqtt_connect(Client, Timeout) :-
  mqtt_connect_(Client, Timeout).

mqtt_connect_(Client, Timeout) :-
  nonvar(Client), integer(Timeout),
  pl_mqtt_connect(Client, Timeout).

% mqtt_is_connected(+Client)
mqtt_is_connected(Client) :-
  nonvar(Client),
  pl_mqtt_is_connected(Client).

% mqtt_disconnect(+Client)
mqtt_disconnect(Client) :-
  nonvar(Client),
  pl_mqtt_disconnect(Client).

% mqtt_destroy(+Client)
% disconnects, stops the network thread and frees the client
mqtt_destroy(Client) :-
  nonvar(Client),
  pl_mqtt_destroy(Client).

% note: the exported predicates do not call each other but share local
% helpers (name_/N), as GNU Prolog resolves intra-file calls to exported
% predicates through the module qualifier at link time.

% mqtt_publish(+Client, +Topic, +Payload)
% QoS 0, not retained
mqtt_publish(Client, Topic, Payload) :-
  mqtt_publish_(Client, Topic, Payload, 0, false).

% mqtt_publish(+Client, +Topic, +Payload, +Qos)
mqtt_publish(Client, Topic, Payload, Qos) :-
  mqtt_publish_(Client, Topic, Payload, Qos, false).

% mqtt_publish(+Client, +Topic, +Payload, +Qos, +Retain)
mqtt_publish(Client, Topic, Payload, Qos, Retain) :-
  mqtt_publish_(Client, Topic, Payload, Qos, Retain).

mqtt_publish_(Client, Topic, Payload, Qos, Retain) :-
  nonvar(Client), atom(Topic),
  nonvar(Payload), integer(Qos), Qos >= 0, Qos =< 2,
  ( Retain == true ; Retain == false ),
  mqtt_text(Payload, PayloadA),
  pl_mqtt_publish(Client, Topic, PayloadA, Qos, Retain).

% mqtt_flush(+Client, +Timeout)
% waits until all published messages were handed to the broker
mqtt_flush(Client, Timeout) :-
  nonvar(Client), integer(Timeout),
  pl_mqtt_flush(Client, Timeout).

% mqtt_subscribe(+Client, +TopicOrList)
mqtt_subscribe(Client, Topics) :-
  mqtt_subscribe_(Client, Topics, 0).

% mqtt_subscribe(+Client, +TopicOrList, +Qos)
% topics may contain the wildcards + and #
mqtt_subscribe(Client, Topics, Qos) :-
  mqtt_subscribe_(Client, Topics, Qos).

mqtt_subscribe_(Client, Topics, Qos) :-
  nonvar(Client), nonvar(Topics),
  integer(Qos), Qos >= 0, Qos =< 2,
  ( atom(Topics) -> pl_mqtt_subscribe(Client, Topics, Qos)
  ; mqtt_subscribe_list(Topics, Client, Qos)
  ).

mqtt_subscribe_list([], _, _).
mqtt_subscribe_list([T|Ts], Client, Qos) :-
  atom(T),
  pl_mqtt_subscribe(Client, T, Qos),
  mqtt_subscribe_list(Ts, Client, Qos).

% mqtt_unsubscribe(+Client, +TopicOrList)
mqtt_unsubscribe(Client, Topics) :-
  nonvar(Client), nonvar(Topics),
  ( atom(Topics) -> pl_mqtt_unsubscribe(Client, Topics)
  ; mqtt_unsubscribe_list(Topics, Client)
  ).

mqtt_unsubscribe_list([], _).
mqtt_unsubscribe_list([T|Ts], Client) :-
  atom(T),
  pl_mqtt_unsubscribe(Client, T),
  mqtt_unsubscribe_list(Ts, Client).

% mqtt_poll(+Client, +Timeout, -Payload, -Meta)
% waits up to Timeout ms for a message; fails on timeout.
% Meta = [topic(T), qos(Q), retain(Bool), mid(Id)]
mqtt_poll(Client, Timeout, Msg, Meta) :-
  nonvar(Client), integer(Timeout),
  var(Msg), var(Meta),
  pl_mqtt_poll(Client, Timeout, Msg, Meta).

% mqtt_dropped(+Client, -Count)
% messages dropped because the receive queue (queue.max) was full
mqtt_dropped(Client, Count) :-
  nonvar(Client),
  pl_mqtt_dropped(Client, Count).
