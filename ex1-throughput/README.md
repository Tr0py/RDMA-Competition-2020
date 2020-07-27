# RDMA Competition Exercise #1 - Throughput


Key idea: We measure the time of client transfering $N$ bytes of data to server, so the throughput between them is $N/Time$. 

A little more detail: Every time we send $N$ bytes of data to server with different packet size, when server receives all of the data, it tells the client.  (Server replys "done" when it receives $N$ bytes of data.)

** Features:  **

* We do warm-up rounds to get OS ready first.
* We keep measuring the throughput until is stable (varaiation < 1%).


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
Notice: The default port that server listen at is 6666, please check if the port is available.

Client side:

```
./client <server-ip-address>
```

## Example 

```
$ ./client 192.168.0.11
1   12.384372   Mbps
2   24.345457   Mbps
4   46.496895   Mbps
8   89.088681   Mbps
16  190.333329  Mbps
32  365.149548  Mbps
64  719.411224  Mbps
128 866.308588  Mbps
256 877.260250  Mbps
512 873.135229  Mbps
1024    874.324603  Mbps
```
