# RDMA Competition Exercise #3 - UCX Throughput

## Brief Description

This is a test measuring **ucx tag API** send throughput with various package size.

- **WARM-UP** We do warm up round to get OS ready.
- **OVERHEAD** We minus the overhead of propagation time and server responding time to get a more accurate transmission time.



## Results

The best throughput is **81.655933 Gbps** at 262144 bytes per package.



## example run

buid

```
make
```



server

```
./ucp_client_server
```



client

```
[ziyi@jupiter012 ex3-ucx-thpt]$ ./ucp_client_server -a 192.168.130.11
ALL SIZE: 16777216
16 1.725609 Gbps
32 3.479385 Gbps
64 6.460019 Gbps
128 12.219908 Gbps
256 18.504370 Gbps
512 29.448716 Gbps
1024 45.021975 Gbps
2048 43.758112 Gbps
4096 12.024635 Gbps
8192 19.337466 Gbps
16384 28.964125 Gbps
32768 38.600643 Gbps
65536 54.205561 Gbps
131072 70.076956 Gbps
262144 81.655933 Gbps
524288 70.180185 Gbps
1048576 65.203801 Gbps
2097152 64.930335 Gbps
4194304 65.823046 Gbps
8388608 64.157472 Gbps
16777216 63.817848 Gbps
```


