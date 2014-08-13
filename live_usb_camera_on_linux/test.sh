#!/bin/bash

make
# valgrind --leak-check=yes ./rend_server &
# sleep 1
# valgrind --leak-check=yes ./rend_client

./rend_server &
sleep 1
./rend_client

