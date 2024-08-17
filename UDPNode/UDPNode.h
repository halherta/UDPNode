// Copyright 2024 Hussam Al-Hertani. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the LICENSE file.
#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <string>
#include <iostream>
#include <atomic>
#include <thread>
#include <queue>
#include <mutex>

#include "../rapidjson/include/rapidjson/writer.h"
#include "../rapidjson/include/rapidjson/stringbuffer.h"
#include "../rapidjson/include/rapidjson/document.h"

// Enumeration for error codes used by UDPNode class.
enum err_code{
    SUCCESS = 0,
    SOCKET_CONN_FAILED = -1,
    BIND_FAILED = -2,
    RECVFROM_FAILED = -3,
    SENDTO_FAILED = -4,
    GETADDRINFO_FAILED = -5,
    PARSE_TIME_FAILED = -6,
    PARSE_MSG_FAILED = -7,
    PARSE_CRC_FAILED = -8
};

// Enumeration for IP family versions.
enum ipFamily{
    ipv4 = AF_INET,
    ipv6 = AF_INET6
};

// Structure representing a received datagram.
struct rxDatagram{
    unsigned int srcport;   // Source port number.
    std::string srcipaddr;  // Source IP address.
    time_t time_stamp;      // Timestamp of the received datagram.
    std::string msg;        // Message content.
    unsigned int crc_checksum;  // CRC checksum of the message.
    bool jointhread;        // Flag to indicate if the thread should join.
};

// Class that handles sending and receiving UDP datagrams.
class UDPNode{
    public:
       /**
        * @brief Constructs a UDPNode object and initializes the socket.
        * 
        * @param lport Local port number to bind the socket.
        * @param ver IP version to use (ipv4 or ipv6).
        * @param maxmsgsize Maximum size of messages to be received.
        * @param maxqsize Maximum size of the receive queue.
        * @param debug Flag to enable/disable debug mode.
        */
        UDPNode(int lport, ipFamily ver, unsigned int maxmsgsize = 1024, unsigned int maxqsize = 100, bool debug = false);
        
        /**
         * @brief Destructor that cleans up resources and closes sockets.
         */
        ~UDPNode(void);
        
        /**
         * @brief Transmits a message to the specified destination.
         * 
         * @param destport Destination port number.
         * @param ver IP version to use (ipv4 or ipv6).
         * @param host Destination IP address or hostname.
         * @param msg Message to be sent.
         * @param jointhread Flag to indicate if the thread should join.
         * @return err_code Error code indicating success or failure.
         */
        err_code tx(int destport, ipFamily ver, std::string host, std::string msg, bool jointhread = false);  
        
        /**
         * @brief Starts the receive loop in a separate thread.
         */
        void startRxLoop(void);
        
        /**
         * @brief Stops the receive loop and joins the thread.
         */
        void endRxLoop(void);
        
        /**
         * @brief Prints the contents of a received datagram.
         * 
         * @param datagram The datagram to be printed.
         */
        void printDatagram(const rxDatagram &datagram);
        
        /**
         * @brief Returns a descriptive error message for a given error code.
         * 
         * @param error_code The error code.
         * @return std::string A string describing the error.
         */
        std::string errorMsg(const err_code &error_code);
        
        /**
         * @brief Checks if there is data available in the receive queue.
         * 
         * @return bool True if data is available, false otherwise.
         */
        bool rxDataAvailable();
        
        /**
         * @brief Returns the current size of the receive queue.
         * 
         * @return int The size of the receive queue.
         */
        int rxDataQueueSize();
        
        /**
         * @brief Reads and removes a datagram from the receive queue.
         * 
         * @return rxDatagram The datagram read from the queue.
         */
        rxDatagram readRxDatagramFromQueue();
        

    private:
        /**
         * @brief Writes a datagram to the receive queue.
         * 
         * @param datagram The datagram to be written to the queue.
         */
        void writeRxDatagramToQueue(const rxDatagram &datagram);
        
        /**
         * @brief Retrieves the IP address from a sockaddr structure.
         * 
         * @param sa The sockaddr structure.
         * @return void* Pointer to the IP address.
         */
        void* getInAddr(struct sockaddr *sa);
        
        /**
         * @brief Retrieves the port number from a sockaddr structure.
         * 
         * @param sa The sockaddr structure.
         * @return uint16_t The port number.
         */
        uint16_t getInPort(struct sockaddr *sa);

        /**
         * @brief Serializes a message and additional data into a JSON string.
         * 
         * @param msg The message to be serialized.
         * @param jointhread Flag to indicate if the thread should join.
         * @return rapidjson::StringBuffer The serialized JSON string.
         */
        rapidjson::StringBuffer serialize(std::string msg, bool jointhread);
        
        /**
         * @brief Creates a socket and binds it to the specified port.
         * 
         * @return err_code Error code indicating success or failure.
         */ 
        err_code createSocketAndBind(void);
        
        /**
         * @brief The main receive loop that listens for incoming datagrams.
         */
        void rxLoop(void);

        /**
         * @brief Parses a received datagram and extracts its contents.
         * 
         * @param their_addr The sockaddr structure of the sender.
         * @param buf The received buffer.
         * @param numbytes The number of bytes received.
         * @param datagram The structure to store the parsed datagram.
         * @return err_code Error code indicating success or failure.
         */
        err_code parseDatagram(const sockaddr_storage &their_addr, char *buf, int numbytes, rxDatagram &datagram);
        
        /**
         * @brief Validates the CRC checksum of a received datagram.
         * 
         * @param datagram The datagram to be validated.
         * @return bool True if the checksum is valid, false otherwise.
         */
        bool isDatagramValid(const rxDatagram &datagram);

        /**
         * @brief Inspects and prints the contents of the receive buffer.
         * 
         * @param their_addr The sockaddr structure of the sender.
         * @param buf The received buffer.
         * @param numbytes The number of bytes received.
         */
        void inspectRxBuffer(struct sockaddr_storage their_addr, char * buf,  int numbytes);
        
        // Mutex to protect access to the receive queue.
        std::mutex _mtx;

        // Maximum size of the receive queue.
        unsigned int _maxqueuesize;

        // Maximum size of a message that can be received.
        unsigned int _maxmessagesize;

        // Queue to store received datagrams.
        std::queue<rxDatagram> _rxqueue;

        // Atomic flag to control the receive loop.
        std::atomic<bool> _stoprecvthread; 
        
        // Thread for the receive loop.
        std::thread _rxthread;

        // String to store the current message being processed.
        std::string _message;

        // IP family version for listening (ipv4 or ipv6).
        ipFamily _listenipver;

        // Port number to bind the socket.
        int _listenport; 

        // File descriptor for the listening & sending sockets.
        int _listensockfd, _sendsockfd;

         // Flag to enable/disable debug mode.
        bool _debug;
};