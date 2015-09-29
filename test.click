c0 :: Classifier(12/0806 20/0001,
                  12/0806 20/0002,
                  12/0800,
                  -);


FromDevice(eth0) -> [0]c0;

out0 :: Queue(200) -> ToDevice(eth0);

//This packet goes to linux stack
tol :: ToHost(fake);


// An "ARP querier" for each interface.
arpq0 :: ARPQuerier(192.168.1.100, AC:22:0B:C6:71:96);


// Deliver ARP responses to ARP queriers as well as Linux.
t :: Tee(2);
c0[1] -> t;

t[0] -> tol;
t[1] ->[1]arpq0

// Connect ARP outputs to the interface queues.
arpq0 -> out0;


// Proxy ARP on eth0 for 18.26.8, as well as cone's IP address.
ar0 :: ARPResponder(192.168.1.100 AC:22:0B:C6:71:96);
c0[0] -> ar0 -> out0;

rt :: RadixIPLookup(192.168.1.100/32 0,
                127.0.0.1/32 0,
                0.0.0.0/0 1);

ip ::   Strip(14)
     -> CheckIPHeader(VERBOSE true)
     -> [0]rt;

c0[2] -> Paint(1) -> ip ;

// IP packets for this machine.
// ToHost expects ethernet packets, so cook up a fake header.
rt[0] -> EtherEncap(0x0800, 1:1:1:1:1:1, 2:2:2:2:2:2) -> tol;

rt[1] -> arpq0;

FromHost(fake, 192.168.1.1/32)
    -> fromhost_cl :: Classifier(12/0806, 12/0800);
 fromhost_cl[0] -> ARPResponder(0.0.0.0/0 1:1:1:1:1:1) -> ToHost(fake);
 fromhost_cl[1] -> ip // IP packets



// Unknown ethernet type numbers.
c0[3] -> Discard;
