% this test receives a few messages from a topic filter
%
% start a publisher:
%   mosquitto_pub -h localhost -t pl/mqtt/demo -m 'some data'
% then, run this test (it ends after 10 s without any message):
%   SWI:  $ swipl -q -g "consult('src/swi-mqtt.pl')" -g "consult('test/t2.pl')" -g test
%   GNU:  $ gplc -o t2 src/gp-mqtt.pl src/mqtt.pl test/t2.pl \
%               libplmqtt-@ARCH@.a -L -lmosquitto
%         $ echo test. | ./t2
%

test :-
  run_test,
  halt.

run_test :-
  mqtt_conf_new(Config),
  mqtt_conf_set(Config, 'client.id', 'pl-mqtt-t2'),
  mqtt_conf_set(Config, 'host', 'localhost'),

  mqtt_client_new(Config, Client),
  mqtt_connect(Client),

  % wildcards + (one level) and # (all remaining levels) are allowed
  mqtt_subscribe(Client, ['pl/mqtt/#'], 1),

  read_messages(Client),

  mqtt_unsubscribe(Client, 'pl/mqtt/#'),
  mqtt_destroy(Client),
  format("client destroyed~n", []).

% reads until a poll times out (mqtt_poll/4 fails)
read_messages(Client) :-
  mqtt_poll(Client, 10000, Msg, Meta),
  !,
  format("read payload: ~p~n     meta: ~p ~n", [Msg, Meta]),
  read_messages(Client).
read_messages(_) :-
  format("timed out~n", []).
