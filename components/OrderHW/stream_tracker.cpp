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

#include "stream_tracker.hpp"
#include <fstream>
#include <boost/tuple/tuple.hpp>

std::ostream & operator <<( std::ostream & anOstream, stream_t const & aStream) {
  for (int i = 0; i < aStream.length(); ++i) {
    anOstream << std::hex << aStream[i] << " ";
  }
  return anOstream;
}

void StreamTracker::countStream(stream_t aStream, int aNode) {
  stream_map::iterator iter;
  bool is_new;

  boost::tie(iter, is_new) = theStreams.insert( std::make_pair(aStream, stream_record()) );
  iter->second.theOccurrences++;
  iter->second.theOccurrencesPerNode[aNode]++;
  if (is_new) {
    theNumUniqueStreams++;
    theTotalSymbols += aStream.length();  //ignores tree overhead
    long estimated_memory = theTotalSymbols * 4 + sizeof(stream_record) * theNumUniqueStreams;  //ignores tree overhead
    if ( estimated_memory - theLastReport > 10000) {
      theLastReport = estimated_memory;
      std::cout << "StreamTracker: " << theNumUniqueStreams << " unique streams; " << theTotalSymbols << " total symbols in streams; " << estimated_memory << " memory consumed" << std::endl;
    }
  }
}


void StreamTracker::printStreams() {
  std::cout << "Finalizing stream tracker" << std::endl;
  int i = 0;
  std::ofstream ot("stream_occurences.out");
  for (stream_map::iterator iter = theStreams.begin(), end = theStreams.end(); iter != end; ++iter) {
    ot << std::setw(5) << i << "\t" << iter->first.length() << "\t" << iter->second.theOccurrences << "\t";
    for (int i = 0; i < 16; ++i) {
      ot << std::setw(3) << iter->second.theOccurrencesPerNode[i] << " ";
    }
    ot << "\n";
    ++i;
  }
  ot.close();

  i = 0;
  std::ofstream str("streams.out");
  str << theTotalSymbols << " total symbols\n";
  str << theNumUniqueStreams << " unique streams\n";
  for (stream_map::iterator iter = theStreams.begin(), end = theStreams.end(); iter != end; ++iter) {
    str << std::setw(5) << std::dec << i << "\t=\t" << iter->first << "\n";
    ++i;
  }
  str.close();
}

void StreamTracker::finalize() {
  printStreams();
}

class Histogram {
    std::vector<unsigned long > theBuckets;
  public:
    void insert( unsigned long anUpdate ) {
      if (theBuckets.size() < anUpdate + 1) {
          theBuckets.resize(anUpdate+1,0);
      }
      theBuckets[anUpdate]++;
    }
    void print(std::ostream & anOstream, std::string const & options = std::string("")) const {
      if (buckets() > 0 ) {
        if (options.size() > 0) {
          if (options == "buckets") {
            anOstream << buckets();
            return;
          } else if (options.substr(0,4) == "sum:") {
            int begin = 0;
            int end = buckets() - 1;
            try {
              std::string range = options.substr(4);
              unsigned int dash = range.find("-");
              if (dash == std::string::npos) {
                //No dash
                begin = end = boost::lexical_cast<int>(range);
              } else {
                std::string begin_str = range.substr(0,dash);
                if (begin_str.size() > 0) {
                  begin = boost::lexical_cast<int>(begin_str);
                } else {
                  begin = 0;
                }
                std::string end_str = range.substr(dash+1);
                if (end_str.size() > 0) {
                  end = boost::lexical_cast<int>(end_str);
                } else {
                  end = buckets() - 1;
                }
              }
            } catch (boost::bad_lexical_cast &) {
               anOstream << "{sum bounds cannot be parsed}";
            } catch (std::out_of_range &) {
              anOstream << "{sum bounds out of range}";
              return;
            }
            long long sum = 0;
            for (int i = begin; i <= end && i < buckets(); ++i) {
              sum += bucketVal(i);
            }
            anOstream << sum;
            return;
          } else {
            anOstream << "{Unrecognized option: " << options << " }";
          }
        }

        anOstream << "\n\tBucket\tSize\n";
        anOstream << "\t" << 0 << ":" << "\t" << bucketVal(0) << "\n";
        int label = 1;
        unsigned long sum = bucketVal(0);
        for (int i = 1; i < buckets(); ++i) {
         anOstream << "\t" << label << ":" << "\t" << bucketVal(i) << "\n";
         label++;
         sum += bucketVal(i);
        }
        anOstream << "\tsum:" << "\t" << sum << "\n\n";
      }
    }
    int buckets() const { return theBuckets.size(); }
    virtual long long bucketVal(int i) const { return theBuckets[i]; }
};


struct UniqueStreamTracker : public StreamTrackerBase {

  typedef std::set < tAddress> UniqueStreamSet;
  UniqueStreamSet theZeroSet;
  UniqueStreamSet theOneSet;

  unsigned long theTotalZeroHits;
  unsigned long theTotalZeroInstances;
  unsigned long theTotalOneHits;
  unsigned long theTotalOneInstances;


  UniqueStreamTracker()
    : theTotalZeroHits(0)
    , theTotalZeroInstances(0)
    , theTotalOneHits(0)
    , theTotalOneInstances(0)
    {}


  void countHeadsPerRegion(UniqueStreamSet & aSet, std::ostream & anOstream) {
    int sizes[] = { 64, 256, 1024, 2048, 8192 };
    int masks[] = { ~63UL, ~255UL, ~1023UL, ~2047UL, ~8191UL};
    Histogram heads_per_block[5];
    tAddress addr[5];
    unsigned long count[5];

    if (aSet.empty()) return;

    tAddress first = * aSet.begin();
    for (int i = 0; i < 5 ; ++ i) {
      addr[i] = first & masks[i];
      count[i] = 0;
    }

    for (UniqueStreamSet::iterator iter = aSet.begin(), end = aSet.end(); iter != end; ++iter) {
      for (int i = 0; i < 5 ; ++ i) {
         if ( (*iter & masks[i]) == addr[i]) {
            ++count[i];
         } else {
            //Accumulate current entry
            heads_per_block[i].insert(count[i]);

            //Reset for new entry
            count[i] = 1;
            addr[i] = *iter & masks[i];
         }
      }
    }
    for (int i = 0; i < 5 ; ++ i) {
      if (count[i] > 0) {
        heads_per_block[i].insert(count[i]);
      }
      anOstream << "Heads per block at " << sizes[i] << " granularity\n";
      heads_per_block[i].print(anOstream);
    }
  }

  void addStream( tAddress aHead, unsigned long aHits) {
    if (aHits > 0) {
      theZeroSet.insert(aHead);
      theTotalZeroInstances++;
      theTotalZeroHits += aHits;
      if (aHits > 1) {
        theOneSet.insert(aHead);
        theTotalOneInstances++;
        theTotalOneHits += aHits;
      }
    }
/*
    StreamInfo & info = theMap[aHead];
    info.theTotalInstances++;
    info.theTotalHits += aHits;
    if (info.theTotalHits > theMaxHits) {
      theMaxHits = info.theTotalHits;
      theMaxHitAddress = aHead;
    }
    if (info.theTotalInstances > theMaxInstances) {
      theMaxInstances = info.theTotalInstances;
      theMaxInstanceAddress = aHead;
    }
 */
  }

  virtual void finalize() {
    std::ofstream stream("stream_info.out");
    stream << "Unique Stream Info - >=1 hit" << std::endl;
    stream << "  Total Hits: " << theTotalZeroHits << std::endl;
    stream << "  Non-0 Streams: " << theTotalZeroInstances << std::endl;
    stream << "  Unique Heads: " << theZeroSet.size() << std::endl;
    //stream << "  Most frequent head (" << std::hex << theMaxInstanceAddress << std::dec << "): " << theMaxInstances << std::endl;
    //stream << "  Most important head (" << std::hex << theMaxHitAddress << std::dec << "): " << theMaxHits << std::endl;
    countHeadsPerRegion(theZeroSet, stream);

    stream << std::endl << std::endl;
    stream << "Unique Stream Info - >= 2 hits" << std::endl;
    stream << "  Total Hits: " << theTotalOneHits << std::endl;
    stream << "  Non-0 Streams: " << theTotalOneInstances << std::endl;
    stream << "  Unique Heads: " << theOneSet.size() << std::endl;
    //stream << "  Most frequent head (" << std::hex << theMaxInstanceAddress << std::dec << "): " << theMaxInstances << std::endl;
    //stream << "  Most important head (" << std::hex << theMaxHitAddress << std::dec << "): " << theMaxHits << std::endl;
    countHeadsPerRegion(theOneSet, stream);

    stream.close();
  }

};

StreamTracker theTracker;

StreamTrackerBase * theStreamTracker = &theTracker;
