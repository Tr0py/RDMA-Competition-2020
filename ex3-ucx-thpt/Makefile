all:
	gcc ucp_client_server.c -lucp -lucs -I/global/home/users/ziyi/ucx-source/install/include -L/global/home/users/ziyi/ucx-source/install/lib -Wl,-rpath,/global/home/users/ziyi/ucx-source/install/lib -o ucp_client_server 

clean:
	rm -f ucp_client_server
