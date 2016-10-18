// DO-NOT-REMOVE begin-copyright-block 
//                                     
// Redistributions of any form whatsoever must retain and/or include the     
// following acknowledgment, notices and disclaimer:                         
//                                                                           
// This product includes software developed by Carnegie Mellon University.   
//                                                                           
// Copyright 2005 by Kun Gao Brian Gold, Nikos Hardavellas, Jangwoo Kim,     
// Ippokratis Pandis, Minglong Shao, Jared Smolens, Stephen Somogyi,         
// Tom Wenisch, Anastassia Ailamaki, Babak Falsafi and James C. Hoe for      
// the SimFlex Project, Computer Architecture Lab at Carnegie Mellon,        
// Carnegie Mellon University.                                               
//                                                                           
// For more information, see the SimFlex project website at:                 
//   http://www.ece.cmu.edu/~simflex                                         
//                                                                           
// You may not use the name Carnegie Mellon University or derivations      
// thereof to endorse or promote products derived from this software.        
//                                                                           
// If you modify the software you must place a notice on or within any       
// modified version provided or made available to any third party stating    
// that you have modified the software.  The notice shall include at least   
// your name, address, phone number, email address and the date and purpose  
// of the modification.                                                      
//                                                                           
// THE SOFTWARE IS PROVIDED AS-IS WITHOUT ANY WARRANTY OF ANY KIND, EITHER 
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

// The Flexus PowerTracker uses power computation routines taken from Wattch, 
// which is available at http://www.eecs.harvard.edu/~dbrooks/wattch-form.html.
// Wattch was developed by David Brooks (dbrooks@eecs.harvard.edu) and Margaret 
// Martonosi (mrm@Princeton.edu) 

// The TemperatureTracker provides an interface to HotSpot, which is available
// at http://lava.cs.virginia.edu/HotSpot. The HotSpot license is available in
// components/PowerTracker/HotSpot/LICENSE.

#include <vector>

#include <core/simulator_layout.hpp>

#include <components/Common/Slices/AbstractInstruction.hpp>
#include <components/Common/Transports/MemoryTransport.hpp>

#define FLEXUS_BEGIN_COMPONENT PowerTracker
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(
	PARAMETER( Frequency, double, "Clock frequency", "frq", 3e9)
	PARAMETER( NumL2Tiles, unsigned int, "Number of L2 cache tiles", "numL2Tiles", 1)
	
	PARAMETER( DecodeWidth, unsigned int, "Decode width", "decodeWidth", 4)
	PARAMETER( IssueWidth, unsigned int, "Issue width", "issueWidth", 4)
	PARAMETER( CommitWidth, unsigned int, "Commit width", "commitWidth", 4)
	PARAMETER( WindowSize, unsigned int, "Instruction window size", "windowSize", 64)
    PARAMETER( NumIntPhysicalRegisters, unsigned int, "Number of integer physical registers", "numIntPregs", 128)
    PARAMETER( NumFpPhysicalRegisters, unsigned int, "Number of floating point physical registers", "numFpPregs", 128)
    PARAMETER( NumIntArchitecturalRegs, unsigned int, "Number of integer architectural registers", "numIntAregs", 32)
    PARAMETER( NumFpArchitecturalRegs, unsigned int, "Number of floating point architectural registers", "numFpAregs", 32)
    
	PARAMETER( LsqSize, unsigned int, "Load/store queue size", "lsqSize", 64)
	PARAMETER( DataPathWidth, unsigned int, "Datapath width in bits", "dataPathWidth", 64)
	PARAMETER( InstructionLength, unsigned int, "Instruction length in bits", "instructionLength", 32)
	PARAMETER( VirtualAddressSize, unsigned int, "Virtual address length in bits", "virtualAddressSize", 64)
	PARAMETER( NumIntAlu, unsigned int, "Number of integer ALUs", "numIntAlu", 4)
	PARAMETER( NumIntMult, unsigned int, "Number of integer multipliers", "numIntMult", 1)
	PARAMETER( NumFpAdd, unsigned int, "Number of floating point ADD/SUB units", "numFpAdd", 2)
	PARAMETER( NumFpMult, unsigned int, "Number of floating point MUL/DIV units", "numFpMult", 1)
	PARAMETER( NumMemPort, unsigned int, "Number of memory access ports", "numMemPort", 2)
	
	// Branch predictor parameters
	PARAMETER( BtbNumSets, unsigned int, "Number of sets in branch target buffer", "btbNumSets", 2048)
	PARAMETER( BtbAssociativity, unsigned int, "Associativity of branch target buffer", "btbAssociativity", 16)
	PARAMETER( GShareSize, unsigned int, "Width of gshare shift register", "gShareSize", 13)
	PARAMETER( BimodalTableSize, unsigned int, "Table size for bimodal predictor", "bimodalTableSize", 16384)
	PARAMETER( MetaTableSize, unsigned int, "Metapredictor table size", "metaTableSize", 16384)
	PARAMETER( RasSize, unsigned int, "Return address stack size", "rasSize", 8)
		
	// Cache parameters
	PARAMETER( L1iNumSets, unsigned int, "Number of sets in L1 instruction cache", "l1iNumSets", 512)
	PARAMETER( L1iAssociativity, unsigned int, "Associativity of L1 instruction cache", "l1iAssociativity", 2)
	PARAMETER( L1iBlockSize, unsigned int, "Block size of L1 instruction cache in bytes", "l1iBlockSize", 64)
	PARAMETER( L1dNumSets, unsigned int, "Number of sets in L1 data cache", "l1dNumSets", 512)
	PARAMETER( L1dAssociativity, unsigned int, "Associativity of L1 data cache", "l1dAssociativity", 2)
	PARAMETER( L1dBlockSize, unsigned int, "Block size of L1 data cache in bytes", "l1dBlockSize", 64)
	PARAMETER( L2uNumSets, unsigned int, "Number of sets in one L2 unified cache tile", "l2uNumSets", 4096)
	PARAMETER( L2uAssociativity, unsigned int, "Associativity of L2 unified cache", "l2uAssociativity", 8)
	PARAMETER( L2uBlockSize, unsigned int, "Block size of L2 unified cache in bytes", "l2uBlockSize", 128)
	
	// TLB parameters	
	PARAMETER( ItlbNumSets, unsigned int, "Number of sets in instruction TLB", "itlbNumSets", 1)
	PARAMETER( ItlbAssociativity, unsigned int, "Associativity of instruction TLB", "itlbAssociativity", 128)
	PARAMETER( ItlbPageSize, unsigned int, "Page size of instruction pages", "itlbPageSize", 4096)
	PARAMETER( DtlbNumSets, unsigned int, "Number of sets in data TLB", "dtlbNumSets", 1)
	PARAMETER( DtlbAssociativity, unsigned int, "Associativity of data TLB", "dtlbAssociativity", 128)
	PARAMETER( DtlbPageSize, unsigned int, "Page size of data pages", "dtlbPageSize", 4096)
	
	// Thermal model
	PARAMETER( HotSpotChipThickness, double, "Chip thickness in meters", "hotSpotChipThickness", 0.00015)	
	PARAMETER( HotSpotConvectionCapacitance, double, "Convection capacitance in J/K", "hotSpotConvectionCapacitance", 140.4)	
	PARAMETER( HotSpotConvectionResistance, double, "Convection resistance in K/W", "hotSpotConvectionResistance", 0.1)	
	PARAMETER( HotSpotHeatsinkSide, double, "Heatsink side in meters", "hotSpotHeatsinkSide", 0.06)
	PARAMETER( HotSpotHeatsinkThickness, double, "Heatsink thickness in meters", "hotSpotHeatsinkThickness", 0.0069)
	PARAMETER( HotSpotHeatSpreaderSide, double, "Heat spreader side in meters", "hotSpotHeatSpreadeSide", 0.03)
	PARAMETER( HotSpotHeatSpreaderThickness, double, "Heat spreader thickness in meters", "hotSpotHeatSpreadeThickness", 0.001)
	PARAMETER( HotSpotInterfaceMaterialThickness, double, "Thermal interface material thickness in meters", "hotSpotChipInterfaceMaterialThickness", 2e-5)
	PARAMETER( HotSpotAmbientTemperature, double, "Celsius ambient temperature", "hotSpotAmbientTemperature", 45)	
	PARAMETER( HotSpotFloorpanFile, std::string, "HotSpot floorplan file", "hotSpotFloorplanFile", "")
	PARAMETER( HotSpotInitialTemperatureFile, std::string, "HotSpot initial temperature input file", "hotSpotInitialTemperatureFile", "")
	PARAMETER( HotSpotSteadyStateTemperatureFile, std::string, "HotSpot steady-state temperature output file", "hotSpotSteadyStateTemperatureFile", "")
	PARAMETER( HotSpotInterval, unsigned int, "Number of cycles between temperature updates", "hotSpotInterval", 10000)
	PARAMETER( HotSpotResetCycle, unsigned int, "Reset HotSpot power and cycle counts after this many cycles to ensure steady-state temperatures are not computed using warm-up period", "hotSpotResetCycle", 0)
	
	PARAMETER( EnablePowerTracker, bool, "Whether PowerTracker is enabled", "enablePowerTracker", false)
	PARAMETER( EnableTemperatureTracker, bool, "Whether TemperatureTracker is enabled", "enableTemperatureTracker", false)
	PARAMETER( DefaultTemperature, double, "Celsius temperature used for all blocks if TemperatureTracker is disabled", "defaultTemperature", 100)
	PARAMETER( PowerStatInterval, unsigned long, "Power stats are only updated every this many baseline cycles. Aggregation reduces accuracy impact of not having a double StatCounter", "powerStatInterval", 50000)
);

COMPONENT_INTERFACE(
	DYNAMIC_PORT_ARRAY( PushInput, bool, L1iRequestIn)
	DYNAMIC_PORT_ARRAY( PushInput, long, DispatchedInstructionIn)
	DYNAMIC_PORT_ARRAY( PushInput, bool, L1dRequestIn)
	DYNAMIC_PORT_ARRAY( PushInput, bool, L2uRequestIn)
	DYNAMIC_PORT_ARRAY( PushInput, bool, StoreForwardingHitIn)
	DYNAMIC_PORT_ARRAY( PushInput, bool, CoreClockTickIn)
	DYNAMIC_PORT_ARRAY( PushInput, bool, L2ClockTickIn)
	
	// These interfaces could be useful if one is making decisions based on runtime power or temperature measurements
	DYNAMIC_PORT_ARRAY( PullOutput, double, CoreAveragePowerOut)	// Average power used by core since last check
	DYNAMIC_PORT_ARRAY( PullOutput, double, L2AveragePowerOut)		// Average power used by L2 tile since last check
	DYNAMIC_PORT_ARRAY( PullOutput, double, CoreTemperatureOut)
	DYNAMIC_PORT_ARRAY( PullOutput, double, L2TemperatureOut)

    DRIVE( PowerTrackerDrive )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT PowerTracker
