require(package "stream_manager")

c0 :: Classifier(12/0806 20/0001,
                  12/0806 20/0002,
                  12/0800,
                  -);

c1 :: Classifier(12/0806 20/0001,
                  12/0806 20/0002,
                  12/0800,
                  -);

FromDevice(eth0, PROMISC true, SNIFFER false) -> c0 ;

FromDevice(eth1, PROMISC true, SNIFFER false) -> c1 ;

out0 :: Queue
//    -> ARPPrint(TIMESTAMP false, ETHER true)
    -> ToDevice(eth0)
    -> Discard ;
out1 :: Queue
//    -> ARPPrint(TIMESTAMP false, ETHER true)
//    -> ToDevice(eth1)
    -> Discard ;
/*
arpq0 :: ARPQuerier(169.254.9.88, 94:57:A5:8E:12:F4) -> out0 ;
arpq1 :: ARPQuerier(169.254.9.93, 94:57:A5:8E:12:F5) -> out1 ;

ar0 :: ARPResponder(169.254.9.88/16 94:57:A5:8E:12:F4) ;

ar1 :: ARPResponder(169.254.9.93/16 94:57:A5:8E:12:F5) ;
*/

arpq0 :: ARPQuerier(1.1.1.1, 94:57:A5:8E:12:F4) -> out0 ;
arpq1 :: ARPQuerier(2.2.2.1, 94:57:A5:8E:12:F5) -> out1 ;

ar0 :: ARPResponder(1.1.1.1/24 94:57:A5:8E:12:F4) ;

ar1 :: ARPResponder(2.2.2.1/24 94:57:A5:8E:12:F5) ;

c1[0] -> ar0 -> out1;
c0[0] -> ar1 -> out0;



c0[1] ->[1]arpq0 ;
c1[1] ->[1]arpq1 ;

stream :: StreamManager;
stream[0] -> Discard ;
stream[1] -> SetTCPChecksum -> SetIPChecksum
//            -> IPPrint(TIMESTAMP false, LENGTH true)
            -> arpq0 ;
stream[2] -> Discard ;


cl :: IPClassifier(tcp dest port 9876,
                    tcp dest port 48698 and syn,
                    tcp dest port 48698 and (fin or rst),
                    tcp dest port 48698,
                    -);

//cl :: IPClassifier(src host 10.42.0.0/24, tcp and syn and not ack, tcp and (fin or rst), tcp and ack, -);
cl[0]->[0]arpq1 ;  // High priority, pass along
cl[1]->[0]stream;  // add stream
cl[2]->[2]stream;  // remove stream
cl[3]->[1]stream;  // update stream
cl[4]->[3]stream;  // do nothing, pass packets along


fr0 :: IPFragmenter(1500) ;
fr0[1] -> ICMPError(1.1.1.1, unreachable, needfrag) -> Discard ;

fr1 :: IPFragmenter(1500) ;
fr1[1] -> ICMPError(2.2.2.1, unreachable, needfrag) -> Discard ;


ip0 :: Strip(14)
    -> MarkIPHeader
    -> fr0
    -> CheckIPHeader2(VERBOSE true)
//    -> IPPrint(TIMESTAMP false,  CONTENTS NONE)
    ->  cl ;

c0[2]-> ip0;

ip1 :: Strip(14)
    -> MarkIPHeader
    -> fr1
    -> CheckIPHeader(VERBOSE true)
//    -> IPPrint(TIMESTAMP false, TOS true, CONTENTS NONE)
    -> [4]stream ;

c1[2]-> ip1;
c0[3] -> Discard;
c1[3] -> Discard;

