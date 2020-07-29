# Lab #2: Verbs API Throughput

## Brief Description

This is a test measuring infiband verbs send throughput with various package size.

* **MAX\_INLINE** is not very useful here since the max inline size is 512 bytes, which is limited by the infiniband library. Packages smaller than 512 bytes won't effectively ultilize the bandiwidth.
* **WARM-UP** We do warm up round to get OS ready.
* **OVERHEAD** We minus the overhead of propagation time and server responding time to get a more accurate transmission time.
* **Best Record** We keep a recored of the best record of (1)highest peak throughput and (2)highest average throughput.

## Results

**The peak throughput is 81.920000 Gbps at 32768 bytes, while the best average throuput is 77.528725 Gbps at 1048576 bytes.**

Client log
```
$ ./run.sh jupiter012
pp_init_ctx... size = 1073741824, rx_depth 100, tx_depth 100.
pp_post_recv...
Warm up..
Starting Throughput calculation..
Throughput calcalation Complete..
servername jupiter012
  local address:  LID 0x0083, QPN 0x0002b5, PSN 0x7c2c3e, GID ::
  remote address: LID 0x0062, QPN 0x00025b, PSN 0x568b60, GID ::
======RTT avr 2.800000 ms======
size	throughput	unit
8192	54.613333	Gbps
16384	53.498776	Gbps
32768	62.415238	Gbps
65536	52.692261	Gbps
131072	73.843380	Gbps
262144	77.101176	Gbps
524288	70.551791	Gbps
1048576	77.528725	Gbps
2097152	74.499183	Gbps
4194304	71.743494	Gbps
8388608	69.510450	Gbps
16777216	68.963995	Gbps
33554432	69.252220	Gbps
67108864	69.146971	Gbps
134217728	69.167906	Gbps
268435456	69.205688	Gbps
536870912	69.190024	Gbps
Best peak throuput 81.920000 Gbps at 32768 bytes.
Best average throuput 77.528725 Gbps at 1048576 bytes.
```

## Build

```
make
```

## Run

### Using the script to run
Server
```
./run.sh 
```
Client
```
./run.sh <server-name>
```

### Run directly

Server
```
./verbs-demo -d mlx5_0
```
Client
```
./verbs-demo -d mlx5_0 <server-name>
```
