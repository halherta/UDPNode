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
