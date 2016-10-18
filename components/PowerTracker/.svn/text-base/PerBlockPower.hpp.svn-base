#ifndef FLEXUS_PER_BLOCK_POWER_HPP_INCLUDED
#define FLEXUS_PER_BLOCK_POWER_HPP_INCLUDED

class PerBlockPower {
	struct CoreBlockPowers {
		double 	l1iDynamic,
				itlbDynamic,
				bpredDynamic,
				intRenameDynamic,
				fpRenameDynamic,
				intRegfileDynamic,
				fpRegfileDynamic,
				windowDynamic,
				intAluDynamic,
				fpAddDynamic,
				fpMulDynamic,
				l1dDynamic,
				dtlbDynamic,
				lsqDynamic,
				resultBusDynamic,
				clockDynamic,
				l1iLeakage,
				itlbLeakage,
				bpredLeakage,
				intRenameLeakage,
				fpRenameLeakage,
				intRegfileLeakage,
				fpRegfileLeakage,
				windowLeakage,
				intAluLeakage,
				fpAddLeakage,
				fpMulLeakage,
				l1dLeakage,
				dtlbLeakage,
				lsqLeakage,
				resultBusLeakage,
				clockLeakage;
	};
	
	
public:
	PerBlockPower();
	~PerBlockPower();
	void setNumTiles(const unsigned int newNmCoreTiles, const unsigned int newNumL2Tiles);

	// Get individual block dynamic or static power value
	inline double getL1iDynamic(const unsigned int coreIndex) const { return corePowers[coreIndex].l1iDynamic; };
	inline double getItlbDynamic(const unsigned int coreIndex) const { return corePowers[coreIndex].itlbDynamic; };
	inline double getBpredDynamic(const unsigned int coreIndex) const { return corePowers[coreIndex].bpredDynamic; };
	inline double getIntRenameDynamic(const unsigned int coreIndex) const { return corePowers[coreIndex].intRenameDynamic; };
	inline double getFpRenameDynamic(const unsigned int coreIndex) const { return corePowers[coreIndex].fpRenameDynamic; };
	inline double getIntRegfileDynamic(const unsigned int coreIndex) const { return corePowers[coreIndex].intRegfileDynamic; };
	inline double getFpRegfileDynamic(const unsigned int coreIndex) const { return corePowers[coreIndex].fpRegfileDynamic; };
	inline double getWindowDynamic(const unsigned int coreIndex) const { return corePowers[coreIndex].windowDynamic; };
	inline double getIntAluDynamic(const unsigned int coreIndex) const { return corePowers[coreIndex].intAluDynamic; };
	inline double getFpAddDynamic(const unsigned int coreIndex) const { return corePowers[coreIndex].fpAddDynamic; };
	inline double getFpMulDynamic(const unsigned int coreIndex) const { return corePowers[coreIndex].fpMulDynamic; };
	inline double getL1dDynamic(const unsigned int coreIndex) const { return corePowers[coreIndex].l1dDynamic; };
	inline double getDtlbDynamic(const unsigned int coreIndex) const { return corePowers[coreIndex].dtlbDynamic; };
	inline double getLsqDynamic(const unsigned int coreIndex) const { return corePowers[coreIndex].lsqDynamic; };
	inline double getResultBusDynamic(const unsigned int coreIndex) const { return corePowers[coreIndex].resultBusDynamic; }
	inline double getClockDynamic(const unsigned int coreIndex) const { return corePowers[coreIndex].clockDynamic; };
	inline double getL1iLeakage(const unsigned int coreIndex) const { return corePowers[coreIndex].l1iLeakage; };
	inline double getItlbLeakage(const unsigned int coreIndex) const { return corePowers[coreIndex].itlbLeakage; };
	inline double getBpredLeakage(const unsigned int coreIndex) const { return corePowers[coreIndex].bpredLeakage; };
	inline double getIntRenameLeakage(const unsigned int coreIndex) const { return corePowers[coreIndex].intRenameLeakage; };
	inline double getFpRenameLeakage(const unsigned int coreIndex) const { return corePowers[coreIndex].fpRenameLeakage; };
	inline double getIntRegfileLeakage(const unsigned int coreIndex) const { return corePowers[coreIndex].intRegfileLeakage; };
	inline double getFpRegfileLeakage(const unsigned int coreIndex) const { return corePowers[coreIndex].fpRegfileLeakage; };
	inline double getWindowLeakage(const unsigned int coreIndex) const { return corePowers[coreIndex].windowLeakage; };
	inline double getIntAluLeakage(const unsigned int coreIndex) const { return corePowers[coreIndex].intAluLeakage; };
	inline double getFpAddLeakage(const unsigned int coreIndex) const { return corePowers[coreIndex].fpAddLeakage; };
	inline double getFpMulLeakage(const unsigned int coreIndex) const { return corePowers[coreIndex].fpMulLeakage; };
	inline double getL1dLeakage(const unsigned int coreIndex) const { return corePowers[coreIndex].l1dLeakage; };
	inline double getDtlbLeakage(const unsigned int coreIndex) const { return corePowers[coreIndex].dtlbDynamic; };
	inline double getLsqLeakage(const unsigned int coreIndex) const { return corePowers[coreIndex].lsqLeakage; };
	inline double getResultBusLeakage(const unsigned int coreIndex) const { return corePowers[coreIndex].resultBusLeakage; }
	inline double getClockLeakage(const unsigned int coreIndex) const { return corePowers[coreIndex].clockLeakage; }
	
	// Get individual block total dynamic plus static power
	inline double getL1iPower(const unsigned int coreIndex) const { return corePowers[coreIndex].l1iDynamic + corePowers[coreIndex].l1iLeakage; };
	inline double getItlbPower(const unsigned int coreIndex) const { return corePowers[coreIndex].itlbDynamic + corePowers[coreIndex].itlbLeakage; };
	inline double getBpredPower(const unsigned int coreIndex) const { return corePowers[coreIndex].bpredDynamic + corePowers[coreIndex].bpredLeakage; };
	inline double getIntRenamePower(const unsigned int coreIndex) const { return corePowers[coreIndex].intRenameDynamic + corePowers[coreIndex].intRenameLeakage; };
	inline double getFpRenamePower(const unsigned int coreIndex) const { return corePowers[coreIndex].fpRenameDynamic + corePowers[coreIndex].fpRenameLeakage; };
	inline double getIntRegfilePower(const unsigned int coreIndex) const { return corePowers[coreIndex].intRegfileDynamic + corePowers[coreIndex].intRegfileLeakage; };
	inline double getFpRegfilePower(const unsigned int coreIndex) const { return corePowers[coreIndex].fpRegfileDynamic + corePowers[coreIndex].fpRegfileLeakage; };
	inline double getWindowPower(const unsigned int coreIndex) const { return corePowers[coreIndex].windowDynamic + corePowers[coreIndex].windowLeakage; };
	inline double getIntAluPower(const unsigned int coreIndex) const { return corePowers[coreIndex].intAluDynamic + corePowers[coreIndex].intAluLeakage; };
	inline double getFpAddPower(const unsigned int coreIndex) const { return corePowers[coreIndex].fpAddDynamic + corePowers[coreIndex].fpAddLeakage; };
	inline double getFpMulPower(const unsigned int coreIndex) const { return corePowers[coreIndex].fpMulDynamic + corePowers[coreIndex].fpMulLeakage; };
	inline double getL1dPower(const unsigned int coreIndex) const { return corePowers[coreIndex].l1dDynamic + corePowers[coreIndex].l1dLeakage; };
	inline double getDtlbPower(const unsigned int coreIndex) const { return corePowers[coreIndex].dtlbDynamic + corePowers[coreIndex].dtlbLeakage; };
	inline double getLsqPower(const unsigned int coreIndex) const { return corePowers[coreIndex].lsqDynamic + corePowers[coreIndex].lsqLeakage; };
	inline double getResultBusPower(const unsigned int coreIndex) const { return corePowers[coreIndex].resultBusDynamic + corePowers[coreIndex].resultBusLeakage; };
	inline double getClockPower(const unsigned int coreIndex) const { return corePowers[coreIndex].clockDynamic + corePowers[coreIndex].clockLeakage; };
	
	// Add to power values
	inline void addToL1iDynamic(const unsigned int coreIndex, const double valueToAdd) { corePowers[coreIndex].l1iDynamic += valueToAdd; };
	inline void addToItlbDynamic(const unsigned int coreIndex, const double valueToAdd) { corePowers[coreIndex].itlbDynamic += valueToAdd; };
	inline void addToBpredDynamic(const unsigned int coreIndex, const double valueToAdd) { corePowers[coreIndex].bpredDynamic += valueToAdd; };
	inline void addToIntRenameDynamic(const unsigned int coreIndex, const double valueToAdd) { corePowers[coreIndex].intRenameDynamic += valueToAdd; };
	inline void addToFpRenameDynamic(const unsigned int coreIndex, const double valueToAdd) { corePowers[coreIndex].fpRenameDynamic += valueToAdd; };
	inline void addToIntRegfileDynamic(const unsigned int coreIndex, const double valueToAdd) { corePowers[coreIndex].intRegfileDynamic += valueToAdd; };
	inline void addToFpRegfileDynamic(const unsigned int coreIndex, const double valueToAdd) { corePowers[coreIndex].fpRegfileDynamic += valueToAdd; };
	inline void addToWindowDynamic(const unsigned int coreIndex, const double valueToAdd) { corePowers[coreIndex].windowDynamic += valueToAdd; };
	inline void addToIntAluDynamic(const unsigned int coreIndex, const double valueToAdd) { corePowers[coreIndex].intAluDynamic += valueToAdd; };
	inline void addToFpAddDynamic(const unsigned int coreIndex, const double valueToAdd) { corePowers[coreIndex].fpAddDynamic += valueToAdd; };
	inline void addToFpMulDynamic(const unsigned int coreIndex, const double valueToAdd) { corePowers[coreIndex].fpMulDynamic += valueToAdd; };
	inline void addToL1dDynamic(const unsigned int coreIndex, const double valueToAdd) { corePowers[coreIndex].l1dDynamic += valueToAdd; };
	inline void addToDtlbDynamic(const unsigned int coreIndex, const double valueToAdd) { corePowers[coreIndex].dtlbDynamic += valueToAdd; };
	inline void addToClockDynamic(const unsigned int coreIndex, const double valueToAdd) { corePowers[coreIndex].clockDynamic += valueToAdd; };
	inline void addToLsqDynamic(const unsigned int coreIndex, const double valueToAdd) { corePowers[coreIndex].lsqDynamic += valueToAdd; };
	inline void addToResultBusDynamic(const unsigned int coreIndex, const double valueToAdd) { corePowers[coreIndex].resultBusDynamic += valueToAdd; };
	inline void addToL1iLeakage(const unsigned int coreIndex, const double valueToAdd) { corePowers[coreIndex].l1iLeakage += valueToAdd; };
	inline void addToItlbLeakage(const unsigned int coreIndex, const double valueToAdd) { corePowers[coreIndex].itlbLeakage += valueToAdd; };
	inline void addToBpredLeakage(const unsigned int coreIndex, const double valueToAdd) { corePowers[coreIndex].bpredLeakage += valueToAdd; };
	inline void addToIntRenameLeakage(const unsigned int coreIndex, const double valueToAdd) { corePowers[coreIndex].intRenameLeakage += valueToAdd; };
	inline void addToFpRenameLeakage(const unsigned int coreIndex, const double valueToAdd) { corePowers[coreIndex].fpRenameLeakage += valueToAdd; };
	inline void addToIntRegfileLeakage(const unsigned int coreIndex, const double valueToAdd) { corePowers[coreIndex].intRegfileLeakage += valueToAdd; };
	inline void addToFpRegfileLeakage(const unsigned int coreIndex, const double valueToAdd) { corePowers[coreIndex].fpRegfileLeakage += valueToAdd; };
	inline void addToWindowLeakage(const unsigned int coreIndex, const double valueToAdd) { corePowers[coreIndex].windowLeakage += valueToAdd; };
	inline void addToIntAluLeakage(const unsigned int coreIndex, const double valueToAdd) { corePowers[coreIndex].intAluLeakage += valueToAdd; };
	inline void addToFpAddLeakage(const unsigned int coreIndex, const double valueToAdd) { corePowers[coreIndex].fpAddLeakage += valueToAdd; };
	inline void addToFpMulLeakage(const unsigned int coreIndex, const double valueToAdd) { corePowers[coreIndex].fpMulLeakage += valueToAdd; };
	inline void addToL1dLeakage(const unsigned int coreIndex, const double valueToAdd) { corePowers[coreIndex].l1dLeakage += valueToAdd; };
	inline void addToDtlbLeakage(const unsigned int coreIndex, const double valueToAdd) { corePowers[coreIndex].dtlbDynamic += valueToAdd; };
	inline void addToLsqLeakage(const unsigned int coreIndex, const double valueToAdd) { corePowers[coreIndex].lsqLeakage += valueToAdd; };
	inline void addToResultBusLeakage(const unsigned int coreIndex, const double valueToAdd) { corePowers[coreIndex].resultBusLeakage += valueToAdd; };
	inline void addToClockLeakage(const unsigned int coreIndex, const double valueToAdd) { corePowers[coreIndex].clockLeakage += valueToAdd; };
	inline void addToL2uDynamic(const unsigned int l2Index, const double valueToAdd) { l2uDynamic[l2Index] += valueToAdd; };
	inline void addToL2uLeakage(const unsigned int l2Index, const double valueToAdd) { l2uLeakage[l2Index] += valueToAdd; };

	// Get power for a single core tile
	double getCoreDynamic(const unsigned int i) const;
	double getCoreLeakage(const unsigned int i) const;
	double getCorePower(const unsigned int i) const;
	
		// Get power for a single L2 tile
	inline double getL2uDynamic(const unsigned int l2Index) const { return l2uDynamic[l2Index]; };
	inline double getL2uLeakage(const unsigned int l2Index) const { return l2uLeakage[l2Index]; };
	inline double getL2uPower(const unsigned int l2Index) const { return l2uDynamic[l2Index] + l2uLeakage[l2Index]; };
	
	double getTotalDynamic() const;
	double getTotalLeakage() const;
	double getTotalPower() const;
		
	PerBlockPower& operator=(const PerBlockPower &source);
	PerBlockPower operator-(const PerBlockPower &p2) const;
	
private:
	unsigned int numCoreTiles;
	unsigned int numL2Tiles;
	
	CoreBlockPowers * corePowers;
	double *l2uDynamic,
		   *l2uLeakage;
};

#endif
