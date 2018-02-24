all: unix_socket_server unix_socket_client

unix_socket_server: unix_socket_server.c migrate_file.c
	$(CC) -o $@ $^ $(CFLAGS)

unix_socket_client: unix_socket_client.c migrate_file.c
	$(CC) -o $@ $^ $(CFLAGS)


clean:
	rm -f unix_socket_server unix_socket_client
