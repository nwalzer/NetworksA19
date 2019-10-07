#include <stdio.h>
#include "project3.h"
#include <string.h>
#include <sys/time.h>
#include <stdlib.h>

extern int TraceLevel;
extern float clocktime;

struct distance_table {
  int costs[MAX_NODES][MAX_NODES];
};
struct distance_table dt0;
struct NeighborCosts   *neighbor0;

/* students to write the following two routines, and maybe some others */

void printdt0( int MyNodeNumber, struct NeighborCosts *neighbor, struct distance_table *dtptr );

int neighbor0IDs[MAX_NODES];

void rtinit0() {
	printf("rtinit0 called\n");
	
	neighbor0 = getNeighborCosts(0);
	int totalNodes = neighbor0->NodesInNetwork;
	int i = 0;
	int j = 0;

	for(i = 0; i < MAX_NODES; i++){ //set everything in the costs array to infinite distance
		neighbor0IDs[i] = -1; //initialize neighbor IDs to invalid value
		for(j = 0; j < MAX_NODES; j++){
			dt0.costs[i][j] = INFINITY;
		}
	}

	j = 0;
	for(i = 0; i < totalNodes; i++){ //establish this node's distance to each other node (1 hop)
		if(neighbor0->NodeCosts[i] != INFINITY){ //if we have a direct connection to this node
			dt0.costs[i][i] = neighbor0->NodeCosts[i];
			neighbor0IDs[j] = i;
			j++;
		}
	}
	printdt0(0, neighbor0, &dt0);
	
	int tempArray[MAX_NODES];
	for(j = 0; j < MAX_NODES; j++){
		int k = 0;
		int lowest = INFINITY;
		for(k = 0; k < MAX_NODES; k++){
			if(dt0.costs[j][k] < lowest){
				lowest = dt0.costs[j][k];
			}
		}
		tempArray[j] = lowest;
	}

	struct RoutePacket toSend;
	toSend.sourceid = 0;
		
	memcpy(&toSend.mincost, &tempArray, sizeof(tempArray));
	
	i = 0;
	while(i < MAX_NODES && neighbor0IDs[i] != -1){
		if(neighbor0IDs[i] == 0){ //don't send to self
			i++;
			continue;
		}

		toSend.destid = neighbor0IDs[i];
		toLayer2(toSend);
				
		printf("At time t=%f node %d is sending a packet to %d with: ",clocktime, 0, toSend.destid);
		for(j = 0; j < MAX_NODES; j++){
			printf(" %d", toSend.mincost[j]);
		}
		printf("\n");

		i++;
	}
	if(TraceLevel == 2){
		for(i = 0; i < MAX_NODES; i++){
			for(j = 0; j < MAX_NODES; j++){
				printf("%d ", dt0.costs[i][j]);
			}
			printf("\n");
		}
	}
}


void rtupdate0( struct RoutePacket *rcvdpkt ) {
	if(TraceLevel == 2){
		printf("At time t=%f rtupdate0 was called from packet sent from %d with:", clocktime, rcvdpkt->sourceid);
	} else {
		printf("At time t=%f rtupdate0 was called\n", clocktime);
	}
	int i = 0;
	int j = 0;
	int sendUpdate = 0;
	int src = rcvdpkt->sourceid;

	for(i = 0; i < MAX_NODES; i++){
		if(TraceLevel == 2){
			printf(" %d", rcvdpkt->mincost[i]);
		}

		if(i == src){ //don't update with src dist to itself
			continue;
		} else if(rcvdpkt->mincost[i] == INFINITY){ //if node doesn't have a path, continue;
			continue;
		} else if(rcvdpkt->mincost[i] + dt0.costs[src][src] == dt0.costs[i][src]){ //if this will cause no change, continue;
			continue;
		} 
		dt0.costs[i][src] = rcvdpkt->mincost[i] + dt0.costs[src][src]; //distance to node = dist from this node to pkt src + src distance
		
		for(j = 0; j < MAX_NODES; j++){
			if(dt0.costs[i][src] < dt0.costs[i][j]){ //if any of the new values is now the shortest distance to that node
				sendUpdate = 1; //we need to update this node's shortest path
			}
		}
	}
	
	if(TraceLevel == 2){
		printf("\nUpdate? %d\n", sendUpdate);
	}
	printdt0(0, neighbor0, &dt0);
	
	if(sendUpdate){
		int tempArray[MAX_NODES];
		for(i = 0; i < MAX_NODES; i++){
			tempArray[i] = INFINITY; //initialize this to big number
			for(j = 0; j < MAX_NODES; j++){
				if(dt0.costs[i][j] < tempArray[i]){ //get shortest distance for dest i
					tempArray[i] = dt0.costs[i][j];
				}
			}
		}
		
		struct RoutePacket toSend;
		toSend.sourceid = 0;
		
		memcpy(&toSend.mincost, &tempArray, sizeof(tempArray));

		i = 0;
		while(i < MAX_NODES && neighbor0IDs[i] != -1){
			if(neighbor0IDs[i] == 0 || neighbor0IDs[i] == src){ //don't send to self or source
				i++;
				continue;
			}

			toSend.destid = neighbor0IDs[i];
			toLayer2(toSend);
			
			printf("At time t=%f node %d is sending a packet to %d with: ", clocktime, 0, toSend.destid);
			for(j = 0; j < MAX_NODES; j++){
				printf(" %d", toSend.mincost[j]);
			}
			printf("\n");

			i++;
		}
	}
}


/////////////////////////////////////////////////////////////////////
//  printdt
//  This routine is being supplied to you.  It is the same code in
//  each node and is tailored based on the input arguments.
//  Required arguments:
//  MyNodeNumber:  This routine assumes that you know your node
//                 number and supply it when making this call.
//  struct NeighborCosts *neighbor:  A pointer to the structure 
//                 that's supplied via a call to getNeighborCosts().
//                 It tells this print routine the configuration
//                 of nodes surrounding the node we're working on.
//  struct distance_table *dtptr: This is the running record of the
//                 current costs as seen by this node.  It is 
//                 constantly updated as the node gets new
//                 messages from other nodes.
/////////////////////////////////////////////////////////////////////
void printdt0( int MyNodeNumber, struct NeighborCosts *neighbor, 
		struct distance_table *dtptr ) {
    int       i, j;
    int       TotalNodes = neighbor->NodesInNetwork;     // Total nodes in network
    int       NumberOfNeighbors = 0;                     // How many neighbors
    int       Neighbors[MAX_NODES];                      // Who are the neighbors

    // Determine our neighbors 
    for ( i = 0; i < TotalNodes; i++ )  {
        if (( neighbor->NodeCosts[i] != INFINITY ) && i != MyNodeNumber )  {
            Neighbors[NumberOfNeighbors] = i;
            NumberOfNeighbors++;
        }
    }
    // Print the header
    printf("                via     \n");
    printf("   D%d |", MyNodeNumber );
    for ( i = 0; i < NumberOfNeighbors; i++ )
        printf("     %d", Neighbors[i]);
    printf("\n");
    printf("  ----|-------------------------------\n");

    // For each node, print the cost by travelling thru each of our neighbors
    for ( i = 0; i < TotalNodes; i++ )   {
        if ( i != MyNodeNumber )  {
            printf("dest %d|", i );
            for ( j = 0; j < NumberOfNeighbors; j++ )  {
                    printf( "  %4d", dtptr->costs[i][Neighbors[j]] );
            }
            printf("\n");
        }
    }
    printf("\n");
}    // End of printdt0

