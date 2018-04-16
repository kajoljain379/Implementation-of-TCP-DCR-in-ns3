# Implementation TCP-DCR algorithm in ns3
## Course Code: CS821	<br/>
## Assignment: #3	<br/>
### Overview		<br/>
TCP has a very important and appreciated feature of Congestion Control. Traditional implementations of TCP assume packet loss mainly due to congestion.This is well suited for wired connections as the probability of packet loss due to channel errors is almost negligible and hence the main reason for packet loss is congestion.
However in wireless this is not the case. In wireless networks significant number of packet losses could be due to channel errors. Hence assuming congestion to be the reason of packet loss every time and reducing the size of congestion window could be dangerous to the utilization of bandwidth in the network.
TCP DCR attempts to solve this issue. It increases the time at which fast retransmit/recovery algorithms are triggered by a time interval 𝜏. This gives a chance to the link layer to recover the packet if it is due to channel errors. If not it triggers the congestion algorithms.

#### Use the following steps to execute

1.) Configure and build the repository using the following commands:
 - ./waf configure
 - ./waf

2.) Run written example with enable and disable dcr
 - ./waf --run="scratch/example_dcr2 --DCR=true --edt=40.0 " > outp.txt
 - ./waf --run="scratch/example_dcr2 --DCR=true --edt=40.0 " > outp_dis.txt

##### Output file will be available in file outp.txt

### References
1. TCP DCR paper IEEE   https://ieeexplore.ieee.org/document/1492363/ 

