// DO-NOT-REMOVE begin-copyright-block 
//                                     
// Redistributions of any form whatsoever must retain and/or include the     
// following acknowledgment, notices and disclaimer:                         
//                                                                           
// This product includes software developed by Carnegie Mellon University.   
//                                                                           
// Copyright 2006 - 2008 by Eric Chung, Michael Ferdman, Brian Gold, Nikos   
// Hardavellas, Jangwoo Kim, Ippokratis Pandis, Minglong Shao, Jared Smolens,
// Stephen Somogyi, Evangelos Vlachos, Tom Wenisch, Anastassia Ailamaki,     
// Babak Falsafi and James C. Hoe for the SimFlex Project, Computer          
// Architecture Lab at Carnegie Mellon, Carnegie Mellon University.          
//                                                                           
// For more information, see the SimFlex project website at:                 
//   http://www.ece.cmu.edu/~simflex                                         
//                                                                           
// You may not use the name 'Carnegie Mellon University' or derivations      
// thereof to endorse or promote products derived from this software.        
//                                                                           
// If you modify the software you must place a notice on or within any       
// modified version provided or made available to any third party stating    
// that you have modified the software.  The notice shall include at least   
// your name, address, phone number, email address and the date and purpose  
// of the modification.                                                      
//                                                                           
// THE SOFTWARE IS PROVIDED 'AS-IS' WITHOUT ANY WARRANTY OF ANY KIND, EITHER 
// EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO ANY WARRANTY  
// THAT THE SOFTWARE WILL CONFORM TO SPECIFICATIONS OR BE ERROR-FREE AND ANY 
// IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,  
// TITLE, OR NON-INFRINGEMENT.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY 
// BE LIABLE FOR ANY DAMAGES, INCLUDING BUT NOT LIMITED TO DIRECT, INDIRECT, 
// SPECIAL OR CONSEQUENTIAL DAMAGES, ARISING OUT OF, RESULTING FROM, OR IN   
// ANY WAY CONNECTED WITH THIS SOFTWARE (WHETHER OR NOT BASED UPON WARRANTY, 
// CONTRACT, TORT OR OTHERWISE).                                             
//                                     
// DO-NOT-REMOVE end-copyright-block   
#include <iostream>
#include <fstream>
#include <string.h>
#include <stdlib.h>
#include <assert.h> 
#include <math.h>

using namespace std;

#define DIMS  2

int numNodes = 0;
int nodesPerDim[DIMS] = { 0 };

bool isTorus = false;
bool isAdaptive = false;

char * outFilename;
ofstream outFile;

bool processArguments ( int argc, char ** argv );
bool generateSwitches ( void );

bool generateMeshTopology ( int nodeId );
bool generateTorusTopology ( int nodeId );

bool generateDeadlockFreeMeshRoute ( int currNode,
                                     int destNode );

bool generateDeadlockFreeTorusRoute ( int currNode,
                                      int destNode );

enum Direction 
  {
    NORTH,
    SOUTH, 
    EAST,
    WEST,
    LOCAL
  };

int main ( int argc, char ** argv )
{

  int
    i,
    j;

  if ( argc != 5 ) {
    cerr << "Usage: " << argv[0] << " numnodes -t|-m -d|-a outputfile" << endl;
    cerr << "   -t specifies a 2D torus" << endl;
    cerr << "   -m specifies a 2D mesh" << endl;
    cerr << "   -d specifies dimension ordered routing" << endl;
    cerr << "   -a specifies adaptive routing tables (not implemented)" << endl;

    return 1;
  }

  if ( processArguments ( argc, argv ) ) 
    return 1;


  // Generate the boilerplate parameters
  outFile << "# Boilerplate stuff" << endl;
  outFile << "ChannelLatency 100" << endl;
  outFile << "ChannelLatencyData 32" << endl;
  outFile << "ChannelLatencyControl 1" << endl;
  outFile << "LocalChannelLatencyDivider 8" << endl;
  outFile << "SwitchInputBuffers 5" << endl;
  outFile << "SwitchOutputBuffers 1" << endl;
  outFile << "SwitchInternalBuffersPerVC 1" << endl;
  outFile << endl;

  // Output all of the node->switch connections
  if ( generateSwitches() )
    return true;

  // Make topology   
  if ( isTorus ) {
    outFile << endl << "# Topology for a " << numNodes << " node TORUS" << endl;
  } else {
    outFile << endl << "# Topology for a " << numNodes << " node MESH" << endl;
  }

  for ( i = 0; i < numNodes; i++ ) { 
    if ( isTorus ) {
        if ( generateTorusTopology ( i ) )
          return 1;
    } else {
      if ( generateMeshTopology ( i ) ) 
        return 1;
    }        
  }

  outFile << endl << "# Deadlock-free routing tables" << endl;

  // For each switch, generate a routing table to each destination
  for ( i = 0; i < numNodes; i++ ) {

    outFile << endl << "# Switch " << i << " -> *" << endl;
    
    // For each destination
    for ( j = 0; j < numNodes; j++ ) {
      if ( isTorus ) {
        if ( generateDeadlockFreeTorusRoute ( i, j ) ) 
          return 1;
      } else {
        if ( generateDeadlockFreeMeshRoute ( i, j ) )
          return 1;
      }
    }

  }

  outFile.close();

  return 0;
}


bool processArguments ( int argc, char ** argv )
{
  int
    i;
  
  numNodes = atoi ( argv[1] );
  
  nodesPerDim[0] = (int)sqrt ( (float)numNodes );

  // Get this to a power of two
  while ( nodesPerDim[0] & (nodesPerDim[0]-1) )
    nodesPerDim[0]--;

  nodesPerDim[1] = numNodes / nodesPerDim[0];

  if ( numNodes <= 0 || ( numNodes & ( numNodes - 1 ) ) ) {
    cerr << "NumNodes must be greater than zero and a power of two" << endl;
    return true;
  }

  for ( i = 0; i < DIMS; i++ ) {
    cout << nodesPerDim[i] << " nodes per dimension " << i << endl;
  }
    
  if ( !strcasecmp ( argv[2], "-t" ) ) {
    isTorus = true;
  } else if ( !strcasecmp ( argv[2], "-m" ) ) {
    isTorus = false;
  } else {
    cerr << "Unexpected argument: " << argv[2] << endl;
    return true;
  }


  if ( !strcasecmp ( argv[3], "-d" ) ) {
    isAdaptive = false;
  } else if ( !strcasecmp ( argv[3], "-a" ) ) {
    isAdaptive = true;
    cerr << "ERROR: adaptive routing not supported yet" << endl;
    return true;
  } else {
    cerr << "Unexpected argument: " << argv[3] << endl;
    return true;
  }

  outFilename = argv[4];

  outFile.open ( outFilename );
  if ( !outFile.good() ) { 
    cerr << "ERROR opening output file: " << outFilename << endl;
    return true;
  }
    
  return false;
}

bool generateSwitches ( void ) 
{
  int
    i;
  
  outFile << "# Basic Switch/Node connections" << endl;
  outFile << "NumNodes " << numNodes << endl;
  outFile << "NumSwitches " << numNodes << endl;
  outFile << "SwitchPorts   5" << endl;
  outFile << "SwitchBandwidth 4" << endl;
  outFile << endl;

  for ( i = 0; i < numNodes; i++ ) { 
    outFile << "Top Node " << i << " -> Switch " << i << ":0" << endl;
  }
  
  return false;
}

int getXCoord ( int nodeId ) 
{
  return ( nodeId % nodesPerDim[0] );
}

int getYCoord ( int nodeId ) 
{
  return ( nodeId / nodesPerDim[0] );
}

int getNodeIdCoord ( int x, int y )
{
  int
    id;

  id = ( x + ( y * nodesPerDim[0] ) );
  
  if ( id >= numNodes ) {
    cerr << "ERROR: node coordinates out of bounds: " << x << ", " << y << endl;
    exit ( 1 );
  }

  return id;
}

int getNodeIdOffset ( int nodeId, Direction dir ) 
{
  int
    x,
    y;
  
  x = getXCoord ( nodeId );
  y = getYCoord ( nodeId );
  
  switch ( dir ) {
  case NORTH:   y--; break;
  case SOUTH:   y++; break;
  case EAST:    x++; break;
  case WEST:    x--; break;

  default:
    cerr << "Invalid direction" << dir << endl;
    exit ( 1 );
  }

  if ( isTorus ) {
    
    if ( x < 0 ) 
      x = nodesPerDim[0]-1;

    if ( x >= nodesPerDim[0] )
      x = 0;
    
    if ( y < 0 ) 
      y = nodesPerDim[1]-1;
    
    if ( y >= nodesPerDim[1] )
      y = 0;

  } else {

    // Invalid offset for a mesh
    if ( x < 0 || y < 0 || 
         x >= nodesPerDim[0] || 
         y >= nodesPerDim[1] ) {
      
      cerr << "ERROR: invalid offset for a mesh!" << endl;
      return -1;
    }
  }  

  return getNodeIdCoord ( x, y );
}

ostream & operator <<  ( ostream & out, const Direction dir )
{

  switch ( dir ) {
  case NORTH:  out << "2"; break;
  case SOUTH:  out << "4"; break;
  case EAST:   out << "3"; break;
  case WEST:   out << "1"; break;
  case LOCAL:  out << "0"; break;
  };

  return out;
}

bool writeS2S ( const int       node1,
                const Direction dir1,
                const int       node2,
                const Direction dir2 )
{

  outFile << "Top Switch " 
          << node1 << ":" << dir1 << " -> Switch " 
          << node2 << ":" << dir2 << endl;

  return false;
} 

/* A switch looks like this:
 *  (port numbers on inside, port 0 is the local CPU)
 *
 *               N
 *               |
 *        +-------------+
 *        |      2      | 
 *        |             | 
 *   W -- | 1    0    3 | -- E
 *        |             | 
 *        |      4      | 
 *        +-------------+
 *               |
 *               S
 */
bool generateMeshTopology ( int nodeId )
{
  int
    x,
    y;

  x = getXCoord ( nodeId );
  y = getYCoord ( nodeId );

  // East
  if ( x != nodesPerDim[0]-1 )
    writeS2S ( getNodeIdCoord ( x, y ),
               EAST,
               getNodeIdCoord ( x+1, y ),
               WEST );

  // South
  if ( y != nodesPerDim[1]-1 ) 
    writeS2S ( getNodeIdCoord ( x, y ),
               SOUTH,
               getNodeIdCoord ( x, y+1 ),
               NORTH );  

  return false;
}

bool generateTorusTopology ( int nodeId )
{
  int
    x,
    y;

  x = getXCoord ( nodeId );
  y = getYCoord ( nodeId );

  // East 
  writeS2S ( getNodeIdCoord ( x, y ),
             EAST,
             getNodeIdCoord ( (x+1)%nodesPerDim[0], y ),
             WEST );
  
  // South
  writeS2S ( getNodeIdCoord ( x, y ),
             SOUTH,
             getNodeIdCoord ( x, (y+1)%nodesPerDim[1] ),
             NORTH );  
  
 return false;
}

bool writeBasicRoute ( int currNode,
                       int destNode,
                       Direction outPort,
                       int outVC )
{
  outFile << "Route Switch " << currNode << " -> " << destNode 
          << " { " << outPort << ":" << outVC << " } " << endl;
  return false;
}

#define ABS(X) ((X) > 0 ? (X) : -(X))

bool generateDeadlockFreeTorusRoute ( int currNode,
                                      int destNode )
{
  int
    xoff,
    yoff;

  // Trivial case, output to port 0, VC 0
  if ( currNode == destNode ) {
    return writeBasicRoute ( currNode, 
                             destNode,
                             LOCAL, 
                             0 );
  }

  xoff =
    getXCoord ( destNode ) - 
    getXCoord ( currNode );
  
  yoff =
    getYCoord ( destNode ) -
    getYCoord ( currNode );
  
  if ( xoff != 0 ) {
    
    if ( xoff > 0 && xoff < (nodesPerDim[0] / 2) || 
         xoff < (-nodesPerDim[0] / 2) ) {
      // Go EAST
      return writeBasicRoute ( currNode,
                               destNode,
                               EAST,
                               getXCoord ( destNode ) > 
                               getXCoord ( currNode ) );
    } else {
      // Go WEST
      return writeBasicRoute ( currNode,
                               destNode,
                               WEST,
                               getXCoord ( destNode ) > 
                               getXCoord ( currNode ) );
    }
  }

  if ( yoff > 0 && yoff < (nodesPerDim[1] / 2) ||
       yoff < (-nodesPerDim[1] / 2) ) {
    // Go SOUTH
    return writeBasicRoute ( currNode,
                             destNode,
                             SOUTH,
                             getYCoord ( destNode ) > 
                             getYCoord ( currNode ) );
  } else {    
    // Go NORTH
    return writeBasicRoute ( currNode,
                             destNode,
                             NORTH,
                             getYCoord ( destNode ) > 
                             getYCoord ( currNode ) );
  }
  
  return false;  
}


bool generateDeadlockFreeMeshRoute ( int currNode,
                                     int destNode )
{
  int
    xoff,
    yoff;

  xoff =
    getXCoord ( destNode ) - 
    getXCoord ( currNode );
 
  yoff =
    getYCoord ( destNode ) -
    getYCoord ( currNode );

  // Trivial case, output to port 0, VC 0
  if ( currNode == destNode ) {
    return writeBasicRoute ( currNode, 
                             destNode,
                             LOCAL, 
                             0 );
  }
  
  if ( xoff < 0 ) {
    return writeBasicRoute ( currNode,
                             destNode,
                             WEST,
                             0 );
  }

  if ( xoff > 0 ) {
    return writeBasicRoute ( currNode,
                             destNode,
                             EAST,
                             0 );   
  }
  
  if ( yoff < 0 ) {
    return writeBasicRoute ( currNode,
                             destNode,
                             NORTH,
                             0 );
  }
  
  if ( yoff > 0 ) { 
    return writeBasicRoute ( currNode,
                             destNode,
                             SOUTH,
                             0 );
  }

  cerr << "ERROR: mesh routing found no route from " << currNode << " to " << destNode << " offset is " << xoff << ", " << yoff <<  endl;

  return true;
}
