% this test demonstrates how to publish data to a topic
%
% start a listener:
%   mosquitto_sub -h localhost -t 'pl/mqtt/#' -v
% then launch this test:
%   SWI:  $ swipl -q -g "consult('src/swi-mqtt.pl')" -g "consult('test/t1.pl')" -g test
%   GNU:  $ gplc -o t1 src/gp-mqtt.pl src/mqtt.pl test/t1.pl \
%               libplmqtt-@ARCH@.a -L -lmosquitto
%         $ echo test. | ./t1
%

test :-
  run_test,
  halt.

run_test :-
  mqtt_version(V),
  format("libmosquitto ~p~n", [V]),

  mqtt_conf_new(Config),
  mqtt_conf_set(Config, 'client.id', 'pl-mqtt-t1'),
  mqtt_conf_set(Config, 'host', 'localhost'),
  mqtt_conf_set(Config, 'port', 1883),

  mqtt_client_new(Config, Client),
  mqtt_connect(Client),
  format("connected~n", []),

  mqtt_publish(Client, 'pl/mqtt/hello', 'hello there.'),
  mqtt_publish(Client, 'pl/mqtt/qos1', 'at least once', 1),
  mqtt_publish(Client, 'pl/mqtt/retained', 'stays on the broker', 1, true),

  mqtt_flush(Client, 2000),
  format("published and flushed~n", []),

  mqtt_disconnect(Client),
  mqtt_destroy(Client),
  mqtt_conf_destroy(Config),
  format("client destroyed~n", []).
