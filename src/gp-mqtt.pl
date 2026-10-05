/*-------------------------------------------------------------------------*/
/* Prolog Interface to MQTT -- GNU Prolog bridge                           */
/*                                                                         */
/* File  : gp-mqtt.pl                                                      */
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

/* the C functions are named gp_mqtt_* (see gp-mqtt-c.c); fct_name/1 maps
 * each one onto the pl_mqtt_* predicate name that the common mqtt.pl
 * (shared with the SWI backend) actually calls. */

:- foreign(pl_mqtt_version(term), [fct_name(gp_mqtt_version)]).
:- foreign(pl_mqtt_conf_new(term), [fct_name(gp_mqtt_conf_new)]).
:- foreign(pl_mqtt_conf_destroy(+term), [fct_name(gp_mqtt_conf_destroy)]).
:- foreign(pl_mqtt_conf_set(+term, +term, +term), [fct_name(gp_mqtt_conf_set)]).
:- foreign(pl_mqtt_conf_dump(+term, term), [fct_name(gp_mqtt_conf_dump)]).
:- foreign(pl_mqtt_client_new(+term, term), [fct_name(gp_mqtt_client_new)]).
:- foreign(pl_mqtt_connect(+term, +term), [fct_name(gp_mqtt_connect)]).
:- foreign(pl_mqtt_is_connected(+term), [fct_name(gp_mqtt_is_connected)]).
:- foreign(pl_mqtt_disconnect(+term), [fct_name(gp_mqtt_disconnect)]).
:- foreign(pl_mqtt_destroy(+term), [fct_name(gp_mqtt_destroy)]).
:- foreign(pl_mqtt_publish(+term, +term, +term, +term, +term), [fct_name(gp_mqtt_publish)]).
:- foreign(pl_mqtt_flush(+term, +term), [fct_name(gp_mqtt_flush)]).
:- foreign(pl_mqtt_subscribe(+term, +term, +term), [fct_name(gp_mqtt_subscribe)]).
:- foreign(pl_mqtt_unsubscribe(+term, +term), [fct_name(gp_mqtt_unsubscribe)]).
:- foreign(pl_mqtt_poll(+term, +term, term, term), [fct_name(gp_mqtt_poll)]).
:- foreign(pl_mqtt_dropped(+term, term), [fct_name(gp_mqtt_dropped)]).

/* mqtt.pl (shared with the SWI backend) is compiled and linked as a
 * sibling object -- see GP_SRCS/GP_OBJS in Makefile.in -- so its
 * mqtt_* guard predicates resolve at link time, not via a runtime
 * consult here. */
