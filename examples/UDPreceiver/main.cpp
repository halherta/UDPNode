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