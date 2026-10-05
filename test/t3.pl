% self-contained round trip: publishes to topics it subscribed to itself
% and checks what comes back. needs a broker on localhost:1883, e.g.
%   mosquitto -p 1883
% run this test:
%   SWI:  $ swipl -q -g "consult('src/swi-mqtt.pl')" -g "consult('test/t3.pl')" -g test
%   GNU:  $ gplc -o t3 src/gp-mqtt.pl src/mqtt.pl test/t3.pl \
%               libplmqtt-@ARCH@.a -L -lmosquitto
%         $ echo test. | ./t3
% exits with status 0 on success, 1 on failure.

test :-
  ( run_test -> format("PASSED~n", []), halt(0)
  ; format("FAILED~n", []), halt(1)
  ).

run_test :-
  mqtt_conf_new(Config),
  mqtt_conf_set(Config, 'client.id', 'pl-mqtt-t3'),
  mqtt_conf_set(Config, 'keepalive', 30),
  mqtt_conf_dump(Config, Dump),
  Dump = [_|_],

  mqtt_client_new(Config, Client),
  mqtt_connect(Client),
  mqtt_is_connected(Client),

  mqtt_subscribe(Client, ['t3/one', 't3/+/two'], 1),
  mqtt_subscribe(Client, 't3/wild/#'),

  mqtt_publish(Client, 't3/one', 'first', 1),
  mqtt_publish(Client, 't3/x/two', 'second', 0),
  mqtt_publish(Client, 't3/wild/a/b', 'third', 1),
  mqtt_publish(Client, 't3/ignored', 'not subscribed'),
  mqtt_flush(Client, 2000),

  collect(Client, 3, Got),
  msort(Got, Sorted),
  Sorted = [ m(first,  't3/one'),
             m(second, 't3/x/two'),
             m(third,  't3/wild/a/b') ],

  % nothing else may arrive
  \+ mqtt_poll(Client, 300, _, _),

  % unsubscribed topics stay silent
  mqtt_unsubscribe(Client, 't3/one'),
  mqtt_publish(Client, 't3/one', 'again', 1),
  mqtt_flush(Client, 2000),
  \+ mqtt_poll(Client, 300, _, _),

  mqtt_dropped(Client, 0),

  mqtt_disconnect(Client),
  mqtt_destroy(Client),
  mqtt_conf_destroy(Config).

collect(_, 0, []) :- !.
collect(Client, N, [m(Msg, Topic)|T]) :-
  mqtt_poll(Client, 2000, Msg, Meta),
  member(topic(Topic), Meta),
  N1 is N - 1,
  collect(Client, N1, T).
