require(package "stream_manager")

//end :: Queue(500)-> IPPrint(TIMESTAMP false, TOS true, CONTENTS NONE)-> ToDump(FILENAME "MyDump.pcap", ENCAP IP)->Discard;
end :: Queue(500)-> ToDump(FILENAME "MyDump.pcap", ENCAP IP)->Discard;
//end :: ToDump(FILENAME "MyDump.pcap", ENCAP IP)->Discard;

stream :: StreamManager
stream[0]->end;
stream[1]->SetTCPChecksum->SetIPChecksum->end;


cl :: IPClassifier(src host 2.0.0.1, tcp and syn and not ack, tcp and (fin or rst), tcp and ack, -);
cl[0]->[0]end;
cl[1]->[0]stream;// add stream
cl[2]->[2]stream;// remove stream
cl[3]->[1]stream;// update stream
cl[4]->[3]stream;// do nothing, pass packets along

source :: FastTCPFlows(100000, 5000, 60, 0:0:0:0:0:0, 2.0.0.1, 1:1:1:1:1:1, 2.0.0.2, 10, 10);
 

p:: PullTee(2);
p[0]->Discard;
p[1]->cl;



/*InfiniteSource(DATA \<
  // Ethernet header
  00 00 c0 ae 67 ef  00 00 00 00 00 00  08 00
  // IP header
  45 11 00 28  00 00 00 00  40 11 77 c3  02 00 00 01  02 00 00 02
  // UDP header
  13 69 13 69  00 14 d6 41
  // UDP payload
  55 44 50 20  70 61 63 6b  65 74 21 0a  04 00 00 00  01 00 00 00  
  01 00 00 00  00 00 00 00  00 80 04 08  00 80 04 08  53 53 00 00
  53 53 00 00  05 00 00 00  00 10 00 00  01 00 00 00  54 53 00 00
  54 e3 04 08  54 e3 04 08  d8 01 00 00
>, LIMIT 25, STOP true)*/



source
	-> Strip(14)
	-> Align(4, 0)    // in case we're not on x86
	-> CheckIPHeader(VERBOSE true)	
	-> p;
