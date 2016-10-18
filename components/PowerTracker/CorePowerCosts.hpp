/*------------------------------------------------------------
 *  Copyright 1994 Digital Equipment Corporation and Steve Wilton
 *                         All Rights Reserved
 *
 * Permission to use, copy, and modify this software and its documentation is
 * hereby granted only under the following terms and conditions.  Both the
 * above copyright notice and this permission notice must appear in all copies
 * of the software, derivative works or modified versions, and any portions
 * thereof, and both notices must appear in supporting documentation.
 *
 * Users of this software agree to the terms and conditions set forth herein,
 * and hereby grant back to Digital a non-exclusive, unrestricted, royalty-
 * free right and license under any changes, enhancements or extensions
 * made to the core functions of the software, including but not limited to
 * those affording compatibility with other hardware or software
 * environments, but excluding applications which incorporate this software.
 * Users further agree to use their best efforts to return to Digital any
 * such changes, enhancements or extensions that they make and inform Digital
 * of noteworthy uses of this software.  Correspondence should be provided
 * to Digital at:
 *
 *                       Director of Licensing
 *                       Western Research Laboratory
 *                       Digital Equipment Corporation
 *                       100 Hamilton Avenue
 *                       Palo Alto, California  94301
 *
 * This software may be distributed (but not offered for sale or transferred
 * for compensation) to third parties, provided such third parties agree to
 * abide by the terms and conditions of this notice.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND DIGITAL EQUIPMENT CORP. DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS.   IN NO EVENT SHALL DIGITAL EQUIPMENT
 * CORPORATION BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *------------------------------------------------------------*/

#ifndef FLEXUS_CORE_POWER_COSTS_HPP_INCLUDED
#define FLEXUS_CORE_POWER_COSTS_HPP_INCLUDED

#include <iostream>

class CorePowerCosts {
public:
	CorePowerCosts();
	CorePowerCosts(const CorePowerCosts &source);
	CorePowerCosts& operator=(const CorePowerCosts &source);

private:
	// Parameters
	double vdd,
		   frq;
	double baseVdd; // Baseline VDD - use by CACTI in computing cache configurations
	int decodeWidth,
		issueWidth,
		commitWidth,
		windowSize,
		numIntPhysicalRegisters, 
		numFpPhysicalRegisters,
		numIntArchitecturalRegs, 
		numFpArchitecturalRegs,
		lsqSize,
		dataPathWidth,
		instructionLength;
		
	int virtualAddressSize;
	
	// Functional units
	int resIntAlu, // Includes ALUs and multipliers because Wattch cannot differentiate
		resFpAdd,
		resFpMult,
		resMemPort;

	// Branch predictor parameters
	int btbNumSets,
		btbAssociativity,
		twoLevelL1Size,
		twoLevelL2Size,
		twoLevelHistorySize,
		bimodalTableSize,
		metaTableSize,
		rasSize;

	// Cache parameters
	int	l1iNumSets,			// L1I
		l1iAssociativity,
		l1iBlockSize,
		l1dNumSets,			// L1D
		l1dAssociativity,
		l1dBlockSize;

	// TLB parameters
	int	itlbNumSets,		// ITLB
		itlbAssociativity,
		itlbPageSize,
		dtlbNumSets,		// DTLB
		dtlbAssociativity,
		dtlbPageSize;
	
	int systemWidth;

	double clockCap;

	// Per-access dynamic power
	double l1iPerAccessDynamic,
		   itlbPerAccessDynamic,
		   intRenamePerAccessDynamic,
		   fpRenamePerAccessDynamic,
		   bpredPerAccessDynamic,
		   intRegFilePerAccessDynamic,
		   fpRegFilePerAccessDynamic,
		   resultBusPerAccessDynamic,
		   selectionPerAccessDynamic,
		   rsPerAccessDynamic,
		   wakeupPerAccessDynamic,
		   intAluPerAccessDynamic,
		   fpAluPerAccessDynamic,
		   fpAddPerAccessDynamic,
		   fpMulPerAccessDynamic,
		   lsqWakeupPerAccessDynamic,
		   lsqRsPerAccessDynamic,
		   l1dPerAccessDynamic,
		   dtlbPerAccessDynamic,
		   clockPerCycleDynamic,
		   maxCycleDynamic;

public:
	// Setters for parameters
	inline void setVdd(const double newVdd) { vdd = newVdd; }
	inline void setBaseVdd(const double newVdd) { baseVdd = newVdd; }
	inline void setFrq(const double newFrq) { frq = newFrq; }
	inline void setDecodeWidth(const int newDecodeWidth) { decodeWidth = newDecodeWidth; }
	inline void setIssueWidth(const int newIssueWidth) { issueWidth = newIssueWidth; }
	inline void setCommitWidth(const int newCommitWidth) { commitWidth = newCommitWidth; }
	inline void setWindowSize(const int newWindowSize) { windowSize = newWindowSize; }
	inline void setNumIntPhysicalRegisters(const int newNumIntPhysicalRegisters) { numIntPhysicalRegisters = newNumIntPhysicalRegisters; }
	inline void setNumFpPhysicalRegisters(const int newNumFpPhysicalRegisters) { numFpPhysicalRegisters = newNumFpPhysicalRegisters; }
	inline void setNumIntArchitecturalRegs(const int newNumIntArchitecturalRegs) { numIntArchitecturalRegs = newNumIntArchitecturalRegs; }
	inline void setNumFpArchitecturalRegs(const int newNumFpArchitecturalRegs) { numFpArchitecturalRegs = newNumFpArchitecturalRegs; }
	inline void setLsqSize(const int newLsqSize) { lsqSize = newLsqSize; }
	inline void setDataPathWidth(const int newDataPathWidth) { dataPathWidth = newDataPathWidth;	}
	inline void setInstructionLength(const int newInstructionLength) { instructionLength = newInstructionLength;	}
	inline void setVirtualAddressSize(const int newVirtualAddressSize) { virtualAddressSize = newVirtualAddressSize; }
	inline void setResIntAlu(const int newResIntAlu) { resIntAlu = newResIntAlu; }
	inline void setResFpAdd(const int newResFpAdd) { resFpAdd = newResFpAdd; }
	inline void setResFpMult(const int newResFpMult) { resFpMult = newResFpMult; }
	inline void setResMemPort(const int newResMemPort) { resMemPort = newResMemPort; }
	inline void setBtbNumSets(const int newBtbNumSets) { btbNumSets = newBtbNumSets; }
	inline void setBtbAssociativity(const int newBtbAssociativity) { btbAssociativity = newBtbAssociativity; }
	inline void setTwoLevelL1Size(const int newTwoLevelL1Size) { twoLevelL1Size = newTwoLevelL1Size; }
	inline void setTwoLevelL2Size(const int newTwoLevelL2Size) { twoLevelL2Size = newTwoLevelL2Size; }
	inline void setTwoLevelHistorySize(const int newTwoLevelHistorySize) { twoLevelHistorySize = newTwoLevelHistorySize; }
	inline void setBimodalTableSize(const int newBimodalTableSize) { bimodalTableSize = newBimodalTableSize; }
	inline void setMetaTableSize(const int newMetaTableSize) { metaTableSize = newMetaTableSize; }
	inline void setRasSize(const int newRasSize) { rasSize = newRasSize; }
	inline void setL1iNumSets(const int newL1iNumSets) { l1iNumSets = newL1iNumSets; }
	inline void setL1iAssociativity(const int newL1iAssociativity) { l1iAssociativity = newL1iAssociativity; }
	inline void setL1iBlockSize(const int newL1iBlockSize) { l1iBlockSize = newL1iBlockSize; }
	inline void setL1dNumSets(const int newL1dNumSets) { l1dNumSets = newL1dNumSets; }
	inline void setL1dAssociativity(const int newL1dAssociativity) { l1dAssociativity = newL1dAssociativity; }
	inline void setL1dBlockSize(const int newL1dBlockSize) { l1dBlockSize = newL1dBlockSize; }
	inline void setItlbNumSets(const int newItlbNumSets) { itlbNumSets = newItlbNumSets; }
	inline void setItlbAssociativity(const int newItlbAssociativity) { itlbAssociativity = newItlbAssociativity; }
	inline void setItlbPageSize(const int newItlbPageSize) { itlbPageSize = newItlbPageSize; }
	inline void setDtlbNumSets(const int newDtlbNumSets) { dtlbNumSets = newDtlbNumSets; }
	inline void setDtlbAssociativity(const int newDtlbAssociativity) { dtlbAssociativity = newDtlbAssociativity; }
	inline void setDtlbPageSize(const int newDtlbPageSize) { dtlbPageSize = newDtlbPageSize; }
	inline void setSystemWidth(const int newSystemWidth) { systemWidth = newSystemWidth; }

	// Getters for parameters
	inline double getVdd() const { return vdd; }
	inline double getBaseVdd() const { return baseVdd; }
	inline double getFrq() const { return frq; }
	inline int getDecodeWidth() const { return decodeWidth; }
	inline int getIssueWidth() const { return issueWidth; }
	inline int getCommitWidth() const { return commitWidth; }
	inline int getWindowSize() const { return windowSize; }
	inline int getNumIntPhysicalRegisters() const { return numIntPhysicalRegisters; }
	inline int getNumFpPhysicalRegisters() const { return numFpPhysicalRegisters; }
	inline int getNumIntArchitecturalRegs() const { return numIntArchitecturalRegs; }
	inline int getNumFpArchitecturalRegs() const { return numFpArchitecturalRegs; }
	inline int getLsqSize() const { return lsqSize; }
	inline int getDataPathWidth() const { return dataPathWidth; }
	inline int getInstructionLength() const { return instructionLength; }
	inline int getVirtualAddressSize() const { return virtualAddressSize; }
	inline int getResIntAlu() const { return resIntAlu; }
	inline int getResFpAdd() const { return resFpAdd; }
	inline int getResFpMult() const { return resFpMult; }
	inline int getResMemPort() const { return resMemPort; }
	inline int getBtbNumSets() const { return btbNumSets; }
	inline int getBtbAssociativity() const { return btbAssociativity; }
	inline int getTwoLevelL1Size() const { return twoLevelL1Size; }
	inline int getTwoLevelL2Size() const { return twoLevelL2Size; }
	inline int getTwoLevelHistorySize() const { return twoLevelHistorySize; }
	inline int getBimodalTableSize() const { return bimodalTableSize; }
	inline int getMetaTableSize() const { return metaTableSize; }
	inline int getRasSize() const { return rasSize; }
	inline int getL1iNumSets() const { return l1iNumSets; }
	inline int getL1iAssociativity() const { return l1iAssociativity; }
	inline int getL1iBlockSize() const { return l1iBlockSize; }
	inline int getL1dNumSets() const { return l1dNumSets; }
	inline int getL1dAssociativity() const { return l1dAssociativity; }
	inline int getL1dBlockSize() const { return l1dBlockSize; }
	inline int getItlbNumSets() const { return itlbNumSets; }
	inline int getItlbAssociativity() const { return itlbAssociativity; }
	inline int getItlbPageSize() const { return itlbPageSize; }
	inline int getDtlbNumSets() const { return dtlbNumSets; }
	inline int getDtlbAssociativity() const { return dtlbAssociativity; }
	inline int getDtlbPageSize() const { return dtlbPageSize; }
	inline int getSystemWidth() const { return systemWidth; }

	// Getters for power costs
	inline double getL1iPerAccessDynamic() const { return l1iPerAccessDynamic; }
	inline double getItlbPerAccessDynamic() const { return itlbPerAccessDynamic; }
	inline double getIntRenamePerAccessDynamic() const { return intRenamePerAccessDynamic; }
	inline double getFpRenamePerAccessDynamic() const { return fpRenamePerAccessDynamic; }
	inline double getBpredPerAccessDynamic() const { return bpredPerAccessDynamic; }
	inline double getIntRegFilePerAccessDynamic() const { return intRegFilePerAccessDynamic; }
	inline double getFpRegFilePerAccessDynamic() const { return fpRegFilePerAccessDynamic; }
	inline double getResultBusPerAccessDynamic() const { return resultBusPerAccessDynamic; }
	inline double getSelectionPerAccessDynamic() const { return selectionPerAccessDynamic; }
	inline double getRsPerAccessDynamic() const { return rsPerAccessDynamic; }
	inline double getWakeupPerAccessDynamic() const { return wakeupPerAccessDynamic; }
	inline double getIntAluPerAccessDynamic() const { return intAluPerAccessDynamic; }
	inline double getFpAluPerAccessDynamic() const { return fpAluPerAccessDynamic; }
	inline double getFpAddPerAccessDynamic() const { return fpAddPerAccessDynamic; }
	inline double getFpMulPerAccessDynamic() const { return fpMulPerAccessDynamic; }
	inline double getLsqWakeupPerAccessDynamic() const { return lsqWakeupPerAccessDynamic; }
	inline double getLsqRsPerAccessDynamic() const { return lsqRsPerAccessDynamic; }
	inline double getL1dPerAccessDynamic() const { return l1dPerAccessDynamic; }
	inline double getDtlbPerAccessDynamic() const { return dtlbPerAccessDynamic; }
	inline double getClockPerCycleDynamic() const { return clockPerCycleDynamic; }
	inline double getMaxCycleDynamic() const { return maxCycleDynamic; }

	// Calculate the power costs
	void computeAccessCosts();

private:
	void calculate_core_power();
	double perCoreClockPower(const double tile_width, const double tile_height, const double die_width, const double die_height);
};
#endif
