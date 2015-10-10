#include <click/config.h>
#include "stream_manager.hh"

CLICK_DECLS

StreamManager::StreamManager() {}
StreamManager::~StreamManager() {}

int StreamManager::initialize(ErrorHandler* errh) { return 0; }

int StreamManager::configure(Vector<String>& conf, ErrorHandler* errh)
{
    String channel;
    // errh->message("My int = %u", 22);
    if (Args(conf, this, errh).read("CHANNEL", WordArg(), channel).complete() < 0)
    {
        return -1;
    }

    _errh = router()->chatter_channel(channel);
    //_errh = errh;
    return 0;
}


void StreamManager::push(int port, Packet* p)
{
     handle_packet(port, p);
    /*if (p)
        output(0).push(p);
    */
}

/*Packet* StreamManager::pull(int port)
{
    Packet* p = input(port).pull();
    if (p)
        p = handle_packet(port, p);
    return p;
}
*/

/**
 * @brief Handles packets coming in on each port
 * @details [long description]
 *
 * @param port click port number
 * @param p an IP/TCP packet
 *
 * @return returns the packet to exit the elements output
 */
Packet* StreamManager::handle_packet(int port, Packet* p)
{
    // we don't support null packets, so return early...
    if (!p)
        return p;

    switch (port)
    {
    // add stream
    case 0:
        p = add_stream(p);
        output(0).push(p);
        break;

    // update timers
    case 1:
        p = update_stream(p);
        output(0).push(p);
        break;

    case 2:
        p = remove_stream(p);
        output(0).push(p);
        break;

    case 3:
        output(0).push(p);
        break;

    // remove stream
    case 4:
        p = update_ack(p);
        output(2).push(p);
        break;

    case 5:
        p = hp_streams(p);
        output(0).push(p);

    // not one of our ports, so do nothing, and pass the packet on.
    default:
        break;
    };

    return p;
}


Packet* StreamManager::hp_streams(Packet* p)
{
    const click_tcp* tcph = p->tcp_header();
    if(tcph->th_syn)
    {
        IPFlowID id(p, false);
        // assign the stream to the hashtable
        hp_lock.acquire();
        hp.set(id, 1);
        hp_lock.release();
    }
    else if((tcph->th_flags & TH_FIN) || (tcph->th_flags & TH_RST) )
    {
         IPFlowID id(p, false);
        // assign the stream to the hashtable
        hp_lock.acquire();
        if (hp.erase(id) == 0)
            hp.erase(id.reverse());
        hp_lock.release();
    }

    return p;
}

/**
 * @brief adds a stream to the table, with timers
 * @details uses a flow id to identify streams, and properly add them to the
 * hash table
 *
 * @param p pointer to a click packet
 * @return returns a packet to be output, identical to input
 */
Packet* StreamManager::add_stream(Packet* p)
{
/*  const click_ip* iph = p->ip_header();
    if (iph->ip_len < 40 || iph->ip_p != IPPROTO_TCP)
    {
        //   DEBUG_CHATTER("Non TCP");
        // ignore non-TCP traffic
        return p;
    }
*/
    const click_tcp* tcph = p->tcp_header();
    // int header_len = (iph->ip_hl << 2) + (tcph->th_off << 2);
    // int datalen = p->length() - header_len;

    // extract flow id
    // IPFlowID id(iph->ip_src.s_addr, tcph->th_sport, iph->ip_dst.s_addr,
    // tcph->th_dport);

    IPFlowID id(p, false);

    /*stream_data::cb_data cb;
    cb.ptr = NULL;
    cb.el = this;
*/

    // assign the stream to the hashtable
    tbl_lock.acquire();
    hash.set(id, stream_data(p, tcph->th_seq, tcph->th_ack, this));
    tbl_lock.release();

    return p;
}

Packet* StreamManager::remove_stream(Packet* p)
{
    const click_ip* iph = p->ip_header();
/*    if (p->length() < 40 || iph->ip_p != IPPROTO_TCP)
    {
        // DEBUG_CHATTER("Non TCP");
        // ignore non-TCP traffic
        return p;
    }
*/
    const click_tcp* tcph = p->tcp_header();

    // extract flow id
    IPFlowID id(iph->ip_src.s_addr, tcph->th_sport, iph->ip_dst.s_addr,
                tcph->th_dport);

    tbl_lock.acquire();
    if (hash.erase(id) == 0)
        hash.erase(id.reverse());

    tbl_lock.release();
    return p;
}

Packet* StreamManager::update_stream(Packet* p)
{
/*    const click_ip* iph = p->ip_header();
    if (p->length() < 40 || iph->ip_p != IPPROTO_TCP)
    {
        // DEBUG_CHATTER("Non TCP");
        // ignore non-TCP traffic
        return p;
    }

    const click_tcp* tcph = p->tcp_header();
*/
    // extract flow id
    // IPFlowID id(iph->ip_src.s_addr, tcph->th_sport,
    // iph->ip_dst.s_addr,tcph->th_dport);

    IPFlowID id(p, false);

    // lock the hash??
    tbl_lock.acquire();

    HashTable_iterator<Pair<const IPFlowID, stream_data> > it = hash.find(id);
    if (it != hash.end() && it->second.frozen)
    {
        it->second.send_zero_wnd();
    }
    tbl_lock.release();

    return p;
}

Packet* StreamManager::update_ack(Packet* p)
{
    const click_tcp* tcph = p->tcp_header();

    // extract flow id
    // IPFlowID id(iph->ip_src.s_addr, tcph->th_sport,
    // iph->ip_dst.s_addr,tcph->th_dport);
/*
    WritablePacket* zwa = p->clone()->uniqueify();
    click_tcp* th = zwa->tcp_header();
    th->th_win = 0;
    output(1).push(zwa);
*/
    IPFlowID id(p, false);

    // lock the hash??
    tbl_lock.acquire();

    HashTable_iterator<Pair<const IPFlowID, stream_data> > it = hash.find(id.reverse());
    if(it != hash.end())
    {
//        printf("ACK: %ld\t SEQ: %ld\n", ntohl(tcph->th_ack), ntohl(tcph->th_seq));
        //update the sequence number
/*        if (ntohl(it->second.seq) < ntohl(tcph->th_seq))
        {
            it->second.seq = tcph->th_seq;
        }

        // update the ack number
        if (ntohl(it->second.ack) < ntohl(tcph->th_ack))
        {
            it->second.ack = tcph->th_ack;
        }
*/
        // if the persist timer has expired, we've already sent
        // 0 WND notification. Unfreeze TCP with tri-ACK(maybe only send 2
        // here?)

        if (it->second.frozen)
        {
            //output(1).push(p->clone());
            //output(1).push(p->clone());
            //output(2).push(p->clone());
            it->second.frozen = false;
        }


//        it->second.send_zero_wnd();
        it->second.reset_timers();

        if(tcph->th_flags & (TH_RST | TH_FIN))
        {
            remove_stream(p);     //remove the stream if we see FIN or RST
        }
        else
        {
            WritablePacket* zwa = p->clone()->uniqueify();
            click_tcp* th = zwa->tcp_header();
            th->th_win = 0;
//            uint32_t old_ack = ntohl(th->th_ack);
//            th->th_ack = htonl(old_ack );
//        output(1).push(zwa);

            it->second.p->kill();
            it->second.p = zwa;
        }
    }
    // unlock the hash??
    tbl_lock.release();
    return p;
}



CLICK_ENDDECLS
EXPORT_ELEMENT(StreamManager)
