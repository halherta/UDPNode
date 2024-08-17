// Copyright 2024 Hussam Al-Hertani. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the LICENSE file.

#include "UDPNode.h"

UDPNode::UDPNode(int lport, ipFamily ver, unsigned int maxmsgsize, unsigned int maxqsize, bool debug):_listenport(lport), _listenipver(ver), _maxqueuesize(maxqsize), _maxmessagesize(maxmsgsize), _debug(debug){
   // Initialize the atomic flag to false.
    _stoprecvthread = false;
    auto rv = createSocketAndBind();
    if (rv != SUCCESS) {
       std::cerr << errorMsg(rv) << std::endl;
       exit(1);
    }
}

err_code UDPNode::createSocketAndBind(void){

    struct addrinfo hints;  // Hints for getaddrinfo.
    int rv;                 // Return value for getaddrinfo.
    struct addrinfo *rxservinfo;  // Linked list of results from getaddrinfo.
    struct addrinfo  *rx_p;       // Pointer to iterate over results.
    err_code error_code = SUCCESS; // Initialize error code to success.
    
     // Zero out the hints structure.
    memset(&hints, 0, sizeof hints);
    hints.ai_family =  _listenipver == ipv4 ? AF_INET: AF_INET6; // set to AF_INET to use IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

       // Get address info for the local machine.
    if ((rv = getaddrinfo(NULL, std::to_string(_listenport).c_str(), &hints, &rxservinfo)) != 0) {
        std::cerr << "getaddrinfo: " << gai_strerror(rv) << std::endl;
        error_code = GETADDRINFO_FAILED;
    }

    // loop through all the results and bind to the first we can
    for(rx_p = rxservinfo; rx_p != NULL; rx_p = rx_p->ai_next) {
        if ((_listensockfd = socket(rx_p->ai_family, rx_p->ai_socktype,rx_p->ai_protocol)) == -1) {
            error_code = SOCKET_CONN_FAILED;
            continue;
        }

        if (bind(_listensockfd, rx_p->ai_addr, rx_p->ai_addrlen) == -1) {
            close(_listensockfd);
            _listensockfd = -1;
            //std::cerr <<"Rx: Failed to bind socket" << std::endl;
            error_code = BIND_FAILED ;
            continue;
        }
        
        break;
    }

    // If no socket was bound, return the last error encountered.
    if (rx_p == NULL) {
        return error_code;
    }

    std::cout << "listening on port: "<< _listenport << "..."<<std::endl;
    
    // Free the linked list.
    freeaddrinfo(rxservinfo);
    rxservinfo = nullptr;
    
    rx_p = nullptr;
    return error_code;
}

UDPNode::~UDPNode(void){

    endRxLoop(); // Ensure the receive loop is stopped
    if(_rxthread.joinable()){
        _rxthread.join();
    }
    std::cout << "closing listening socket and exiting..." << std::endl;
    
    // Close the listening and sending sockets.
    if (_listensockfd != -1){
        close(_listensockfd);
        _listensockfd = -1;
    }
    if (_sendsockfd != -1){
        close(_sendsockfd);
        _sendsockfd = -1;
    }
}

void UDPNode::startRxLoop(void){
     // Start the receive loop in a separate thread.
    _stoprecvthread = false;
    _rxthread = std::thread(&UDPNode::rxLoop, this);
}

void UDPNode::endRxLoop(void){
    // Set the atomic flag to true to stop the receive loop.
    _stoprecvthread = true;
    //send a datagram to the listening thread to unblock recvfrom
    tx(_listenport, _listenipver, "localhost","Goodbye",true);   
    
    // If the receive thread is joinable, join it.
    if(_rxthread.joinable()){
        _rxthread.join();
    }

    if (_listensockfd != -1){
        close(_listensockfd);
        _listensockfd = -1;
    }
}

void UDPNode::rxLoop(void){
    int numbytes;       // Number of bytes received.
    err_code error_code = SUCCESS;
    struct sockaddr_storage their_addr; // Address of the sender.
    std::unique_ptr<char[]> buf(new char[_maxmessagesize]);
    socklen_t addr_len;
    addr_len = sizeof their_addr;
    while(!_stoprecvthread){
        
        if(_debug){
            std::cout << "rxloop: In loop" << std::endl;
        }

        memset(buf.get(),0,_maxmessagesize);
        numbytes = recvfrom(_listensockfd, buf.get(), _maxmessagesize-1 , 0,(struct sockaddr *)&their_addr, &addr_len);
        if (numbytes == -1) {
            error_code = RECVFROM_FAILED;
            break;
        } else if ( numbytes > 0){   

            buf[numbytes] = '\0';
            if(_debug){
                inspectRxBuffer(their_addr, buf.get(),  numbytes); 
            }

            // Create a new datagram structure to store the received data.
            rxDatagram datagram;
            error_code = parseDatagram(their_addr, buf.get(), numbytes, datagram);
            if(error_code != SUCCESS){
                std::cerr << errorMsg(error_code) << std::endl;
                break;
            }
            
            // Validate the CRC checksum.
            if(!isDatagramValid(datagram)){
                std::cerr << "rxloop: CRC Checksum invalid. Discarding... " << std::endl;
            }

            if( rxDataQueueSize() >=  _maxqueuesize){
                std::cerr << "rxloop: Datagram Receive queue is full. Discarding incoming datagrams..." << std::endl;
            }

            /*

            if(datagram.jointhread){
                std::cerr << "Need to exit thread, breaking while " << std::endl;
                break;
            } 
            
            */
           
           // Write the datagram to the receive queue.
            if(datagram.jointhread == false && _rxqueue.size() < _maxqueuesize && isDatagramValid(datagram) ){
                writeRxDatagramToQueue(datagram);
            }
        }
    }
    if(_debug){
        std::cout << "rxloop: Exiting recv thread..." << std::endl;
    }

    if(error_code != SUCCESS){
        std::cerr << errorMsg(error_code) << std::endl;
    }
    
}

void UDPNode::writeRxDatagramToQueue(const rxDatagram &datagram){
    std::lock_guard<std::mutex> lock(_mtx);
    _rxqueue.push(datagram);
}

// get sockaddr, IPv4 or IPv6:
void * UDPNode::getInAddr(struct sockaddr *sa){
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

uint16_t UDPNode::getInPort(struct sockaddr *sa){
    if (sa->sa_family == AF_INET) {
        return (((struct sockaddr_in*)sa)->sin_port);
    }
    
    return (((struct sockaddr_in6*)sa)->sin6_port);
}

bool  UDPNode::rxDataAvailable(){
    std::lock_guard<std::mutex> lock(_mtx);
    return !_rxqueue.empty();
}

int  UDPNode::rxDataQueueSize(){
    std::lock_guard<std::mutex> lock(_mtx);
    return _rxqueue.size();
}

rxDatagram  UDPNode::readRxDatagramFromQueue(){
    std::lock_guard<std::mutex> lock(_mtx);
    rxDatagram retval = _rxqueue.front();
    _rxqueue.pop();
    return retval;
}

err_code UDPNode::tx(int destport, ipFamily ver, std::string host, std::string msg, bool jointhread){
    int numbytes;
    struct addrinfo hints;
    int rv;
    err_code error_code = SUCCESS;
    struct addrinfo *txservinfo, *tx_p;
    memset(&hints, 0, sizeof hints);
	hints.ai_family =  ver == ipv4 ? AF_INET : AF_INET6; // set to AF_INET to use IPv4
	hints.ai_socktype = SOCK_DGRAM;

	if ((rv = getaddrinfo(host.c_str(), std::to_string(destport).c_str(), &hints, &txservinfo)) != 0) {
		std::cerr << "tx: getaddrinfo: " << gai_strerror(rv) << std::endl;
		error_code = GETADDRINFO_FAILED;
	}

	// loop through all the results and make a socket
	for(tx_p = txservinfo; tx_p != NULL; tx_p = tx_p->ai_next) {
		if ((_sendsockfd = socket(tx_p->ai_family, tx_p->ai_socktype,tx_p->ai_protocol)) == -1) {
			continue;
		}

		break;
	}

	if (tx_p == NULL) {
		std::cerr << "tx: failed to create socket" << std::endl;
        error_code = SOCKET_CONN_FAILED;
 	}
    
    rapidjson::StringBuffer s = serialize(msg, jointhread);
   
	
    if ((numbytes = sendto(_sendsockfd, s.GetString(), strlen(s.GetString()), 0, tx_p->ai_addr, tx_p->ai_addrlen)) == -1) {
		error_code = SENDTO_FAILED;
	}


	freeaddrinfo(txservinfo);
    txservinfo = nullptr;
    tx_p = nullptr;
    if(_debug){
	    std::cout << "tx: sent "<< numbytes <<" bytes to " << host << ":" << destport << std::endl;
    }

    if(_sendsockfd != -1){
        close(_sendsockfd);
        _sendsockfd = -1;
    }
    
    return error_code;
}


void UDPNode::printDatagram(const rxDatagram &datagram){
            std::cout << "Source Port: " << datagram.srcport << std::endl;
            std::cout << "Source IP Address: " << datagram.srcipaddr << std::endl;
            std::cout << "Time Stamp: " << datagram.time_stamp << std::endl;
            std::cout << "Message: " << datagram.msg << std::endl;
            std::cout << "CRC Checksum: " << datagram.crc_checksum << std::endl;
            std::cout << "Join Thread: " <<  (datagram.jointhread ? "true" : "false") << std::endl;
            std::cout <<  (isDatagramValid(datagram) ? "CRC checksum valid": "CRC checksum invalid") << std::endl;
            std::cout << std::endl;
}


err_code UDPNode::parseDatagram(const sockaddr_storage &their_addr, char *buf, int numbytes, rxDatagram &datagram){
            err_code error_code = SUCCESS;
            rapidjson::Document d;
            char s[32]; // should 16 bytes + 1
            
            if(_debug){
                std::cout << "Parsing datagram..." << std::endl;
            }
            
            d.Parse(buf);
            
            datagram.srcport = getInPort((struct sockaddr *)&their_addr); 
            datagram.srcipaddr = std::string(inet_ntop(their_addr.ss_family, getInAddr((struct sockaddr *)&their_addr),s, sizeof s));
            
            if(d.HasMember("Time")){
                datagram.time_stamp = static_cast<time_t>(d["Time"].GetUint64());
            } else {
                error_code = PARSE_TIME_FAILED;
            }
            
            if(d.HasMember("Msg")){
                datagram.msg = std::string(d["Msg"].GetString());
            }else{
                error_code = PARSE_MSG_FAILED;
            }
            
            if(d.HasMember("CRC")){
                datagram.crc_checksum = static_cast<unsigned int>(d["CRC"].GetUint());
            }else{
               error_code = PARSE_CRC_FAILED; 
            }

            if(d.HasMember("Join_thr")){
                datagram.jointhread = d["Join_thr"].GetBool() ;
            }else {
                datagram.jointhread = false ;
            }
            return error_code;
}

bool UDPNode::isDatagramValid(const rxDatagram &datagram){
    unsigned int crc_check_act = 0; 
    for(int i = 0 ; i < datagram.msg.size(); i++){
        crc_check_act ^= datagram.msg[i];
    }
    return crc_check_act == datagram.crc_checksum;
}


std::string UDPNode::errorMsg(const err_code &error_code){
    std::string error_message;
    
    switch(error_code){
        case SUCCESS:
            error_message = "Success!";
            break;
        case SOCKET_CONN_FAILED:
            error_message = "Socket creation failed";
            break;        
        case BIND_FAILED:
            error_message = "Bind failed";
            break;         
        case RECVFROM_FAILED:
            error_message = "Recvfrom function failed";
            break;
        case SENDTO_FAILED:
            error_message = "Sendto function failed";
            break;
        case GETADDRINFO_FAILED:
            error_message = "Getaddrinfo function failed";
            break; 
        case PARSE_TIME_FAILED:
            error_message = "Parsing time from buffer (to JSON) failed";
            break;
        case PARSE_MSG_FAILED:
            error_message = "Parsing message from buffer (to JSON) failed";
            break;
        case PARSE_CRC_FAILED:
            error_message = "Parsing CRC from buffer (to JSON) failed";
            break;      
        default:
            error_message = "Invalid error code";
            break;    
    }
    return error_message;
}


rapidjson::StringBuffer UDPNode::serialize(std::string msg, bool jointhread){
     // serialize
    rapidjson::StringBuffer s;
    rapidjson::Writer<rapidjson::StringBuffer> writer(s);
    writer.StartObject();               // Between StartObject()/EndObject(), 
    writer.Key("Time");                // output a key,
    writer.Uint64(static_cast<unsigned long>(time(0)));
    writer.Key("Msg");
    writer.String(msg.c_str());

    unsigned char crc_checksum = 0;
    for(int i = 0 ; i < strlen(msg.c_str()) ; i++)
        crc_checksum ^= msg.c_str()[i];
    
    writer.Key("CRC");
    writer.Uint(static_cast<unsigned int>(crc_checksum));
    
    if(jointhread){
       writer.Key("Join_thr");
       writer.Bool(true); 
    }

    writer.EndObject();
    return s;
}

void UDPNode::inspectRxBuffer(struct sockaddr_storage their_addr, char * buf,  int numbytes){
    char s[32]; 
    std::cout << "Got datagram from: " << inet_ntop(their_addr.ss_family, getInAddr((struct sockaddr *)&their_addr),s, sizeof s)<< std::endl;
    std::cout << "Datagram is " << numbytes << " bytes long"  << std::endl;
    std::cout << "Datagram contents: " << buf << std::endl;
    std::cout << std::endl;
}