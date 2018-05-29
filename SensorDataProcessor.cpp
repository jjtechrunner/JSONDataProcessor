/***************** Temperature Data Processing written in C++*******************/
/*  This program expects temperature reading from sensors in a JSON format
    as input in a file and identifies the average, median and mode of the
	temperatures per sensor and displays in the results in JSON array format
	possibly in a sorted order.
*/

// standard library headers
#include <fstream>
#include <iostream>
#include <sstream>

// boost library headers
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/algorithm/string/erase.hpp>




// Hard coded error codes
const int kECInvalidInputFile=1000;
const int kErrorReadingJSON=1001;

namespace TDP //Temperature Data Processor
{
	//Exception class used to register error with error/warning code and error text	
	class TDPException
	{	
		public:
			// constructor takes error code and error text
			TDPException(int errorCode, const std::string& iErrorText)
				 : _errorCode(errorCode),
				   _errorText(iErrorText)
			{}
			
			// copy constructor
		   TDPException(const TDPException& iRHS)
		   {
			   _errorCode = iRHS.getErrorCode();
			   _errorText = iRHS.getErrorText();
		   }
			
			const int getErrorCode() const
			{
				return _errorCode;
			}
			
			const std::string& getErrorText() const
			{
				return _errorText;
			}
		   
		    // returns both error code and error text as concatenated string with a separator '-'
		    const std::string getErrorString() const
		    {
				std::stringstream ss;
				ss << _errorCode << "-" << _errorText;
				return ss.str();
		    }

		private:
		   int _errorCode; // error code
		   std::string _errorText; // error text
		   TDPException()
		   {}		
	};

	// Class defines the temperature reading with timestamp
	// not used at the moment and keeping for future evolution in 
	// case if there is a requirement to display the temperature data 
	// in the order of the time sstamp per sensor and also
	// display average, median and mode per day. NOT USED at the moment	
	class TemperatureReading
	{
		public:
			TemperatureReading()
				:_temperature(0.00)
			{}
			
			void setTimeStamp(const std::string& iTimeStamp)
			{
				_timeStamp = iTimeStamp;
			}
			
			void setTemperature(const float& iTemp)
			{
				_temperature = iTemp;
			}
			
			std::string getTimeStamp() const
			{
				return _timeStamp;
			}
			
			std::string& getTimeStamp()
			{
				return _timeStamp;
			}
			
			float getTemperature() const
			{
				return _temperature;
			}
			
			float getTemperature()
			{
				return _temperature;
			}
			
			bool operator <(const TemperatureReading& iRHS)
			{
				return _timeStamp < iRHS.getTimeStamp();
			}
			
			bool operator==(const TemperatureReading& iTmpread)
			{
				return _timeStamp == iTmpread.getTimeStamp();
			}
		
		private:
			std::string _timeStamp;
			float  _temperature;
	};

	// Represeting Sensor object with temperature readings
	// timestamp is not stored at the moment
	// computes the total while adding the temperature, so that we can find the 
	// average with complexity O(1), no need to traverse the vector again
	class Sensor
	{
		public:
			// constructor
			Sensor(std::string id):
			_id(id),
			_sorted(false),
			_totalTemperature(0.0)
			{}
			
			// copy constructor
			Sensor(const Sensor& iObj)
			{
				_id = iObj.getId();
				_sorted = iObj.getSorted();
				_temperatureReadings = iObj.getTemperatureReadings();
				_totalTemperature = iObj.getTotalTemperature();
			}
		
		public:
			// add temperature reading to the sensor object
			void addTemperatureReading(const float& iTemperature)
			{
				_totalTemperature = _totalTemperature + iTemperature;				
				_temperatureReadings.push_back(iTemperature);
			}
			
			// writing for the symmetry
			void removeTemperatureReading(const TemperatureReading& iTempReading)
			{
				// TBC - Not required at this stage
			}
			
			// sort the vector for the temperature readings
			void sortData()
			{
				if(!_sorted)
				{
					std::sort(_temperatureReadings.begin(), _temperatureReadings.end());
					_sorted = true;
				}				
			}
			
			// get methods
			
			// return identifier of sensor
			const std::string getId() const
			{
				return _id;
			}
			
			// return temperature readings
			const std::vector<float>& getTemperatureReadings() const
			{
				return _temperatureReadings;
			}
			
			// return the average of the temperature readings
			const float getAverage() const
			{
				float average = _totalTemperature/_temperatureReadings.size();
				// round to 2 decimal places
				return std::floor((average * 100) + .5) / 100;
			}
			
			// return the median of the temperature readings
			const float getMedian() const
			{
				const size_t n = _temperatureReadings.size();
				float median = (n % 2 > 0)
				               ? _temperatureReadings[n/2]
							   : ((_temperatureReadings[(n-1)/2] + _temperatureReadings[n/2])/2);

				// round to 2 decimal places							   
				return std::floor((median * 100) + .5) / 100;
			}
			
			// return the mode(s)) of the temperature readings
			std::vector<float> getModes() const
			{
				std::map<int, std::vector<float> > modeValues;
				std::vector<float>::const_iterator i, j;
				int maxCount=1;
				for(i=_temperatureReadings.begin();i < _temperatureReadings.end(); ++i)
				{
					int count = 1;
					for(j=i+1; j < _temperatureReadings.end(); ++j)
					{
						if((*i) == (*j))
						{
							count++;
						}
						else
						{
							break;
						}
					}

					if(count > 1 && count >= maxCount)						
					{
						maxCount = count;
						modeValues[maxCount].push_back((*i));
					}
					i = j;
				}				
				return modeValues[maxCount];
			}	
			
			const bool getSorted() const
			{
				return _sorted;
			}
			
			// get the sum of tje temperature readings
			const float getTotalTemperature() const
			{
				return _totalTemperature;
			}
		private:
			std::string _id;  // Sensor identifier
			float _totalTemperature; // sum of temperature reading to help to find average
			std::vector<float> _temperatureReadings; // storing the temperature readings
			bool _sorted; // indicates whether the readings are sorted or not
	};
	
} // end namesapce TDP




int main(int argc, char *argv[])
{	
	// display usage
	if(argc < 2)
	{
		std::cout <<"Refrigerator Temperature Processor written by J. Jose \n";
		std::cout <<"================ Usage is =========================== \n";
		std::cout << "          -i <input file> \n";
		std::cin.get();
		exit(0);
	}


	//extract the parameters
	std::string inputFile;
	if(!std::strcmp(argv[1], "-i"))
	{	
		inputFile = argv[2];
	}
	

	// check input file is populated
	if(inputFile.empty())
	{
		std::cout << "Required arguments are not populated, Please try again.\n";
		return 0;
	}

	try
	{
		try
		{
			std::map<std::string, TDP::Sensor> _sensorData;
			
			// Short alias for this namespace
			namespace pt = boost::property_tree;

			// read the JSON array data using boost property tree
			pt::ptree jsonData;

			// Load the json file in this ptree
			pt::read_json(inputFile, jsonData);

			// iterate through JSON data and create the Sensor objects
			for(auto v = jsonData.begin(); v != jsonData.end(); ++v) 
			{
				const std::string sensorId =  v->second.get_child("id").get_value<std::string>();
				const float temperature = v->second.get_child("temperature").get_value<float>();
						  
				// populate the sensor data
				auto sensorDataIt = _sensorData.find(sensorId);
				if(sensorDataIt != _sensorData.end())
				{
					sensorDataIt->second.addTemperatureReading(temperature);
				}
				else
				{
					TDP::Sensor sensorObj(sensorId);
					sensorObj.addTemperatureReading(temperature);
					_sensorData.insert(std::make_pair(sensorId, sensorObj));;
				}
			}
			
			// TO DO - Even though it is not so explicit in the requirement
			// output seems to be sorted based on average. This is not done 
			// at this stage.
			
			std::stringstream ss;
			ss << '[';
			bool first = true;
			
			// generate JOSN array output
			for(auto i = _sensorData.begin(); i != _sensorData.end(); ++i)
			{
				// sort the temp readings
				i->second.sortData();

				if(first)
				{
					first = false;
				}
				else
				{
					ss << ",\n";
				}
				
				ss << '{' << '\"' << "id\":\"" <<  i->first << "\"," 
				                  <<"\"average\":\"" << i->second.getAverage() << "\","
								  <<"\"median\":\"" << i->second.getMedian() << "\","
								  <<"\"mode\":[";
								  
				const std::vector<float> modes = i->second.getModes();
				bool firstMode = true;
				for(auto j = modes.begin(); j < modes.end(); j++)
				{
					if(firstMode)
					{
						firstMode = false;
					}
					else
					{
						ss << ',';
					}
					ss << *j;
				}
				
				ss <<"]}";
			}
				
			ss << ']';
			
			std::cout << ss.str() << std::endl;			

		}	
		catch (const boost::property_tree::ptree_error& e) 
		{
			std::cout << "Exception in processing JSN data " << e.what() << std::endl;
			std::string aExceptionStr = "Exception in processing input file " + inputFile;
			throw TDP::TDPException(kErrorReadingJSON, aExceptionStr);
		}
	}
	catch(const TDP::TDPException& ex)
	{
		std::cout << "KO: Exception in processing temperature readings " << ex.getErrorString() << std::endl;
	}
	catch(...)
	{
		std::cout << "KO: Unknown exception" << std::endl;
	}
	
	return 0;
}

