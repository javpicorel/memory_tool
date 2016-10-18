#ifndef FLEXUS_TEMPERATURE_TRACKER_HPP_INCLUDED
#define FLEXUS_TEMPERATURE_TRACKER_HPP_INCLUDED

// The TemperatureTracker provides an interface to HotSpot, which is available
// at http://lava.cs.virginia.edu/HotSpot. The HotSpot license is available in
// components/PowerTracker/HotSpot/LICENSE.

#include <components/PowerTracker/HotSpot/temperature.h>
#include <components/PowerTracker/PerBlockPower.hpp>
#include <string>

class TemperatureTracker {
public:
	TemperatureTracker() : enabled(false) {}
	~TemperatureTracker();
	
	void initialize(thermal_config_t &config, const std::string floorplanFile, const unsigned int interval, const unsigned int aNumCoreTiles, const unsigned int aNumL2Tiles);

	void addToCycles(const unsigned long x) { cycles += x; }

	void updateTemperature(const PerBlockPower &intervalPower);
	double getKelvinTemperature(const std::string blockName);
	double getCoreTemperature(const unsigned int i);
	double getL2Temperature(const unsigned int i);
	void setConstantTemperature(const double temp) { constantTemp = temp + 273.15; }
	void writeSteadyStateTemperatures();
	
	void reset();
	
private:
	bool enabled;
	double constantTemp;
	
	flp_t *flp; 							// floorplan
	RC_model_t *model;						// temperature model
	double *hs_temp, *hs_power;				// instantaneous temperature and power values
	double *overall_power, *steady_temp;	// steady state temperature and power values
	double *avg_power;						
	unsigned long cycles;
	
	unsigned int numCoreTiles;
	unsigned int numL2Tiles;
	unsigned int cycleInterval;
	
	// Block area as a proportion of total area. Used to assign clock power
	double icacheAreaProportion,
		   dcacheAreaProportion,
		   bpredAreaProportion,
		   dtbAreaProportion,
		   fpAddAreaProportion,
		   fpRegAreaProportion,
		   fpMulAreaProportion,
		   fpMapAreaProportion,
		   intMapAreaProportion,
		   windowAreaProportion,
		   intRegAreaProportion,
		   intExecAreaProportion,
		   ldStQAreaProportion,
		   itbAreaProportion;
};
#endif
