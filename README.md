# RDMA Competition Exercise #1 - Throughput


Key idea: Send 1 Gb data to server with different packet size. 

> Sizes/Byte: 1 2 4 8 16 32 64 128 256 512 1024 


## Build

```
make
```

## Run

Server side:

``` 
./server
```

Client side:

```
./client <server-ip-address>
```

## Example 

```
$ ./client 192.168.0.11
1   13.703065   Mbps
2   27.751375   Mbps
4   52.575544   Mbps
8   102.855527  Mbps
16  198.683720  Mbps
32  381.679389  Mbps
64  743.770919  Mbps
128 871.459695  Mbps
256 863.837599  Mbps
512 878.155873  Mbps
1024    864.024193  Mbps
```
