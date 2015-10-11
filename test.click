
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

out0 :: Queue(10000)
//    -> ARPPrint(TIMESTAMP false, ETHER true)
    -> ToDevice(eth0)
    -> Discard ;
out1 :: Queue(10000)
//    -> ARPPrint(TIMESTAMP false, ETHER true)
    -> ToDevice(eth1)
    -> Discard ;

arpq0 :: ARPQuerier(169.254.9.88, 94:57:A5:8E:12:F4) -> out0 ;
arpq1 :: ARPQuerier(169.254.9.93, 94:57:A5:8E:12:F5) -> out1 ;

ar0 :: ARPResponder(169.254.9.88/16 94:57:A5:8E:12:F4) ;

ar1 :: ARPResponder(169.254.9.93/16 94:57:A5:8E:12:F5) ;
c1[0] -> ar0 -> out1;
c0[0] -> ar1 -> out0;

tf :: Tee(2) ;
tf[0] -> out0 ;
tf[1] -> out1 ;

FromHost(fake0) -> tf ;

tol :: ToHost(fake0);

t0 :: Tee(2);
t0[0] -> tol;
t0[1] ->[1]arpq0

t1 :: Tee(2);
t1[0] -> tol;
t1[1] ->[1]arpq1

c0[1] -> t0;
c1[1] -> t1 ;

fr0 :: IPFragmenter(1500, HONOR_DF false) ;
fr0[1] -> ICMPError(169.254.9.88, unreachable, needfrag) -> arpq0 ;

fr1 :: IPFragmenter(1500, HONOR_DF false) ;
fr1[1] -> ICMPError(169.254.9.93, unreachable, needfrag) -> arpq1 ;

ip0 :: Strip(14)
    -> MarkIPHeader
    -> fr0
//    -> IPPrint(TIMESTAMP false, LENGTH true)
    -> CheckIPHeader(OFFSET 0, VERBOSE true)
    -> arpq1 ;

c0[2]-> ip0;


ip1 :: Strip(14)
    -> MarkIPHeader
    -> fr1
    -> CheckIPHeader2(OFFSET 0, VERBOSE true)
//    -> IPPrint(TIMESTAMP false, LENGTH true)
    -> arpq0 ;

c1[2]-> ip1;


c0[3] -> Discard;
c1[3] -> Discard;




