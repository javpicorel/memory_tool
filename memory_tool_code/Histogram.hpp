#ifndef FLEXUS_FAST3DCACHE_HISTOGRAM_HPP_INCLUDED
#define FLEXUS_FAST3DCACHE_HISTOGRAM_HPP_INCLUDED

#include <core/stats.hpp>
#include <iomanip>
#include <boost/lexical_cast.hpp>


using namespace Flexus::SharedTypes;
using namespace Flexus;
using namespace Stat;
using namespace std;

struct Histogram_Entry{
 unsigned int key;
 unsigned long value; 
};

struct Histogram {

  std::string name;
  unsigned int number_of_entries;
  Histogram_Entry *histogram;
  long double totalValue;  

  void sort() { 
    for(unsigned int i=0; i<number_of_entries-1; i++) 
     for(unsigned int j=i+1; j<number_of_entries; j++)
      if(histogram[i].value<histogram[j].value) { 
         unsigned int tmpk=histogram[i].key;
         histogram[i].key=histogram[j].key;
         histogram[j].key=tmpk;

         unsigned long tmpv=histogram[i].value;
         histogram[i].value=histogram[j].value;
         histogram[j].value=tmpv;
       }
  }
  void reset(){
  for(unsigned int i=0; i<number_of_entries; i++){
     histogram[i].key=i;
     histogram[i].value=0;
     totalValue=0.0;
   }
  }
  void print(ofstream& file) {
    sort();
    unsigned long tot=total();
    file<<std::endl<<"Histogram "<<name<<std::endl; 
    file<<"      value "<<"     count "<<"   percentage" <<std::endl;
    file<<"-----------------------------------------" <<std::endl;
    for(unsigned int i=0; i<number_of_entries; i++) 
      if(histogram[i].value!=0) file<<setw(10)<<histogram[i].key<<"   "<<setw(10)<<(histogram[i].value)<<"   "<<setw(6)<<setprecision(3)<<(100.0*(histogram[i].value)/tot)<<"%"<<std::endl;
      file<<std::endl<<"total =        "<<total()<<std::endl;
      file<<std::endl<<"average value= "<<average()<<std::endl; 
      file<<std::endl<<"second average value= "<<average2()<<std::endl; 
   }
    
   void print (ofstream& file, unsigned int entries_to_print){
        sort();
        unsigned long tot=total();
        file<<std::endl<<"Histogram "<<name<<std::endl; 
        file<<"      value "<<"     count "<<"   percentage" <<std::endl;
        file<<"-----------------------------------------" <<std::endl;
       if (number_of_entries<entries_to_print) entries_to_print = number_of_entries;
        for(unsigned int i=0; i<entries_to_print; i++) 
            if(histogram[i].value!=0) file<<setw(10)<<histogram[i].key<<"   "<<setw(10)<<(histogram[i].value)<<"   "<<setw(6)<<setprecision(3)<<(100.0*(histogram[i].value)/tot)<<"%"<<std::endl;
        file<<std::endl<<"total =                 "<<total()<<std::endl;
        file<<std::endl<<"average value =         "<<average()<<std::endl; 
        file<<std::endl<<"average value (check) = "<<average2()<<std::endl;
    }
 
  void add(unsigned int key){ 
      if(key>=number_of_entries-1) histogram[number_of_entries-1].value++;
      else histogram[key].value++;
      totalValue+=key;   
  } 

 double getPercentageByKey(unsigned int key ){
  unsigned long tot=total();
  return (double)(100.0*histogram[key].value/tot);
 } 

  

  unsigned long total(){
   unsigned long tot=0; 
   for(unsigned int i=0; i<number_of_entries; i++) tot+=(histogram[i].value);
   return tot;
  } 

  double average(){
   unsigned long tot=0; 
   double uk=0.0;
   for(unsigned int i=0; i<number_of_entries; i++){
       tot+=(histogram[i].value)*(histogram[i].key);
       uk+=histogram[i].value;
   }
   return tot/uk;
  }
 
 double average2(){
  double uk=0.0;
  for(unsigned int i=0; i<number_of_entries; i++) uk+=histogram[i].value;
  return totalValue/uk; 
 }
 
  Histogram(std::string const &aname, unsigned int entries) { 
    histogram = new Histogram_Entry[entries];
    name.clear();
    name.append(aname);
    number_of_entries = entries;  
    reset();          
  }

};


struct  HistogramArray{
	Histogram** histograms;
	std::string name;
	unsigned int entries;
	unsigned int numOfHistograms;

	HistogramArray(std::string const&aname, unsigned int ent, unsigned int num){
		init(aname, ent, num);
	}

	HistogramArray(std::string const&aname, unsigned int ent){
		init(aname, ent, ent);
	}  
	void init(std::string const&aname, unsigned int ent, unsigned int num){
		entries = ent;
		numOfHistograms = num;
		name.clear();
		name.append(aname);
		histograms = new Histogram* [num];
        for(unsigned i=0; i<num; i++){
            std:string nname;
            nname.clear();
            nname.append(aname);
            nname.append("-");
            std::string s = boost::lexical_cast<string>(i );
            nname.append(s);
            histograms[i]= new Histogram(nname, ent);
        } 
    } 
  
    Histogram* getHistogram(unsigned i){
        return (histograms[i]); 
    }
    
    void print(ofstream& file) {
        for(unsigned i=0; i<numOfHistograms; i++) {
            histograms[i]->print(file);
            file<<std::endl; 
        }
    }

    void print(ofstream& file, int n) {
        for(unsigned i=0; i<numOfHistograms; i++) {
            histograms[i]->print(file, n);
            file<<std::endl; 
        }
    }
    
};
#endif 
