
c0 :: Classifier(12/0806 20/0001,
                  12/0806 20/0002,
                  12/0800,
                  -);

c1 :: Classifier(12/0806 20/0001,
                  12/0806 20/0002,
                  12/0800,
                  -);

FromDevice(eth0, PROMISC true) -> c0 ;

FromDevice(eth1, PROMISC true) -> c1 ;

out0 :: Queue(5000)
//    -> ARPPrint(TIMESTAMP false, ETHER true)
    -> ToDevice(eth0)
    -> Discard ;
out1 :: Queue(5000)
//    -> ARPPrint(TIMESTAMP false, ETHER true)
    -> ToDevice(eth1)
    -> Discard ;

arpq0 :: ARPQuerier(169.254.9.88, 94:57:A5:8E:12:F4) -> out0 ;
arpq1 :: ARPQuerier(169.254.9.93, 94:57:A5:8E:12:F5) -> out1 ;

ar0 :: ARPResponder(169.254.9.88/16 94:57:A5:8E:12:F4) ;

ar1 :: ARPResponder(169.254.9.93/16 94:57:A5:8E:12:F5) ;
c1[0] -> ar0 -> out1;
c0[0] -> ar1 -> out0;



c0[1] ->[1]arpq0 ;
c1[1] ->[1]arpq1 ;


ip0 :: Strip(14)
    -> CheckIPHeader(VERBOSE true)
//    -> IPPrint(TIMESTAMP false,  CONTENTS NONE)
    ->  arpq1 ;

c0[2]-> ip0;

ip1 :: Strip(14)
    -> CheckIPHeader(VERBOSE true)
//    -> IPPrint(TIMESTAMP false, TOS true, CONTENTS NONE)
    -> arpq0 ;

c1[2]-> ip1;
c0[3] -> Discard;
c1[3] -> Discard;




