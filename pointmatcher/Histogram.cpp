// kate: replace-tabs off; indent-width 4; indent-mode normal
// vim: ts=4:sw=4:noexpandtab
/*

Copyright (c) 2010--2011,
François Pomerleau and Stephane Magnenat, ASL, ETHZ, Switzerland
You can contact the authors at <f dot pomerleau at gmail dot com> and
<stephane at magnenat dot net>

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL ETH-ASL BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "Histogram.h"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <limits>
#include <algorithm>

namespace PointMatcherSupport
{
	using namespace std;
	
	template<typename T>
	Histogram<T>::Histogram(const size_t binCount, const std::string& name, const std::string& 
		filePrefix, const bool dumpStdErrOnExit):
		binCount(binCount),
		name(name),
		filePrefix(filePrefix),
		dumpStdErrOnExit(dumpStdErrOnExit)
	{
	}
	
	template<typename T>
	Histogram<T>::~Histogram()
	{
		T meanV, varV, medianV, lowQt, highQt, minV, maxV;
		uint64_t bins[binCount];
		uint64_t maxBinC;
		if (dumpStdErrOnExit || filePrefix.size() > 0)
			computeStats(meanV, varV, medianV, lowQt, highQt, minV, maxV, bins, maxBinC);
		
		if (filePrefix.size() > 0)
		{
			std::cerr << "writing to " << (filePrefix + name) << std::endl;
			std::ofstream ofs((filePrefix + name).c_str());
			for (size_t i = 0; i < this->size(); ++i)
				ofs << ((*this)[i]) << "\n";
		}
		
		if (dumpStdErrOnExit)
		{
			std::fill(bins, bins+binCount, uint64_t(0));
			std::cerr.precision(4);
			std::cerr.fill(' ');
			std::cerr.flags(std::ios::left);
			std::cerr << "Histogram " << name << ":\n";
			std::cerr << "  count: " << this->size() << ", mean: " << meanV << "\n";
			if(this->size() > 1)
			{
				for (size_t i = 0; i < binCount; ++i)
				{
					const T v(minV + i * (maxV - minV) / T(binCount));
					std::cerr << "  " << std::setw(10) << v << " (" << std::setw(6) << bins[i] << ") : ";
					//std::cerr << (bins[i] * 60) / maxBinC << " " ;
					for (size_t j = 0; j < (bins[i] * 60) / maxBinC; ++j)
						std::cerr << "*";
					std::cerr << "\n";
				}
				std::cerr << std::endl;
			}
		}
	}
	
	template<typename T>
	void Histogram<T>::computeStats(T& meanV, T& varV, T& medianV, T& lowQt, T& highQt, T& minV, T& maxV, uint64_t* bins, uint64_t& maxBinC)
	{
		typedef typename std::vector<T>::iterator Iterator;
		
		//assert(this->size() > 0);
		if(this->size() > 0)
		{
			// basic stats
			meanV = 0;
			minV = std::numeric_limits<T>::max();
			maxV = std::numeric_limits<T>::min();
			for (size_t i = 0; i < this->size(); ++i)
			{
				const T v((*this)[i]);
				meanV += v;
				minV = std::min<T>(minV, v);
				maxV = std::max<T>(maxV, v);
			}
			meanV /= T(this->size());
			// var and hist
			std::fill(bins, bins+binCount, uint64_t(0));
			maxBinC = 0;
			varV = 0;
			if (minV == maxV)
			{
				medianV = lowQt = highQt = minV;
				return;
			}
			for (size_t i = 0; i < this->size(); ++i)
			{
				const T v((*this)[i]);
				varV += (v - meanV)*(v - meanV);
				const size_t index((v - minV) * (binCount) / ((maxV - minV) * (1+std::numeric_limits<T>::epsilon()*10)));
				//std::cerr << "adding value " << v << " to index " << index << std::endl;
				++bins[index];
				maxBinC = std::max<uint64_t>(maxBinC, bins[index]);
			}
			varV /= T(this->size());
			// median
			const Iterator lowQtIt(this->begin() + (this->size() / 4));
			const Iterator medianIt(this->begin() + (this->size() / 2));
			const Iterator highQtIt(this->begin() + (3*this->size() / 4));
			std::nth_element(this->begin(), medianIt, this->end());
			medianV = *medianIt;
			std::nth_element(this->begin(), lowQtIt, this->end());
			lowQt = *lowQtIt;
			std::nth_element(this->begin(), highQtIt, this->end());
			highQt = *highQtIt;
		}
		else
		{
			meanV = std::numeric_limits<T>::quiet_NaN();
			varV = std::numeric_limits<T>::quiet_NaN();
			medianV = std::numeric_limits<T>::quiet_NaN();
			lowQt = std::numeric_limits<T>::quiet_NaN();
			highQt = std::numeric_limits<T>::quiet_NaN();
			minV = std::numeric_limits<T>::quiet_NaN();
			maxV = std::numeric_limits<T>::quiet_NaN();
			maxBinC = 0;
		}
	}
	
	template<typename T>
	void Histogram<T>::dumpStats(std::ostream& os)
	{
		T meanV, varV, medianV, lowQt, highQt, minV, maxV;
		uint64_t bins[binCount];
		uint64_t maxBinC;
		computeStats(meanV, varV, medianV, lowQt, highQt, minV, maxV, bins, maxBinC);
		os << meanV << " " << varV << " " << medianV << " " << lowQt << " " << highQt << " " << minV << " " << maxV << " " << binCount << " ";
		
		for (size_t i = 0; i < binCount; ++i)
			os << bins[i] << " ";
		os << maxBinC;
	}
	
	template struct Histogram<unsigned>;
	template struct Histogram<float>;
	template struct Histogram<double>;
} // namespace PointMatcherSupport
