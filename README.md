
# Interfacing to MQTT from Prolog

Prolog bindings for [libmosquitto](https://mosquitto.org/), in the same layout and style as `pl_kafka`:
one Prolog-neutral C core (`src/mqtt_core.[ch]`), a thin SWI-Prolog bridge (`src/swi-mqtt-c.c`),
a thin GNU Prolog bridge (`src/gp-mqtt-c.c`, `src/gp-mqtt.pl`) and a shared Prolog layer (`src/mqtt.pl`).

Each client runs a network thread around libmosquitto's `mosquitto_loop_forever`. It reconnects
after any lost connection, also after the errors that make libmosquitto give up (a broken TLS
stream, a failed DNS lookup): the loop is restarted with a backoff of 1s doubling up to 60s,
until `mqtt_disconnect/1` or `mqtt_destroy/1`. Lost connections and reconnects are logged to
stderr. Incoming messages are queued and fetched with `mqtt_poll/4`; subscriptions are renewed
after a reconnect.

## compilation

requires libmosquitto (`libmosquitto-dev`), SWI-Prolog and/or GNU Prolog, autoconf, pkg-config.

```sh
aclocal --force && autoheader --force && autoconf --force
```
then run
```sh
./configure
```
then call `make swi`, `make gp`

install for SWI-Prolog like `pl_kafka` (the foreign library is called `plmqtt`):
```sh
mkdir -p ${HOME}/lib/sbcl
cp plmqtt-Linux ${HOME}/lib/sbcl/plmqtt
cp src/mqtt.qlf src/swi-mqtt.pl ${HOME}/lib/sbcl/
echo ":- assertz(file_search_path(sbcl,'${HOME}/lib/sbcl'))." >> ${HOME}/.config/swi-prolog/init.pl
```

## predicates

| predicate | meaning |
|-----------|---------|
| `mqtt_version(-V)` | version of libmosquitto |
| `mqtt_conf_new(-Conf)` / `mqtt_conf_destroy(+Conf)` | configuration object |
| `mqtt_conf_set(+Conf, +Key, +Value)` | set an option (values may be atoms, strings or integers) |
| `mqtt_conf_dump(+Conf, -Pairs)` | list of `Key(Value)` (the password is masked) |
| `mqtt_client_new(+Conf, -Client)` | create a client (does not connect) |
| `mqtt_connect(+Client[, +TimeoutMs])` | connect, wait for CONNACK (default 5000 ms) |
| `mqtt_is_connected(+Client)` | true while the session is up |
| `mqtt_publish(+Client, +Topic, +Payload[, +Qos[, +Retain]])` | publish; Qos 0..2 (default 0), Retain `true`/`false` |
| `mqtt_flush(+Client, +TimeoutMs)` | wait until published messages were handed to the broker |
| `mqtt_subscribe(+Client, +TopicOrList[, +Qos])` | subscribe; `+` and `#` wildcards allowed |
| `mqtt_unsubscribe(+Client, +TopicOrList)` | |
| `mqtt_poll(+Client, +TimeoutMs, -Payload, -Meta)` | next message; fails on timeout. `Meta = [topic(T), qos(Q), retain(Bool), mid(Id)]` |
| `mqtt_dropped(+Client, -Count)` | messages dropped because the queue was full |
| `mqtt_disconnect(+Client)` / `mqtt_destroy(+Client)` | `destroy` also stops the network thread and frees the client |

configuration keys (defaults in brackets):

`client.id` (random), `host` (localhost), `port` (1883), `keepalive` (60), `clean.session` (true),
`protocol.version` (3.1.1; also 3.1, 5), `username`, `password`, `tls.cafile`, `tls.capath`,
`tls.certfile`, `tls.keyfile`, `tls.insecure` (false), `will.topic`, `will.payload`, `will.qos` (0),
`will.retain` (false), `queue.max` (100000 received messages kept for `mqtt_poll`).

`clean.session=false` requires a `client.id`. Payloads and topics are UTF-8 text; a received payload that is
not valid UTF-8 is returned byte-for-byte as a Latin-1 atom (SWI-Prolog; with GNU Prolog payloads
containing NUL bytes are cut at the first one). Like in `pl_kafka`, predicates fail on errors and
print a message to stderr.

## examples

```prolog
?- mqtt_conf_new(C), mqtt_conf_set(C, host, 'test.mosquitto.org'),
   mqtt_client_new(C, Cl), mqtt_connect(Cl),
   mqtt_subscribe(Cl, 'pl/mqtt/#', 1),
   mqtt_publish(Cl, 'pl/mqtt/hi', 'hello', 1, false),
   mqtt_poll(Cl, 2000, Msg, Meta).
Msg = hello,
Meta = [topic('pl/mqtt/hi'), qos(1), retain(false), mid(1)].
```

see `test/`: `t1.pl` publishes, `t2.pl` receives, `t3.pl` is a self-contained round trip that exits
with status 0/1 (needs a broker on localhost:1883, e.g. `mosquitto -p 1883`):

```sh
swipl -q -g "consult('src/swi-mqtt.pl')" -g "consult('test/t3.pl')" -g test
gplc -o t3 src/gp-mqtt.pl src/mqtt.pl test/t3.pl libplmqtt-Linux.a -L -lmosquitto && echo 'test.' | ./t3
```

note for GNU Prolog: in `src/mqtt.pl` the exported predicates share local helpers instead of calling
each other, because gplc resolves such calls through the module qualifier at link time.
