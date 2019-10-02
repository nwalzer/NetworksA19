#include <stdio.h>
#include "project3.h"
#include <string.h>

extern int TraceLevel;

struct distance_table {
  int costs[MAX_NODES][MAX_NODES];
};
struct distance_table dt1;
struct NeighborCosts   *neighbor1;

void printdt1( int MyNodeNumber, struct NeighborCosts *neighbor, struct distance_table *dtptr );

int neighbor1IDs[MAX_NODES];

void rtinit1() {
	printf("rtinit1 called\n");
	neighbor1 = getNeighborCosts(1);
	int totalNodes = neighbor1->NodesInNetwork;
	int i = 0;
	int j = 0;

	for(i = 0; i < MAX_NODES; i++){ //set everything in the costs array to infinite distance
		neighbor1IDs[i] = -1; //initialize neighbor IDs to invalid value
		for(j = 0; j < MAX_NODES; j++){
			dt1.costs[i][j] = INFINITY;
		}
	}

	j = 0;
	for(i = 0; i < totalNodes; i++){ //establish this node's distance to each other node (1 hop)
		if(neighbor1->NodeCosts[i] != INFINITY){ //if we have a direct connection to this node
			dt1.costs[i][i] = neighbor1->NodeCosts[i];
			neighbor1IDs[j] = i;
			j++;
		}
	}
	printdt1(1, neighbor1, &dt1);
	
	int tempArray[MAX_NODES];
	for(j = 0; j < MAX_NODES; j++){
		int k = 0;
		int lowest = INFINITY;
		for(k = 0; k < MAX_NODES; k++){
			if(dt1.costs[j][k] < lowest){
				lowest = dt1.costs[j][k];
			}
		}
		tempArray[j] = lowest;
		printf("%d: %d ", j, lowest);
	}
	printf("\n");

	struct RoutePacket toSend;
	toSend.sourceid = 1;
		
	memcpy(&toSend.mincost, &tempArray, sizeof(tempArray));
	
	i = 0;
	while(i < MAX_NODES && neighbor1IDs[i] != -1){
		if(neighbor1IDs[i] == 1){ //don't send to self
			i++;
			continue;
		}

		toSend.destid = neighbor1IDs[i];
		toLayer2(toSend);

		printf("Node %d is sending a packet to %d with: ", 1, toSend.destid);
		for(j = 0; j < MAX_NODES; j++){
			printf(" %d", toSend.mincost[j]);
		}
		printf("\n");

		i++;
	}
	for(i = 0; i < MAX_NODES; i++){
		for(j = 0; j < MAX_NODES; j++){
			printf("%d ", dt1.costs[i][j]);
		}
		printf("\n");
	}
}
/*arr[i][j]
  i = 0: j=0, 1, 2
  i = 1: j=0, 1, 2
*/
void rtupdate1( struct RoutePacket *rcvdpkt ) {
	int i = 0;
	int sendUpdate;
	int src = rcvdpkt->sourceid;

	for(i = 0; i < MAX_NODES; i++){
		if(rcvdpkt->mincost[i] < dt1.costs[i][src]){
			dt1.costs[i][src] = rcvdpkt->mincost[i];
		}
	}
	printdt1(1, neighbor1, &dt1);
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
void printdt1( int MyNodeNumber, struct NeighborCosts *neighbor, 
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
}    // End of printdt1

