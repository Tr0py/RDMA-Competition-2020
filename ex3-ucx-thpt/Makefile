all:
	gcc ucp_client_server.c -lucp -lucs -o ucp_client_server -luct -lucs -I/global/home/users/ziyi/ucx-source/install/include -L/global/home/users/ziyi/ucx-source/install/lib -Wall -Werror -Wl,-rpath,/global/home/users/ziyi/ucx-source/install/lib  

clean:
	rm -f ucp_client_server
