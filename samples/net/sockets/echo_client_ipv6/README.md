

1. Added  USE_POLL_EVENT define
Use USE_POLL_EVENT if you want to use sockets events

2. Removed UDP references and files

3. Added  wait_event(int ) routine on echo-client.c

4. Add 1 second delay on process_tcp_proto() before send_tcp_data.
The delay avoids overflow on the minicom console
