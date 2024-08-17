# UDPNode C++ Class

## Overview

`UDPNode` is a C++ class designed to facilitate UDP-based communication over the network. It supports both IPv4 and IPv6, allowing for the transmission and reception of messages in a multithreaded environment. This class is particularly useful for applications requiring fast, connectionless communication.

## Class Features

- **Initialize and Configure UDP Sockets**: The class can initialize UDP sockets for both sending and receiving messages.
- **Multithreaded Receive Loop**: It provides a mechanism to start and stop a receive loop in a separate thread.
- **Queue Management**: Received messages are stored in a thread-safe queue, allowing asynchronous processing of incoming data.
- **Data Serialization**: Messages are serialized using JSON, and each message is associated with a CRC checksum for validation.
- **Error Handling**: Provides detailed error handling and reporting through an enumerated type `err_code`.

## Downloading UDPNode

The UDPNode class relies on the RapidJson library and requires the inclusion of a few of that libraries header files. To download UDPNode with all of its dependencies:

``` bash
git clone --recurse-submodules https://github.com/halherta/UDPNode.git 
```

## Class Functions

### Constructor

```cpp
UDPNode(int listen_port, ipFamily listen_ip_version, int max_message_size, int max_queue_size);
```
- Parameters:
    - listen_port: Port number to listen for incoming messages.
    - listen_ip_version: IP version (IPv4 or IPv6) to use for the listening socket.
    - max_message_size: Maximum size of the incoming message buffer.
    - max_queue_size: Maximum number of messages to store in the queue.

### Destructor

```cpp
~UDPNode(void);
```
- Cleans up resources, closes sockets, and stops the receive thread if running.

### Transmission
```cpp
err_code tx(int dest_port, ipFamily ip_version, std::string host, std::string msg, bool join_thread);

```

- Parameters:
    - dest_port: Destination port to send the message to.
    - ip_version: IP version (IPv4 or IPv6) to use for sending.
    - host: Hostname or IP address of the destination.
    - msg: Message to be sent.
    - join_thread: Boolean indicating if the receiver should join the thread.

- Returns: Error code indicating success or the type of failure.

### Receive Loop Management

```cpp
void startRxLoop(void);
void endRxLoop(void);
```
- `startRxLoop()`: Starts the receive loop in a new thread.
- `endRxLoop()`: Stops the receive loop and joins the thread.

### Datagram Handling

```cpp
err_code parseDatagram(const sockaddr_storage &their_addr, char *buf, int numbytes, rxDatagram &datagram);
bool isDatagramValid(const rxDatagram &datagram);
void printDatagram(const rxDatagram &datagram);
```
- `parseDatagram()`: Parses the received buffer into a rxDatagram structure.
- `isDatagramValid()`: Checks if the received datagram's CRC matches the calculated value.
- `printDatagram()`: Outputs the contents of a rxDatagram structure.

### Queue Management

```cpp
void writeRxDatagramToQueue(const rxDatagram &datagram);
bool rxDataAvailable(void);
int rxDataQueueSize(void);
rxDatagram readRxDatagramFromQueue(void);
```
- `writeRxDatagramToQueue()`: Adds a received datagram to the queue.
- `rxDataAvailable()`: Checks if there is data available in the queue.
- `rxDataQueueSize()`: Returns the number of datagrams in the queue.
- `readRxDatagramFromQueue()`: Retrieves and removes the front datagram from the queue.

### Utility Functions

```cpp
std::string errorMsg(const err_code &error_code);
void* getInAddr(struct sockaddr *sa);
uint16_t getInPort(struct sockaddr *sa);
rapidjson::StringBuffer serialize(std::string msg, bool join_thread);
void inspectRxBuffer(struct sockaddr_storage their_addr, char *buf, int numbytes);
```
- `errorMsg()`: Converts an error code to a human-readable message.
- `getInAddr()`: Extracts the IP address from a sockaddr structure.
- `getInPort()`: Extracts the port number from a sockaddr structure.
- `serialize()`: Serializes a message into a JSON object with a CRC checksum.
- `inspectRxBuffer()`: Outputs the raw contents of the received buffer.

### Usage Example

#### UDP Receiver

```cpp

#include <iostream>
#include "UDPNode.h"


int main(void){

    UDPNode node(3490, ipv6,1024,5,true);
    node.startRxLoop();
    usleep(10000000);
    if(node.rxDataAvailable()){
        unsigned int datagramsavailable = node.rxDataQueueSize();
        for( int i = 0 ; i < datagramsavailable ; i++){
            node.printDatagram(node.readRxDatagramFromQueue());
            usleep(100000);
        }
    }
    node.endRxLoop();
    return 0;
}

```

#### UDP Transmitter

```cpp
#include <iostream>
#include <unistd.h>
#include "UDPNode.h"


int main(void){
    UDPNode node(5590, ipv6, 1024, 5,true);
    
    node.tx(3490, ipv6,"::1","Attitude is the \"little thing\" that makes a big difference");
    node.tx(3490, ipv6,"::1","Life is too short to spend another day at war with yourself");
    node.tx(3490, ipv6,"::1","The best time to plant a tree was 20 years ago. The second best time is now");
    node.tx(3490, ipv6,"::1","Be happy for this moment. This moment is your life");
    node.tx(3490, ipv6,"::1","A good laugh and a long sleep are the two best cures for anythin");
    node.tx(3490, ipv6,"::1","The sun is a daily reminder that we too can rise again from the darkness, that we too can shine our own light");
    node.tx(3490, ipv6,"::1","Today is a good day to try");
    node.tx(3490, ipv6,"::1","The only person you are destined to become is the person you decide to be");
    
    return 0;
}
```

### Compilation Instructions

Compiling from the command line:

```bash
g++ -std=c++11 -pthread -o udpnode main.cpp UDPNode.cpp
```

You can also have a look at the examples to see an example CMakeLists.txt for cmake compilation

### Dependencies

- RapidJSON: A fast JSON parser/generator for C++ with both SAX/DOM style API.
- POSIX Threads (pthreads): For multithreading support.

