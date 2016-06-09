#ifndef _POSITIONSERVICE_H_
#define _POSITIONSERVICE_H_

#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <map>

// CElementFile reads one data element.
// Data element can be primitive type, vector, or user defined type, as long as it 
// supports >> operator.
template <typename TElement>
class CElementFile
{
public:
	// Initialize CElementFile with file name
	CElementFile(std::string fileName) : m_sFileName{ fileName } {}
	// Read file content to data element
	TElement Read()
	{
		TElement element;
		std::ifstream ifs(m_sFileName);
		if (!ifs)
			throw std::runtime_error("Can't open file to read: " + m_sFileName);
		ifs >> element;
		return element;
	}

private:
	std::string m_sFileName; //file name including path for ticker file
};

// Make CElementFile works for a vector of elements
template<typename TElement>
std::istream& operator>>(std::istream& is, std::vector<TElement>& velement)
{
	TElement element;
	while (is >> element)
	{
		velement.push_back(element);
	}
	return is;
}

// Fill message represents one line in fills file.
struct SFillMessage
{
	std::string m_sMessageType; // always F
	long long m_nMilliseconds; // Milliseconds from the unix epoch
	std::string m_sSymbolName; 
	double m_dFillPrice;
	int m_nFillSize;
	std::string m_sSideIndicator; // B for buy and S for sell
	bool IsBuyOrder() const { return m_sSideIndicator == "B"; }
};
// Make CElementFile working for SFillMessage
std::istream& operator>>(std::istream& is, SFillMessage& element);

// Price message represents one line in prices file.
struct SPriceMessage
{
	std::string m_sMessageType; // always P
	long long m_nMilliseconds; // Milliseconds from the unix timestamp
	std::string m_sSymbolName;
	double m_dCurrentPrice;
};
// Make CElementFile working for SPriceMessage
std::istream& operator>>(std::istream& is, SPriceMessage& element);

// PNL message represents one line to stdout.
struct SPnLMessage
{
	std::string m_sMessageType; // always PNL
	long long m_nMilliseconds; // Milliseconds from the unix timestamp
	std::string m_sSymbolName;
	int m_nCurrentSize; // signed size owned
	double m_dPnL; // mark to market P&L
};
// Make stdout working for SPnLMessage
std::ostream& operator<<(std::ostream& os, const SPnLMessage& element);

// define fills and prices files
typedef CElementFile<std::vector<SFillMessage>> CFillMessageFile;
typedef CElementFile<std::vector<SPriceMessage>> CPriceMessageFile;

class CPositionService; // forward declaration
// CMessageService is a publisher which owns fill and price update messages.  It processes each price update message
// in prices file and notify subscriber (CPositionService) of price update message.
class CMessageService
{
public:
	void Subscribe(CPositionService* pPositionService) { m_pPositionService = pPositionService; }
	// Read fill and price update messages from file and store messages in member variables.
	void ReadMessages(const std::string& sFillsFile, const std::string& sPricesFile);
	// Go through each price update message and notify subsriber
	void ProcessPriceMessages();

private:
	CPositionService* m_pPositionService; // subscriber of message service
	std::vector<SFillMessage> m_cFillMessages; // store fill messages from fills file
	std::vector<SPriceMessage> m_cPriceMessages; // store price update messages from prices file
};

// CPositionService is a subscriber to CMessageService.  For each price update notification from CMessageService, it
// keeps tracking of current positions.
class CPositionService
{
public:
	// Given price update notification, update current positions and output PnL message.
	void OnUpdatePriceMessage(const SPriceMessage& priceMessage, const std::vector<SFillMessage>& fillMessages);

private:
	// keep tracking of current positions, key is symbol and value is a pair of <total cash used, size>
	std::map<std::string, std::pair<double, int>> m_cPositions; 
};

#endif
