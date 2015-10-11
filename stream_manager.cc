#include <click/config.h>
#include "stream_manager.hh"

CLICK_DECLS

StreamManager::StreamManager():frozen(false) {}
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
       // p = add_stream(p);
        output(0).push(p);
        break;

    // reply to ZWPs or any other packets if we're FROZEN
    case 1:
        //p = update_stream(p);
        output(0).push(p);
        break;

    case 2:
        //p = remove_stream(p);
        output(0).push(p);
        break;

    case 3:
        output(1).push(p);
        break;

    // remove stream
    case 4:
       // p = update_ack(p);
        output(2).push(p);
        break;

    case 5:
        //p = hp_add(p);
        output(0).push(p);
        break;

    case 6:
        //p = hp_remove(p);
        output(0).push(p);
        break;

    // not one of our ports, so do nothing, and pass the packet on.
    default:
        break;
    };

    return p;
}


Packet* StreamManager::hp_add(Packet* p)
{
    IPFlowID id(p, false);

    // assign the stream to the hashtable
    hp_lock.acquire();
    hp_hash.set(id, 1);
    frozen = true;
    hp_lock.release();

    return p;
}

Packet* StreamManager::hp_remove(Packet* p)
{
    IPFlowID id(p, false);

    // move the stream from the hashtable
    hp_lock.acquire();
    if (hp_hash.erase(id) == 0)
        hp_hash.erase(id.reverse());
    if(hp_hash.empty())
        frozen = false;
//    tbl_lock.acquire();

  //  tbl_lock.release();
    hp_lock.release();

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
    IPFlowID id(p, false);

    // lock the hash
    tbl_lock.acquire();

    HashTable_iterator<Pair<const IPFlowID, stream_data> > it = hash.find(id);
    if (it != hash.end() && it->second.frozen && frozen)
    {
        if(it->second.update_zwa)
            it->second.send_zero_wnd(p);
        else
            it->second.send_zero_wnd();
    }
    tbl_lock.release();

    return p;
}

Packet* StreamManager::update_ack(Packet* p)
{
    const click_tcp* tcph = p->tcp_header();

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

/*        if (it->second.frozen)
        if(frozen)
        {
            //output(1).push(p->clone());
            //output(1).push(p->clone());
            //output(2).push(p->clone());
            it->second.frozen = false;
        }
*/


//        it->second.send_zero_wnd();
        it->second.reset_timers();

        if(tcph->th_flags & (TH_RST | TH_FIN))
        {
            remove_stream(p);     //remove the stream if we see FIN or RST
            return p;
        }
        else
        {
            WritablePacket* zwa = p->clone()->uniqueify();
            click_tcp* th = zwa->tcp_header();
            th->th_win = 0;
            it->second.last_window = th->th_win;

            it->second.p->kill();
            it->second.p = zwa;
            tbl_lock.release();
            p->kill();
            return zwa->clone();
        }
    }
    // unlock the hash??
    tbl_lock.release();
    return p;
}


void StreamManager::unfreeze()
{
    tbl_lock.acquire();
    for(HashTable_iterator<Pair<const IPFlowID, stream_data> > it = hash.begin(); it != hash.end(); ++it)
    {
        it->second.unfreeze();
    }
    tbl_lock.release();
}

CLICK_ENDDECLS
EXPORT_ELEMENT(StreamManager)
